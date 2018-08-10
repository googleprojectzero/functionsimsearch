"""
  A simple example script demonstrating the use of FunctionSimSearch via the IDA
  Python API.

  Registers two hotkeys:
     Shift-S -- hash the current function, print the hash, and add it to the
                search index in /tmp/example.simhash
     Shift-L -- hash the current function, search for the best hits in the data-
                base, and display the best guesses along with their similarity
                score.
"""
import functionsimsearch
import idaapi
import base64
import os

def parse_function_meta_data(filename):
  # Returns a dictionary mapping (id, address) tuples to 
  # (filename, functionname) tuples.
  result = {}
  for line in file(filename, "rt").readlines():
    tokens = line.split()
    exe_id = long(tokens[0], 16)
    address = long(tokens[2], 16)
    function_name = base64.decodestring(tokens[3])
    file_name = tokens[1]
    result[(exe_id, address)] = (file_name, function_name)
  return result

def split_instruction_list(instructions, split_mnemonic):
  """
    Takes a sequence of instructions of the form:
     [ (address, "mnem", ("op1", ..., "opN")), ... ]
    and returns multiple lists of such instructions, split after each
    instruction where "mnem" is equal to split_mnemonic.

    Can be re-used for other disassemblers that do not split basic blocks after
    a CALL.
  """
  results = []
  index = 0
  results.append([])
  while index < len(instructions):
    # Appends the current instruction under consideration to the last list in the
    # list-of-lists 'results'.
    results[-1].append(instructions[index])
    # Checks if the right mnemonic to 'split' on is encountered.
    if (instructions[index][1] == split_mnemonic):
      # Time to split. Simply appends an empty list to 'results'.
      results.append([])
    index = index + 1
  # It is possible to have appended an empty list if the instruction-to-split-on
  # was the last instruction of the block. Remove it if this happens.
  if len(results[-1]) == 0:
    results.pop()
  return results

def get_flowgraph_from(address):
  """
    Generates a flowgraph object that can be fed into FunctionSimSearch from a
    given address in IDA.
  """
  call_instruction_string
  if not address:
    address = here()
  ida_flowgraph = idaapi.FlowChart(idaapi.get_func(address))
  flowgraph = functionsimsearch.FlowgraphWithInstructions()
  for block in ida_flowgraph:
    # Add all the ida-flowgraph-basic blocks. We do this up-front so we can
    # more easily add edges later, and adding a node twice does not hurt.
    flowgraph.add_node(block.start_ea)

  for block in ida_flowgraph:
    instructions = [ (i, GetMnem(i), (print_operand(i, 0),
      print_operand(i, 1))) for i in Heads(block.start_ea, block.end_ea)]
    small_blocks = split_instruction_list(instructions, call_instruction_string)
    for small_block in small_blocks:
      flowgraph.add_node(small_block[0][0])
      small_block_instructions = tuple(instruction[1:] for instruction 
        in small_block)
      flowgraph.add_instructions(small_block[0][0], small_block_instructions)
    for index in range(len(small_blocks)-1):
      flowgraph.add_edge(small_blocks[index][0][0], small_blocks[index+1][0][0])
    for successor_block in block.succs():
      flowgraph.add_edge(small_blocks[-1][0][0], successor_block.start_ea)
  return flowgraph

def export_idb_as_json(output_prefix):
  """ Writes the entire IDB into a big JSON file and create a separate symbol
  file as well. """
  output_file = file(output_prefix + ".json", "wt")
  output_file.write("{ \"flowgraphs\" : [ ")
  executable_id = long(ida_nalt.retrieve_input_file_sha256()[0:16], 16)
  path = ida_nalt.get_input_file_path()
  function_addresses = [ x for x in Functions(MinEA(), MaxEA()) ]
  for function in function_addresses:
    flowgraph = get_flowgraph_from(address=function)
    json = flowgraph.to_json()
    print("Writing flowgraph for function %lx (%d bytes)..." % (function, 
      len(json)))
    output_file.write(json)
    if function != function_addresses[-1]:
      output_file.write(",\n")
  output_file.write("]}")
  output_file.close()
  print("Done.")

def save_all_functions():
  search_index
  sim_hasher
  for function in Functions(MinEA(), MaxEA()):
    save_function(function)

def print_hash():
  sim_hasher
  flowgraph = get_flowgraph_from(here())
  hashes = sim_hasher.calculate_hash(flowgraph)
  print("Hash for flowgraph at %lx is %lx %lx" % (here(), hashes[0], hashes[1]))

def save_function(function_address=None):
  search_index
  sim_hasher
  if function_address == None:
    function_address = here()
  flowgraph = get_flowgraph_from(function_address)
  hashes = sim_hasher.calculate_hash(flowgraph)
  executable_id = long(ida_nalt.retrieve_input_file_sha256()[0:16], 16)
  address = idaapi.get_func(here()).start_ea
  search_index.add_function(hashes[0], hashes[1], executable_id, address)
  print("%lx:%lx %lx-%lx Added function to search index." % (executable_id,
    address, hashes[0], hashes[1]))

