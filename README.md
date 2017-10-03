# FunctionSimSearch

FunctionSimSearch - Example C++ code to demonstrate how to do SimHash-based
similarity search over CFGs extracted from disassemblies.

## Getting Started

These instructions will get you a copy of the project up and running on your
local machine for development and testing purposes.

### Prerequisites

This code has a few external dependencies. The dependencies are:
  - DynInst 9.3, built and the relevant shared libraries installed
  - (In order to build DynInst, you may need to build libdwarf from scratch with --enable-shared)
  - PE-parse, a C++ PE parser: https://github.com/trailofbits/pe-parse.git
  - PicoSHA2, a C++ SHA256 hash library: https://github.com/okdshin/PicoSHA2.git
  - SPII, a C++ library for automatic differentiation & optimization: https://github.com/PetterS/spii.git

### Installing

You should be able to build the code by doing the following:

1. Download, build and install DynInst 9.3. This may involve building boost from
   source inside the DynInst directory tree (at least it did for me), and building
   libdwarf from scratch.
2. Get the dependencies:
```bash
mkdir third_party
cd third_party
git clone https://github.com/okdshin/PicoSHA2.git
git clone https://github.com/trailofbits/pe-parse.git
git clone https://github.com/PetterS/spii.git
cd pe-parse
cmake ./CMakeLists
make
cd ..
cd spii
cmake ./CMakeLists
make
cd ../..
make
```
This should build the relevant executables to try. On Debian stretch and later,
you may have to add '-fPIC' into the pe-parse CMakeLists.txt to make sure your
generated library supports being relocated.

## Running the tests

You can run the tests by doing:
```
cd tests
./runtests
./slowtests
```
Note that the tests use relative directories, assuming that you actually changed
your directory, so running
```
tests/runtests
```
will not work.

Also be aware that a fair number of the tests are pretty expensive to run, and
expect the full testsuite to eat all your CPU for a few minutes; the suite of
slow tests may keep your computer busy for hours.

## Running the tools

At the moment, the following executables will be built (in alphabetical order):

#### addsinglefunctiontoindex

```
./addsinglefunctiontoindex ELF /bin/tar ./function_search.index 40deadb
./addsinglefunctionstoindex PE ~/sources/mpengine/engine/mpengine.dll ./function_search.index 40deadb
```

Disassemble the specified input file, disassemble the file, then find the function
at the specified address and at it to the search index. Incurs the full cost of
disassembling the entire executable, so use with care.

#### addfunctionstoindex

```
./addfunctionstoindex ELF /bin/tar ./function_search.index 5
./addfunctionstoindex PE ~/sources/mpengine/engine/mpengine.dll ./function_search.index 5
```

Disassemble the specified input file, find functions with more than 5 basic blocks,
calculate the SimHash for each such function and add it to the search index file.

#### createfunctionindex

```
./createfunctionindex ./function_search.index
```

Creates a file to use for the function similarity search index. Most likely the
first command you want to run.

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

#### dumpfunctionindex

```
./dumpfunctionindex ./function_search.index
```

Dumps the content of the search index to text. The content consists of 5 text
colums:

| HashID | SimHash First Part | SimHash Second Part | Executable ID | Address |
| ------ | ------------------ | ------------------- | ------------- | ------- |
|  ...   |   ...              | ...                 | ...           | ...     |

#### dumpfunctionindexinfo

```
./dumpfunctionindexinfo ./function_search.index
```

Prints information about the index file - how much space is used, how much space
is left, how many functions are indexed etc.

Example output:
```
[!] FileSize: 537919488 bytes, FreeSpace: 36678432 bytes
[!] Indexed 270065 functions, total index has 7561820 elements
```

#### dumpsinglefunctionfeatures

```
./dumpsinglefunctionfeatures ELF /bin/tar 0x43AB900
```

Disassembles the input file, finds the relevant function, and dumps the 64-bit
IDs of the features that will be used for the SimHash calculation to stdout.
You will probably not need this command unless you experiment with the machine
learning features in the codebase.

#### evalsimhashweights

```
./evalsimhashweights /datadirectory /datadirectory/weights.txt
```

