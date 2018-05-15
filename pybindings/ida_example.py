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
    results[-1].append(instructions[index])
    if (instructions[index][1] == split_mnemonic):
      results.append([])
    index = index + 1
  if len(results[-1]) == 0:
    results.pop()
  return results

def get_flowgraph_from(address):
  """
    Generates a flowgraph object that can be fed into FunctionSimSearch from a
    given address in IDA.
  """
  ida_flowgraph = idaapi.FlowChart(idaapi.get_func(here()))
  flowgraph = functionsimsearch.FlowgraphWithInstructions()
  for block in ida_flowgraph:
    # Add all the ida-flowgraph-basic blocks. We do this up-front so we can
    # more easily add edges later, and adding a node twice does not hurt.
    flowgraph.add_node(block.start_ea)

  for block in ida_flowgraph:
    instructions = [ (i, GetMnem(i), (print_operand(i, 0),
      print_operand(i, 1))) for i in Heads(block.start_ea, block.end_ea)]
    small_blocks = split_instruction_list(instructions, "call")
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

def save_function():
  search_index
  sim_hasher
  flowgraph = get_flowgraph_from(here())
  hashes = sim_hasher.calculate_hash(flowgraph)
  executable_id = long(ida_nalt.retrieve_input_file_sha256()[0:16], 16)
  address = idaapi.get_func(here()).start_ea
  search_index.add_function(hashes[0], hashes[1], executable_id, address)
  print("%lx:%lx %lx-%lx Added function to search index." % (executable_id,
    address, hashes[0], hashes[1]))

def load_function():
  search_index
  sim_hasher
  flowgraph = get_flowgraph_from(here())
  hashes = sim_hasher.calculate_hash(flowgraph)
  executable_id = long(ida_nalt.retrieve_input_file_sha256()[0:16], 16)
  address = idaapi.get_func(here()).start_ea
  results = search_index.query_top_N(hashes[0], hashes[1], 5)
  if len(results) == 0:
    print("%lx:%lx %lx-%lx No search results" %
      (executable_id, address, hashes[0], hashes[1]))
  for result in results:
    print("%lx:%lx %lx-%lx Best match is %f - %lx:%lx" %
      (executable_id, address, hashes[0], hashes[1],
      max(float(result[0]) / 128.0 - 0.5, 0.0)*2,
      result[1], result[2]))

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
  search_index
  sim_hasher
  del search_index
  del sim_hasher
except:
  hotkey_context_S = idaapi.add_hotkey("Shift-S", save_function)
  hotkey_context_L = idaapi.add_hotkey("Shift-L", load_function)
  if hotkey_context_S is None or hotkey_context_L is None:
    print("FunctionSimSearch: Failed to register hotkeys.")
    del hotkey_context_S
    del hotkey_context_L
  else:
    print("FunctionSimSearch: Hotkeys registered.")
  create_index = True
  if os.path.isfile('/tmp/example.simhash'):
    create_index = False
  search_index = functionsimsearch.SimHashSearchIndex("/tmp/example.simhash",
    create_index, 28)
  sim_hasher = functionsimsearch.SimHasher()



