# FunctionSimSearch

FunctionSimSearch - Example C++ code to demonstrate how to do MinHash-based
similarity search over CFGs extracted from disassemblies.

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

### Prerequisites

This code has a few external dependencies. The dependencies are:
  - DynInst 9.3, built and the relevant shared libraries installed
  - (In order to build DynInst, you may need to build libdwarf from scratch with --enable-shared)
  - PE-parse, a C++ PE parser: https://github.com/trailofbits/pe-parse.git
  - PicoSHA2, a C++ SHA256 hash library: https://github.com/okdshin/PicoSHA2.git

### Installing

You should be able to build the code by doing the following:

1. Download, build and install DynInst 9.3
2. Get the dependencies:
```bash
mkdir third_party
cd third_party
git clone https://github.com/okdshin/PicoSHA2.git
git clone https://github.com/trailofbits/pe-parse.git
cd pe-parse
cmake ./CMakeLists
make
cd ../..
make
```
This should build the relevant executables to try.

## Running the tests

There are no tests yet. This will change eventually, most likely using gtest.

## Running the tools

At the moment, the following executables will be built:

#### disassemble

```
./disassemble ELF /bin/tar
./disassemble PE ~/sources/mpengine/engine/mpengine.dll
```

Disassemble the specified file and dump the disassembly to stdout. The input
file can either be a 32-bit/64-bit ELF, or a 32-bit PE file. Adding support
for 64-bit PE is easy and will be done soon.

#### dotgraphs

```
./dotgraphs ELF /bin/tar /tmp/graphs
./dotgraphs PE ~/sources/mpengine/engine/mpengine.dll /tmp/graphs
```

Disassemble the specified file and write the CFGs as dot files to the specified
directory.

#### graphhashes

```
./graphhashes ELF /bin/tar /tmp/graphs
./graphhashes PE ~/sources/mpengine/engine/mpengine.dll /tmp/graphs
```

Disassemble the specified file and write a hash of the CFG structure of each
disassembled function to stdout. These hashes encode **only** the graph
structure and completely ignore any mnemonic; as such they are not very useful
on small graphs.

#### createfunctionindex

```
./createfunctionindex ./function_search.index
```

Creates a new search index to store function fingerprints in. The code currently
creates 1 GB file (which should be plenty for the casual reverse engineer).
Adjust the source code if you need more / less storage.

#### addfunctionstoindex

```
./addfunctionstoindex ELF /bin/tar ./function_search.index 5
./addfunctionstoindex PE ~/sources/mpengine/engine/mpengine.dll ./function_search.index 5
```

Disassemble the specified input file, and add every function with more than 5
basic blocks to the search index file.

#### matchfunctionsfromindex

```
./matchfunctionsfromindex ELF /bin/tar ./function_search.index 5 10
./matchfunctionsfromindex PE ~/sources/mpengine/engine/mpengine.dll ./function_search.index 5 10
```

Disassemble the specified input file, and for each function with more than 5
basic blocks, retrieve the top-10 most similar functions from the search index.

## Built With

* [DynInst](http://www.dyninst.org/downloads/dyninst-9.x) - Used to generate disassemblies
* [Boost](http://www.boost.org) - Boost for persistent map and set containers

## Authors

* **Thomas Dullien** - *Initial work* - [FunctionSimSearch](https://github.com/thomasdullien/functionsimsearch)

## License

This project is licensed under the Apache 2.0 License - see the [LICENSE](LICENSE.md) file for details



