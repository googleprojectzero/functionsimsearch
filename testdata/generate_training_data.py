#!/usr/bin/python3

# A Python script to generate training data given ELF or PE files including their
# relevant debug info.
#
# This Python script is meant to replace the horrible bash script that is
# currently responsible for doing this.

import subprocess, os, fnmatch, codecs, random, shutil, multiprocessing, glob
import sys
from absl import app
from absl import flags
from subprocess import Popen, PIPE, STDOUT
from operator import itemgetter
from collections import defaultdict

# Make sure you have a trailing slash.
flags.DEFINE_string('work_directory',
  "/media/thomasdullien/storage/functionsimsearch/train_data/",
  "The directory into which the training data will be written")

# Generate the fingerprint hashes.
flags.DEFINE_boolean('generate_fingerprints', True, "Decides whether the " +
  "hashes of all features should be extracted and written.")

# Generate full disassemblies in JSON, too. This is not necessary for any of
# the tools in this repository, but may be useful if you wish to experiment
# with disassembly data in other machine-learning contexts.
flags.DEFINE_boolean('generate_json_data', True, "Decides whether JSON output " +
  "of the full disassembly should be extracted and written (not necessary for " +
  "training, but useful for diagnostics and visualization.")

# Disable the use of mnemonic data.
flags.DEFINE_boolean('disable_mnemonic', False, "Disable the extraction of " +
  "mnemonic-based features. Useful to test the power of mnemonics vs. graphs.")

# Clobber existing data directory or not.
flags.DEFINE_boolean('clobber', False, "Clobber output directory or not.")

# Directory for executable files to train on.
flags.DEFINE_string('executable_directory', './',
  "The directory where the ELF and PE executables to train on can be found " +\
  "in their relevant subdirectories ELF/**/* and PE/**/*")

#=============================================================================

FLAGS = flags.FLAGS

def FindELFTrainingFiles():
  """ Returns the list of ELF files that should be used for training. These
  ELF files need to contain objdump-able debug information.

  """
  elf_files = [ filename for filename in glob.iglob(
    FLAGS.executable_directory + 'ELF/**/*', recursive=True)
    if os.path.isfile(filename) ]
  print("Returning list of files from ELF directory: %s" % elf_files)
  return elf_files

def FindPETrainingFiles():
  """ Returns the list of PE files that should be used for training. These
  PE files need to have associated text files (with suffix .debugdump) that
  contains the output of dia2dump in the same directory. """
  exe_files = [ filename for filename in glob.iglob(
    FLAGS.executable_directory + 'PE/**/*.exe',
    recursive=True) if os.path.isfile(filename) ]
  dll_files = [ filename for filename in glob.iglob(
    FLAGS.executable_directory + 'PE/**/*.dll',
    recursive=True) if os.path.isfile(filename) ]
  result = exe_files + dll_files
  print("Returning list of files from PE directory: %s" % result)
  return result

def ObtainFunctionSymbols(training_file, file_format):
  if file_format == "ELF":
    return ObtainELFFunctionSymbols(training_file)
  elif file_format == "PE":
    return ObtainPEFunctionSymbols(training_file)

def SaneBase64(input_string):
  """ Because Python3 attempts to win 'most idiotic language ever', encoding a
  simple string as base64 without risking to have strange newlines added is
  difficult. This functions is an insane solution: Call command line
  base64encode instead of dealing with Python. """
  encoded_string = subprocess.run(["base64", "-w0"], stdout=PIPE,
    input=bytes(input_string, encoding="utf-8")).stdout.decode("utf-8")
  return encoded_string

def ObtainELFFunctionSymbols(training_file):
  """ Runs objdump to obtain the symbols in an ELF file and then returns a
  dictionary for this file. """
  result = {}
  symbols = [ line for line in subprocess.check_output(
    [ "objdump", "-t", training_file ] ).decode("utf-8").split("\n")
      if line.find(" F .text") != -1 ]
  syms_and_address = []
  for sym in symbols:
    tokens = sym.split()
    if tokens[2] == 'F':
      address = int(tokens[0], 16)
      sym = SaneBase64(tokens[5])
      result[address] = sym
  return result

def find_nth(haystack, needle, n):
  start = haystack.find(needle)
  while start >= 0 and n > 1:
    start = haystack.find(needle, start+len(needle))
    n -= 1
  return start

