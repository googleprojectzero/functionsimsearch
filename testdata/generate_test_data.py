#!/usr/bin/python3

# A Python script to generate test data given ELF or PE files including their
# relevant debug info.
#
# This Python script is meant to replace the horrible bash script that is
# currently responsible for doing this.

import subprocess, os, fnmatch, codecs, random, shutil, multiprocessing
from subprocess import Popen, PIPE, STDOUT
from operator import itemgetter
from collections import defaultdict

# Make sure you have a trailing slash.
work_directory="/media/thomasdullien/storage/functionsimsearch/train_data/"

#=============================================================================
# A number of boolean variables to allow skipping of certain processing steps.

# Generate the fingerprint hashes.
generate_fingerprints = True

# Generate full disassemblies in JSON, too. This is not necessary for any of
# the tools in this repository, but may be useful if you wish to experiment
# with disassembly data in other machine-learning contexts.
generate_json_data = True

# Disable the use of mnemonic data.
disable_mnemonic = False
#=============================================================================

if generate_fingerprints and generate_json_data:
  shutil.rmtree(work_directory)
  os.mkdir(work_directory)

def FindELFTrainingFiles():
  """ Returns the list of ELF files that should be used for training. These
  ELF files need to contain objdump-able debug information.

  """
  dirnames_and_masks = [ ('./unrar.5.5.3.builds', 'unrar.x??.O?'),
    ('./ffmpeg.3.2.10.sos', '*lib*.so.*') ]

  filenames = []
  for dirname, mask in dirnames_and_masks:
    if not os.path.exists(dirname):
      continue
    filenames = filenames + [
      dirname + "/" + filename for filename in os.listdir(dirname)
      if fnmatch.fnmatch(filename, mask) ]
  return filenames

def FindPETrainingFiles():
  """ Returns the list of PE files that should be used for training. These
  PE files need to have associated text files (with suffix .debugdump) that
  contains the output of dia2dump in the same directory. """
  pe_list = [
    "./unrar.5.5.3.builds/VS2015/unrar32/Release/UnRAR.exe",
    "./unrar.5.5.3.builds/VS2015/unrar32/Debug/UnRAR.exe",
    "./unrar.5.5.3.builds/VS2015/unrar64/Release/UnRAR.exe",
    "./unrar.5.5.3.builds/VS2015/unrar64/Debug/UnRAR.exe",
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_standard/avutil.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_standard/swresample-2.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_standard/avfilter.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_standard/swresample.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_standard/swscale.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_standard/avfilter-6.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_standard/avdevice.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_standard/avutil-55.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_standard/avdevice-57.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_standard/avformat-57.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_standard/avcodec.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_standard/swscale-4.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_standard/avcodec-57.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_standard/avformat.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_size_optimized/avutil.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_size_optimized/swresample-2.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_size_optimized/avfilter.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_size_optimized/swresample.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_size_optimized/swscale.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_size_optimized/avfilter-6.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_size_optimized/avdevice.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_size_optimized/avutil-55.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_size_optimized/avdevice-57.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_size_optimized/avformat-57.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_size_optimized/avcodec.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_size_optimized/swscale-4.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_size_optimized/avcodec-57.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2017_size_optimized/avformat.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_size_optimized/avutil.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_size_optimized/swresample-2.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_size_optimized/avfilter.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_size_optimized/swresample.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_size_optimized/swscale.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_size_optimized/avfilter-6.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_size_optimized/avdevice.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_size_optimized/avutil-55.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_size_optimized/avdevice-57.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_size_optimized/avformat-57.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_size_optimized/avcodec.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_size_optimized/swscale-4.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_size_optimized/avcodec-57.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_size_optimized/avformat.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_standard/avutil.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_standard/swresample-2.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_standard/avfilter.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_standard/swresample.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_standard/swscale.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_standard/avfilter-6.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_standard/avdevice.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_standard/avutil-55.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_standard/avdevice-57.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_standard/avformat-57.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_standard/avcodec.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_standard/swscale-4.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_standard/avcodec-57.dll',
    './ffmpeg.3.2.10.visual_studio_builds/vs_2015_standard/avformat.dll']
  pe_list = [ filename for filename in pe_list if os.path.exists(filename) ]
  return pe_list

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
    inputdata = open( work_directory + "/" + "functions_%s.txt" % training_file_id,
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
  directory_name = work_directory + "/" + "json_%s" % file_id
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
  if disable_mnemonic:
    mnemonic = "--disable_instructions=true"
  else:
    mnemonic = "--disable_instructions=false"
 
  write_fingerprints = open(
    work_directory + "/" + "functions_%s.txt" % file_id, "wt")
 
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
  if generate_fingerprints:
    print("Running functionfingerprints on all files.")
    pool.map(RunFunctionFingerprints, argument_tuples)
  if generate_json_data:
    print("Running dotgraphs on all files.")
    pool.map(RunJSONDotgraphs, argument_tuples)

  for training_file in training_files:
    sha256sum = subprocess.check_output(["sha256sum", training_file]).split()[0]
    file_id = sha256sum[0:16].decode("utf-8")
    # Run objdump
    print("Running objdump on %s... " % training_file, end='')
    objdump_symbols = ObtainFunctionSymbols(training_file, file_format)
    # Get the functions that our disassembly could find.
    print("Getting disassembled functions. ", end='')
    disassembled_functions = ObtainDisassembledFunctions(file_id)
    # Write the symbols for the functions that the disassembly found.
    print("Opening and writing extracted_symbols_%s.txt. " % file_id, end='')
    output_file = open( work_directory + "/" +
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
  for filename in os.listdir(work_directory):
    if fnmatch.fnmatch(filename, "extracted_symbols_*.txt"):
      contents = open( work_directory + "/" + filename, "rt" ).readlines()
      for line in contents:
        file_id, filename, address, symbol, vuln = line.split()
        result[symbol].append((file_id, address))
  return result

def SplitPercentageOfSymbolToFileAddressMapping( symbol_dict, percentage ):
  """
  Split the dictionary into two randomly chosen sub-dictionaries of a given
  percentage.
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
  """
  # Construct a set of things that should be the same.
  attraction_set = set()
  symbols_as_list = list(input_map.keys())
  while len(attraction_set) != number_of_pairs:
    symbol = random.choice( symbols_as_list )
    while len( input_map[symbol] ) == 1:
      symbol = random.choice( symbols_as_list )
    element_one = random.choice( input_map[symbol] )
    element_two = element_one
    while element_one == element_two:
      element_two = random.choice( input_map[symbol] )
    ordered_pair = tuple(sorted([element_one, element_two]))
    attraction_set.add(ordered_pair)
  # Construct a set of things that should not be the same.
  repulsion_set = set()
  while len(repulsion_set) != number_of_pairs:
    symbol_one = random.choice( symbols_as_list )
    symbol_two = symbol_one
    while symbol_one == symbol_two:
      symbol_two = random.choice( symbols_as_list )
    element_one = random.choice( input_map[symbol_one] )
    element_two = random.choice( input_map[symbol_two] )
    ordered_pair = tuple(sorted([element_one, element_two]))
    repulsion_set.add(ordered_pair)
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
      pair[0][0]))
  result.close()