def load_function():
  search_index
  sim_hasher
  meta_data
  flowgraph = get_flowgraph_from(here())
  hashes = sim_hasher.calculate_hash(flowgraph)
  executable_id = long(ida_nalt.retrieve_input_file_sha256()[0:16], 16)
  address = long(idaapi.get_func(here()).start_ea)
  results = search_index.query_top_N(hashes[0], hashes[1], 5)
  if len(results) == 0:
    print("%lx:%lx %lx-%lx No search results" %
      (executable_id, address, hashes[0], hashes[1]))
  for result in results:
    if not meta_data.has_key((result[1], result[2])):
      print("%lx:%lx %lx-%lx Best match is %f - %lx:%lx" %
        (executable_id, address, hashes[0], hashes[1],
        #max(float(result[0]) / 128.0 - 0.5, 0.0)*2,
        result[0],
        result[1], result[2]))
    else:
      filename, functionname = meta_data[(result[1], result[2])]
      functionname = functionname.replace('\n', '')
      functionname = functionname.replace('\r', '')
      print("%lx:%lx %lx-%lx Best match is %f - %lx:%lx %s '%s'" %
        (executable_id, address, hashes[0], hashes[1],
        #max(float(result[0]) / 128.0 - 0.5, 0.0)*2,
        result[0],
        result[1], result[2], filename, functionname))
  print("--------------------------------------")


import ida_idp
processor_to_call_instructions = { "arm" : "BLX", "pc" : "call" }
call_instruction_string = processor_to_call_instructions[ida_idp.get_idp_name()]

try:
  hotkey_context_S
  if idaapi.del_hotkey(hotkey_context_S):
    print("FunctionSimSearch: Hotkey S unregistered.")
    del hotkey_context_S
  else:
    print("FunctionSimSearch: Failed to unregister hotkey S.")
  hotkey_context_L
  if idaapi.del_hotkey(hotkey_context_L):
    print("FunctionSimSearch: Hotkey L unregistered.")
    del hotkey_context_L
  else:
    print("FunctionSimSearch: Failed to unregister hotkey L.")
  hotkey_context_H
  if idaapi.del_hotkey(hotkey_context_H):
    print("FunctionSimSearch: Hotkey H unregistered.")
    del hotkey_context_H
  else:
    print("FunctionSimSearch: Failed to unregister hotkey H.")
  search_index
  sim_hasher
  del search_index
  del sim_hasher
except:
  # Ask the user for a directory. The code expects the directory to contain
  # a search index ("simhash.index"), a metadata file ("simhash.meta"), and
  # optionally a feature weights file ("simhash.weights").
  data_directory = AskStr("/var/tmp/",
    "Please enter a data directory. If no index is found, it will be created.")
  while not os.path.exists(data_directory):
    data_directory = AskStr("/var/tmp/", 
      "Please enter an EXISTING data directory.")

  index_file = os.path.join(data_directory, "simhash.index")
  metadata_file = os.path.join(data_directory, "simhash.meta")
  weights_file = os.path.join(data_directory, "simhash.weights")

  # Register the hotkeys for the plugin.
  hotkey_context_S = idaapi.add_hotkey("Shift-S", save_function)
  hotkey_context_L = idaapi.add_hotkey("Shift-L", load_function)
  hotkey_context_H = idaapi.add_hotkey("Shift-H", print_hash)
  if hotkey_context_S is None or hotkey_context_L is None:
    print("FunctionSimSearch: Failed to register hotkeys.")
    del hotkey_context_S
    del hotkey_context_L
    del hotkey_context_H
  else:
    print("FunctionSimSearch: Hotkeys registered.")

  create_index = True
  if os.path.isfile(index_file):
    create_index = False
  if os.path.isfile(metadata_file):
    print("Parsing meta_data from file %s" % metadata_file)
    meta_data = parse_function_meta_data(metadata_file)
    print("Parsed meta_data.")
    for i in meta_data.keys()[0:10]:
      print("%lx:%lx" %i )
  else:
    meta_data = {}
  print("Calling functionsimsearch.SimHashSearchIndex(\"%s\", %s, 28)" % (
    index_file, create_index))
  try:
    search_index = functionsimsearch.SimHashSearchIndex(index_file,
      create_index, 28)
    if os.path.isfile(weights_file):
      print("Calling functionsimsearch.SimHasher(\"%s\")" % weights_file)
      sim_hasher = functionsimsearch.SimHasher(weights_file)
    else:
      sim_hasher = functionsimsearch.SimHasher()
  except:
    print("Failure to create/open the search index!")