def ObtainPEFunctionSymbols(training_file):
  result = {}
  filetype = subprocess.check_output(["file", "-b", training_file]).decode("utf-8")
  if filetype == "PE32+ executable (console) x86-64, for MS Windows\n":
    default_base = 0x140000000
  elif filetype == "PE32 executable (console) Intel 80386, for MS Windows\n":
    default_base = 0x400000
  elif filetype == "PE32 executable (DLL) (GUI) Intel 80386, for MS Windows\n":
    default_base = 0x10000000
  else:
    print("Problem: %s has unknown file type" % training_file)
    print("Filetype is: %s" % filetype)
  function_lines = [
    line for line in open(training_file + ".debugdump", "rt", errors='ignore').readlines() if
    line.find("Function") != -1 and line.find("static") != -1
    and line.find("crt") == -1 ]
  for line in function_lines:
    symbol = line[ find_nth(line, ", ", 3) + 2 :]
    try:
      address = int(line.split("[")[1].split("]")[0], 16) + default_base
    except:
      print("Invalid line, failed to split - %s" % line)
      continue
    # We still need to stem and encode the symbol.
    stemmed_symbol = subprocess.run(["../bin/stemsymbol"], stdout=PIPE,
      input=bytes(symbol, encoding="utf-8")).stdout
    if len(stemmed_symbol) > 0:
      result[address] = SaneBase64(stemmed_symbol.decode("utf-8"))
  return result

def ObtainDisassembledFunctions(training_file_id):
  """ Returns a sorted list with the functions that the disassembler found.
  """
  try:
    inputdata = open( FLAGS.work_directory + "/" + "functions_%s.txt" % training_file_id,
      "rt" ).readlines()
  except:
    print("Could not open functions_%s.txt, returning empty list." % training_file_id)
    return []
  data = [ int(line.split()[0].split(":")[1], 16) for line in inputdata ]
  data.sort()
  return data

def RunJSONDotgraphs(argument_tuple):
  """ Run the bin/dotgraphs utility to generate JSON output from the
  disassembly and write the output to a directory named after the file id. """
  training_file = argument_tuple[0]
  file_id = argument_tuple[1]
  file_format = argument_tuple[2]

  # Make the directory.
  directory_name = FLAGS.work_directory + "/" + "json_%s" % file_id
  shutil.rmtree(directory_name, ignore_errors=True)
  try:
    os.mkdir(directory_name)
  except OSError:
    print("Directory %s exists already." % directory_name)

  try:
    fingerprints = subprocess.check_call(
      [ "../bin/dotgraphs", "--no_shared_blocks", "--json",
      "--format=%s" % file_format, "--input=%s" % training_file, 
      "--output=%s" % directory_name])
  except:
    print("Failure to run dotgraphs (%s:%s->%s)" % \
      (file_format, training_file, file_id))


  print("Done with dotgraphs. (%s:%s->%s)" % \
    (file_format, training_file, file_id))

def RunFunctionFingerprints(argument_tuple):
  """ Run the bin/functionfingerprints utility to generate features from the
  disassembly and write the output to a file named after the file id. """
  training_file = argument_tuple[0]
  file_id = argument_tuple[1]
  file_format = argument_tuple[2]
  if FLAGS.disable_mnemonic:
    mnemonic = "--disable_instructions=true"
  else:
    mnemonic = "--disable_instructions=false"
 
  write_fingerprints = open(
    FLAGS.work_directory + "/" + "functions_%s.txt" % file_id, "wt")
 
  try:
    fingerprints = subprocess.check_call(
      [ "../bin/functionfingerprints", "--no_shared_blocks",
      "--format=%s" % file_format, "--input=%s" % training_file, 
      "--minimum_function_size=5", "--verbose=true", mnemonic ], 
      stdout = write_fingerprints)
  except:
    print("Failure to run functionfingerprints (%s:%s->%s)" % \
      (file_format, training_file, file_id))

  write_fingerprints.close()
  print("Done with functionfingerprints. (%s:%s->%s)" % \
    (file_format, training_file, file_id))