Evaluates the weight file specified on labeled data in /datadirectory. Refer
to the tutorial about weight learning for details.

#### functionfingerprints

```
./functionfingerprints ELF /bin/tar 5 true
./functionfingerprints PE ~/sources/mpengine/engine/mpengine.dll 5 false
```

Disassembles the target file and all functions therein. If the last argument
(verbose) is set to "false", this tool will simply dump the SimHash hashes
of the functions in the specified executable to stdout, in the format:

```
FileID:Address SimHashA SimHashB
```

If verbose is set to "true", the tool will dump the feature IDs of the
features that enter the SimHash calculation, so the output will look like:

```
FileID:Address Feature1 Feature2 ... FeatureN
FileID:Address FeatureM ... FeatureK
```

The features themselves are 128-bit hashes. The output of the tool in verbose
mode is used to create training data for the machine learning components.


#### graphhashes

```
./graphhashes elf /bin/tar /tmp/graphs
./graphhashes pe ~/sources/mpengine/engine/mpengine.dll /tmp/graphs
```

disassemble the specified file and write a hash of the cfg structure of each
disassembled function to stdout. these hashes encode **only** the graph
structure and completely ignore any mnemonics; as such they are not very useful
on small graphs.

#### growfunctionindex

```
./growfunctionindex ./function_search.index 512
```

Expand the search index file by 512 megabytes. Index files unfortunately cannot
be dynamically resized, so when one nears being full, it is a good idea to
grow it.


#### matchfunctionsfromindex

```
./matchfunctionsfromindex ELF /bin/tar ./function_search.index 5 10 .90
./matchfunctionsfromindex PE ~/sources/mpengine/engine/mpengine.dll ./function_search.index 5 10 .90
```

Disassemble the specified input file, and for each function with more than 5
basic blocks, retrieve the top-10 most similar functions from the search index.
Each match must be at least 90% similar.

#### trainsimhashweights

```
./trainsimhashweights
```

Experimental tool to help learn feature weights from examples. Do not use yet.


## End-to-end tutorial: How to build an index of vulnerable functions to scan for

## End-to-end tutorial: Training weights prior to building an index of vulnerable functions

How does one train weights prior to building a search index? We need a couple
of ingredients:

* Labeled data to train weights on. These are pairs of functions that are known
  to be the same, and pairs of functions that are known *not* to be the same.
* Labeled data to evaluate the weights on. These are the same as above, but are
  kept as a held-out dataset to see if the learning process actually had any
  predictive value.

In order to generate labelled datasets, the usual process involves compiling the
same codebase with many different compilers and compiler settings. In an ideal
world, we would have something like Compiler Explorer; in the meantime, people
will have to build things themselves.

The git repository contains builds of unrar.5.5.3 using GCC 4.8 and VS2015 to
get started in the ./testdata directory.

### Generating input data

Input data currently consists of three text files:
1. ```functions.txt``` - a file with the functions in the training set. The format
   of this file is identical to the output of the functionfingerprints command.
2. ```attract.txt``` - a file with pairs of functions that should be considered
   the same. The format is individual text lines of the form
   ```[fileId]:[Address] [fileId]:[Address]```
3. ```repulse.txt``` - a file with the same format, but specifying functions that
   should not be considered the same.

So let's walk through generating these files from the unrar executables:

First, generate a ```functions.txt``` from the executables in question:
```
# Process the GCC-compiled files.
for i in $(ls -al ../testdata/unrar.5.5.3.builds/unrar.x??.O?); do ./functionfingerprints ELF $i 5 true >> ./functions.txt; done
# Process the VS2015-compiled files.
for i in $(find ../testdata/unrar.5.5.3.builds/VS2015/ -iname \*.exe); do ./functionfingerprints PE $i 5 true >> ./functions.txt; done
```



## Built With

* [DynInst](http://www.dyninst.org/downloads/dyninst-9.x) - Used to generate disassemblies
* [Boost](http://www.boost.org) - Boost for persistent map and set containers

## Authors

* **Thomas Dullien** - *Initial work* - [FunctionSimSearch](https://github.com/thomasdullien/functionsimsearch)

## License

This project is licensed under the Apache 2.0 License - see the [LICENSE](LICENSE.md) file for details