def WriteFunctionsTxt( output_directory ):
  """
  Simply copy all the contents of all the functions_*.txt files into the output
  directory.
  """
  output_file = open( output_directory + "/functions.txt", "wt" )
  for filename in os.listdir(work_directory):
    if fnmatch.fnmatch(filename, "functions_????????????????.txt"):
      data = open(work_directory + "/" + filename, "rt").read()
      output_file.write(data)
  output_file.close()

print("Processing ELF training files to extract features...")
ProcessTrainingFiles(FindELFTrainingFiles(), "ELF")
print("Processing PE training files to extract features...")
ProcessTrainingFiles(FindPETrainingFiles(), "PE")

# We now have the extracted symbols in a set of files called
# "extracted_symbols_*.txt"

# Get a map that maps every symbol to the files in which it occurs.
print("Loading all extracted symbols and grouping them...")
symbol_to_files_and_names = BuildSymbolToFileAddressMapping()

# Split off 10% of the symbols into a control map.
print("Splitting into validation set and training set...")
validation_map, training_map = SplitPercentageOfSymbolToFileAddressMapping(
  symbol_to_files_and_names, 0.20)

os.mkdir(work_directory + "/training_data")
os.mkdir(work_directory + "/validation_data")

# Write the training set.
print("Writing training attract.txt and repulse.txt...")
WriteAttractAndRepulseFromMap( training_map, work_directory + "/training_data",
  number_of_pairs=3000)

# Write the validation set.
print("Writing validation attract.txt and repulse.txt...")
WriteAttractAndRepulseFromMap( validation_map, work_directory +
  "/validation_data", number_of_pairs=500 )

# Write functions.txt into both directories.
print("Writing the function.txt files...")
WriteFunctionsTxt( work_directory + "/training_data" )
WriteFunctionsTxt( work_directory + "/validation_data" )

print("Done, ready to run training.")