def ProcessTrainingFiles(training_files, file_format):
  # Begin by launching a pool of parallel processes to call
  # RunFunctionFingerprints.
  argument_tuples = []
  for training_file in training_files:
    sha256sum = subprocess.check_output(["sha256sum", training_file]).split()[0]
    file_id = sha256sum[0:16].decode("utf-8")
    argument_tuples.append((training_file, file_id, file_format))
  # Use a quarter of the available cores.
  pool = multiprocessing.Pool(max(1, int(multiprocessing.cpu_count() / 8)))
  if FLAGS.generate_fingerprints:
    print("Running functionfingerprints on all files.")
    pool.map(RunFunctionFingerprints, argument_tuples)
  if FLAGS.generate_json_data:
    print("Running dotgraphs on all files.")
    pool.map(RunJSONDotgraphs, argument_tuples)

  for training_file in training_files:
    sha256sum = subprocess.check_output(["sha256sum", training_file]).split()[0]
    file_id = sha256sum[0:16].decode("utf-8")
    # Run objdump
    print("Obtaining function symbols from %s... " % training_file, end='')
    objdump_symbols = ObtainFunctionSymbols(training_file, file_format)
    # Get the functions that our disassembly could find.
    print("Getting disassembled functions. ", end='')
    disassembled_functions = ObtainDisassembledFunctions(file_id)
    # Write the symbols for the functions that the disassembly found.
    print("Opening and writing extracted_symbols_%s.txt. " % file_id, end='')
    output_file = open( FLAGS.work_directory + "/" +
      "extracted_symbols_%s.txt" % file_id, "wt" )
    symbols_to_write = []
    for function_address in disassembled_functions:
      if function_address in objdump_symbols:
        symbols_to_write.append((function_address,
          objdump_symbols[function_address]))
    print("Sorting...", end='')
    symbols_to_write.sort(key=lambda a: a[1].lower())
    # symbols_to_write contains only those functions that are both in the dis-
    # assembly and that we have symbols for.
    print("Writing...", end='')
    for address, symbol in symbols_to_write:
      output_string = "%s %s %16.16lx %s false\n" % (file_id, training_file,
        address, symbol)
      output_file.write(output_string)
    print("Done")
    output_file.close()

def BuildSymbolToFileAddressMapping():
  """
  Constructs a map of symbol-string -> [ (file_id, address), ... ] so that each
  symbol is associated with all the files and addresses where it occurs.
  """
  result = defaultdict(list)
  # Iterate over all the extracted_symbols_*.txt files.
  for filename in os.listdir(FLAGS.work_directory):
    if fnmatch.fnmatch(filename, "extracted_symbols_*.txt"):
      contents = open( FLAGS.work_directory + "/" + filename, "rt" ).readlines()
      for line in contents:
        file_id, filename, address, symbol, vuln = line.split()
        result[symbol].append((file_id, address))
  return result

def SplitPercentageOfSymbolToFileAddressMapping( symbol_dict, percentage ):
  """
  Split the dictionary into two randomly chosen sub-dictionaries of a given
  percentage. This means that the given percentage of functions will be placed
  in the validation dictionary, the rest in the training dictionary.
  """
  result_validation = defaultdict(list)
  result_training = defaultdict(list)
  # Initialize the RNG to a seed to allow reproducible behavior.
  rng = random.seed(a=0xDEADBEEF)
  for key, value in symbol_dict.items():
    random_value = random.uniform(0.0, 1.0)
    if random_value < percentage:
      result_validation[key] = value
    else:
      result_training[key] = value
  return (result_validation, result_training)

def WriteAttractAndRepulseFromMap( input_map, output_directory,
  number_of_pairs=1000 ):
  """
  Writes repulse.txt and attract.txt into output_directory. Each file will
  contain number_of_pairs many pairs.

  The function is probabilistic with potentially infinite runtime, so we
  induce a brutal upper limit of number_of_pairs**3 max loop iterations.
  """
  # Construct a set of things that should be the same.
  attraction_set = set()
  symbols_as_list = list(input_map.keys())
  max_loop_iterations = number_of_pairs**3
  while len(attraction_set) != number_of_pairs and max_loop_iterations > 0:
    symbol = random.choice( symbols_as_list )
    while len( input_map[symbol] ) == 1:
      symbol = random.choice( symbols_as_list )
    element_one = random.choice( input_map[symbol] )
    element_two = element_one
    while element_one == element_two:
      element_two = random.choice( input_map[symbol] )
    ordered_pair = tuple(sorted([element_one, element_two]))
    attraction_set.add(ordered_pair)
    max_loop_iterations = max_loop_iterations - 1
  # Construct a set of things that should not be the same.
  repulsion_set = set()
  max_loop_iterations = number_of_pairs**3
  while len(repulsion_set) != number_of_pairs and max_loop_iterations > 0:
    symbol_one = random.choice( symbols_as_list )
    symbol_two = symbol_one
    while symbol_one == symbol_two:
      symbol_two = random.choice( symbols_as_list )
    element_one = random.choice( input_map[symbol_one] )
    element_two = random.choice( input_map[symbol_two] )
    ordered_pair = tuple(sorted([element_one, element_two]))
    repulsion_set.add(ordered_pair)
    max_loop_iterations = max_loop_iterations - 1
  # Write the files.
  WritePairsFile( attraction_set, output_directory + "/attract.txt" )
  WritePairsFile( repulsion_set, output_directory + "/repulse.txt" )

