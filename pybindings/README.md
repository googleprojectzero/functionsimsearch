## FunctionSimSearch Python Bindings


The FunctionSimSearch C++ code is built with two primary concerns: Follow the
UNIX philosophy of many different command line tools that can be combined on
the shell, and speed / efficiency.

What works well for a group of command-line-tools does not necessarily lead to
a good Python API. Furthermore, a Python API should be an API in the sense that
it should provide at least a modicum of stability for programmers programming
against it.

The Python API is split in two halves: A high-level API that is very similar
to what the command-line tools provide, and a more low-level API that is meant
for environments where someone wishes to embed FunctionSimSearch into existing
disassembly tools (e.g. without using DynInst).


High-level API: The functionsimsearch.tools module:
```
  functionsimsearch.tools.addfunctionstoindex(
  functionsimsearch.tools.addsinglefunctiontoindex(
  functionsimsearch.tools.createfunctionindex(
  functionsimsearch.tools.disassemble(
  functionsimsearch.tools.dotgraphs(
  functionsimsearch.tools.dumpfunctionindex(
  functionsimsearch.tools.dumpfunctionindexinfo(
  functionsimsearch.tools.dumpsinglefunctionfeatures(
  functionsimsearch.tools.functionfingerprints
  functionsimsearch.tools.graphhashes
  functionsimsearch.tools.growfunctionindex
  functionsimsearch.tools.trainsimhashweights


# Interface for easy use of other disassembly engines.

functionsimsearch.FlowGraphWithInstructions class:
  .add_node(address, array_of_instructions)
  .add_edge(address, address)

functionsimsearch.SimHasher class:
  .calculate_hash(functionsimsearch.FlowGraphWithInstructions)

functionsimsearch.SimHashSearchIndex class:
  .get_free_size()
  .get_used_size()
  .query_top_N(hash_a, hash_b, N)

functionsimsearch.growsearchindex function?



```




