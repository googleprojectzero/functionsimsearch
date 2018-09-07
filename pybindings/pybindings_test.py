import functionsimsearch, unittest

def popcount(x):
  return bin(x).count('1')

class TestFunctionSimSearch(unittest.TestCase):
  def test_construction(self):
    flowgraph = functionsimsearch.FlowgraphWithInstructions()
    # Create an example CFG.
    nodedata = [
      (0x5A5F6179, (("mov", ()), ("shr", ()), ("xor", ()), ("jz", ()))),
      (0x5A5F6187, (("mov", ()), ("and", ()), ("cmp", ()), ("jz", ()))),
      (0x5A5F6195, (("mov", ()), ("jmp", ()))),
      (0x5A5F6184, (("mov", ()), ("ret", ()))),
      (0x5A5F6182, (("xor", ()),)) ]

    edgedata = [
      (0x5A5F6179, 0x5A5F6187),
      (0x5A5F6179, 0x5A5F6182),
      (0x5A5F6182, 0x5A5F6184),
      (0x5A5F6187, 0x5A5F6184),
      (0x5A5F6187, 0x5A5F5195)]

    for n in nodedata:
      flowgraph.add_node(n[0])
    for e in edgedata:
      flowgraph.add_edge(e[0], e[1])
    for n in nodedata:
      flowgraph.add_instructions(n[0], n[1])

    # Now calculate the similarity hash of the graph.
    hasher = functionsimsearch.SimHasher()
    function_hash = hasher.calculate_hash(flowgraph)

    # Make a minor change to the graph (adding an extra node (5 node graph becomes
    # a 6-node graph).
    flowgraph.add_node(0)
    flowgraph.add_edge(0, nodedata[0][0])

    # Hash the changed version.
    function_hash2 = hasher.calculate_hash(flowgraph)

    # Calculate the distance between the two hashes - simply hamming distance between
    # bit vectors:
    distance = popcount(function_hash[0]^function_hash2[0]) +\
      popcount(function_hash[1]^function_hash2[1])

    self.assertTrue((1.0 - (distance/128.0) > 0.7))

  def test_json(self):
    """ Test JSON decoding of graphs """
    jsonstring = """{"edges":[{"destination":1518838580,"source":1518838565},{"destination":1518838572,"source":1518838565},{"destination":1518838578,"source":1518838572},{"destination":1518838574,"source":1518838572},{"destination":1518838580,"source":1518838574},{"destination":1518838578,"source":1518838574},{"destination":1518838580,"source":1518838578}],"name":"CFG","nodes":[{"address":1518838565,"instructions":[{"mnemonic":"xor","operands":["EAX","EAX"]},{"mnemonic":"cmp","operands":["[ECX + 4]","EAX"]},{"mnemonic":"jnle","operands":["5a87a334"]}]},{"address":1518838572,"instructions":[{"mnemonic":"jl","operands":["5a87a332"]}]},{"address":1518838574,"instructions":[{"mnemonic":"cmp","operands":["[ECX]","EAX"]},{"mnemonic":"jnb","operands":["5a87a334"]}]},{"address":1518838578,"instructions":[{"mnemonic":"mov","operands":["AL","1"]}]},{"address":1518838580,"instructions":[{"mnemonic":"ret near","operands":["[ESP]"]}]}]}"""

    fg = functionsimsearch.FlowgraphWithInstructions()
    fg.from_json(jsonstring)
    hasher = functionsimsearch.SimHasher()
    function_hash = hasher.calculate_hash(fg)
    self.assertTrue(function_hash[0] == 0xa7ef296fa5dea3ee)

  def test_add_and_search(self):
    """ Test adding hashes to the searchindex and searching for the nearest
    neighbor """
    import os.path
    create_index = True
    if os.path.isfile('/tmp/index.index'):
      create_index = False
    searchindex = functionsimsearch.SimHashSearchIndex("/tmp/index.index", 
      create_index, 28)

    hashA = 0xDEADBEEFDEADBEEF
    hashB = 0x0BADCAFEC0FEC0FE

    if create_index:
      searchindex.add_function(hashA, hashB, 0x1D, 0x400000)
      searchindex.add_function(hashA^3, hashB, 0x1D, 0x400001)
      searchindex.add_function(hashA^7, hashB, 0x1D, 0x400002)
      searchindex.add_function(hashA^15, hashB, 0x1D, 0x400003)
      searchindex.add_function(hashA^31, hashB^3, 0x1D, 0x400004)
      searchindex.add_function(hashA^31, hashB^7, 0x1D, 0x400005)
      searchindex.add_function(hashA^31, hashB^15, 0x1D, 0x400006)
      searchindex.add_function(hashA^31, hashB^31, 0x1D, 0x400007)
      searchindex.add_function(hashA^63, hashB^31, 0x1D, 0x400008)

    foo = searchindex.query_top_N(0xDEADBEEFDEADBEEF, 0x0BADCAFEC0FEC0FE, 9)
    self.assertTrue(foo[0][0] == 128)
    self.assertTrue(foo[0][2] == 0x400000)
    self.assertTrue(foo[1][0] == 126)

    odds = searchindex.odds_of_random_hit(110)
    odds = searchindex.odds_of_random_hit(110.0)
    print(odds)

  def test_hasher_with_weights(self):
    """ Tests whether the loading of a weights file works. """
    jsonstring = """{"edges":[{"destination":1518838580,"source":1518838565},{"destination":1518838572,"source":1518838565},{"destination":1518838578,"source":1518838572},{"destination":1518838574,"source":1518838572},{"destination":1518838580,"source":1518838574},{"destination":1518838578,"source":1518838574},{"destination":1518838580,"source":1518838578}],"name":"CFG","nodes":[{"address":1518838565,"instructions":[{"mnemonic":"xor","operands":["EAX","EAX"]},{"mnemonic":"cmp","operands":["[ECX + 4]","EAX"]},{"mnemonic":"jnle","operands":["5a87a334"]}]},{"address":1518838572,"instructions":[{"mnemonic":"jl","operands":["5a87a332"]}]},{"address":1518838574,"instructions":[{"mnemonic":"cmp","operands":["[ECX]","EAX"]},{"mnemonic":"jnb","operands":["5a87a334"]}]},{"address":1518838578,"instructions":[{"mnemonic":"mov","operands":["AL","1"]}]},{"address":1518838580,"instructions":[{"mnemonic":"ret near","operands":["[ESP]"]}]}]}"""

    fg = functionsimsearch.FlowgraphWithInstructions()
    fg.from_json(jsonstring)
    hasher = functionsimsearch.SimHasher("../testdata/weights.txt")
    function_hash = hasher.calculate_hash(fg)
    self.assertTrue(function_hash[0] == 0xa6ef292a658e83ee)


if __name__ == '__main__':
  unittest.main()