def WritePairsFile( set_of_pairs, output_name ):
  """
  Take a set of pairs ((file_idA, addressA), (file_idB, addressB)) and write them
  into a file as:
    file_idA:addressA file_idB:addressB
  """
  result = open(output_name, "wt")
  for pair in set_of_pairs:
    result.write("%s:%s %s:%s\n" % (pair[0][0], pair[0][1], pair[1][0],
      pair[1][1]))
  result.close()

def WriteFunctionsTxt( output_directory ):
  """
  Simply copy all the contents of all the functions_*.txt files into the output
  directory.
  """
  output_file = open( output_directory + "/functions.txt", "wt" )
  for filename in os.listdir(FLAGS.work_directory):
    if fnmatch.fnmatch(filename, "functions_????????????????.txt"):
      data = open(FLAGS.work_directory + "/" + filename, "rt").read()
      output_file.write(data)
  output_file.close()

def main(argv):
  del argv # unused.

  # Refuse to run on Python less than 3.5 (unpredictable!).

  if sys.version_info[0] < 3 or sys.version_info[1] < 5:
    print("This script requires Python version 3.5 or higher.")
    sys.exit(1)

  if FLAGS.clobber:
    shutil.rmtree(FLAGS.work_directory)
    os.mkdir(FLAGS.work_directory)

  print("Processing ELF training files to extract features...")
  ProcessTrainingFiles(FindELFTrainingFiles(), "ELF")
  print("Processing PE training files to extract features...")
  ProcessTrainingFiles(FindPETrainingFiles(), "PE")
  # We now have the extracted symbols in a set of files called
  # "extracted_symbols_*.txt"

  # Get a map that maps every symbol to the files in which it occurs.
  print("Loading all extracted symbols and grouping them...")
  symbol_to_files_and_address = BuildSymbolToFileAddressMapping()

  # First, generate the training and validation data for performance on unseen
  # functions - to test how well we generalize beyond things we have already
  # seen variants of.

  # Split off 20% of the symbols into a control map.
  print("Splitting into validation set and training set...")
  validation_map, training_map = SplitPercentageOfSymbolToFileAddressMapping(
    symbol_to_files_and_address, 0.20)

  os.mkdir(FLAGS.work_directory + "/training_data_unseen")
  os.mkdir(FLAGS.work_directory + "/validation_data_unseen")

  # Write the training set.
  print("Writing unseen training attract.txt and repulse.txt...")
  WriteAttractAndRepulseFromMap( training_map, FLAGS.work_directory + "/training_data_unseen",
    number_of_pairs=3000)

  # Write the validation set.
  print("Writing unseen validation attract.txt and repulse.txt...")
  WriteAttractAndRepulseFromMap( validation_map, FLAGS.work_directory +
    "/validation_data_unseen", number_of_pairs=500 )

  # Secondly, generate the training and validation data for performance on 'seen'
  # functions -- e.g. how well we perform if we need to spot a variant of a function
  # we have not seen before.

  os.mkdir(FLAGS.work_directory + "/training_data_seen")
  os.mkdir(FLAGS.work_directory + "/validation_data_seen")
  
  # Each function is implemented in a number of different executables - and
  # symbol_to_files_and_address gives us that mapping. In order to evaluate
  # our performance on seen functions, split 20% of each function into the
  # validation map.
  
  validation_map = defaultdict(list)
  training_map = defaultdict(list)
  for symbol, files_and_address in symbol_to_files_and_address.items():
    # Make a copy of files_and_address.
    temp = [x for x in files_and_address]
    # Shuffle the copy.
    random.shuffle(temp)
    # Take the first few elements for the validation dictionary, the rest
    # for the training dictionary
    fraction = int(float(len(temp)) * 0.3)
    if fraction > 1:
      validation_map[symbol] = temp[0:fraction]
      training_map[symbol] = temp[fraction:]
  
  # Write the training set.
  print("Writing seen training attract.txt and repulse.txt...")
  WriteAttractAndRepulseFromMap( training_map, FLAGS.work_directory + "/training_data_seen",
    number_of_pairs=1500)

  # Write the validation set.
  print("Writing seen validation attract.txt and repulse.txt...")
  WriteAttractAndRepulseFromMap( validation_map, FLAGS.work_directory +
    "/validation_data_seen", number_of_pairs=150 )

  # Write functions.txt into both directories.
  print("Writing the function.txt files...")
  WriteFunctionsTxt( FLAGS.work_directory + "/training_data_unseen" )
  WriteFunctionsTxt( FLAGS.work_directory + "/validation_data_unseen" )
  WriteFunctionsTxt( FLAGS.work_directory + "/training_data_seen" )
  WriteFunctionsTxt( FLAGS.work_directory + "/validation_data_seen" )

  print("Done, ready to run training.")

if __name__ == '__main__':
  app.run(main)

