## FunctionSimSearch Python Bindings

The FunctionSimSearch C++ code is built with two primary concerns: Follow the
UNIX philosophy of many different command line tools that can be combined on
the shell, and speed / efficiency.

What works well for a group of command-line-tools does not necessarily lead to
a good Python API. Furthermore, a Python API should be an API in the sense that
it should provide at least a modicum of stability for programmers programming
against it.

At the moment, the Python API is extremely rudimentary, exposing just the bare
functionality of SimHashing and nearest neighbor search.

In order to build & install the Python extension, please issue:
```
  python ./setup.py install --user
```
in the main FunctionSimSearch directory after you have built the rest of
FunctionSimSearch.

Note that the Python extensions should build even if DynInst does not build (the
extension is meant to work inside other disassemblers, and therefore does not
need DynInst).

```
# Interface for easy use of other disassembly engines.

functionsimsearch.FlowGraphWithInstructions class:
  .add_node(address)
  .add_instructions(address, (("mnem", ("op1", "op2")), ...))
  .add_edge(address, address)

functionsimsearch.SimHasher class:
  .calculate_hash(some_FlowGraphWithInstructions)

functionsimsearch.SimHashSearchIndex class:
  .query_top_N(hash_a, hash_b, N)

```

Please refer to the example IDA Plugin and the Python test code for example
usage.




