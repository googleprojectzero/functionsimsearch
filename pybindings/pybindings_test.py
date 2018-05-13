import functionsimsearch

flowgraph = functionsimsearch.FlowgraphWithInstructions()

# A graph description of a simple graph with some code.

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

print(flowgraph.to_json())

hasher = functionsimsearch.SimHasher()
function_hash = hasher.calculate_hash(flowgraph)
print(function_hash)
