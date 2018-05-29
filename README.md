# FunctionSimSearch

FunctionSimSearch - Example C++ code to demonstrate how to do SimHash-based
similarity search over CFGs extracted from disassemblies.

## Getting started for the lazy (using Docker)

Make sure you have Docker installed. Then do:

```bash
docker build -t functionsimsearch .
```

After the container is built, you can run all relevant commands by doing
```bash
sudo docker run -it --rm -v $(pwd):/pwd functionsimsearch COMMAND ARGUMENTS_TO_COMMAND
sudo docker run -it --rm -v $(pwd):/pwd functionsimsearch disassemble --format=ELF --input=/bin/tar
```

The last command should dump the disassembly of ./elf_file to stdout.

## Getting Started for the less lazy (build from source)

These instructions will get you a copy of the project up and running on your
local machine for development and testing purposes.

### Prerequisites

This code has a few external dependencies. The dependencies are:
  - CMake, for building
  - DynInst 9.3, built and the relevant shared libraries installed
  - (In order to build DynInst, you may need to build libdwarf from scratch with --enable-shared)
  - PE-parse, a C++ PE parser: https://github.com/trailofbits/pe-parse.git
  - PicoSHA2, a C++ SHA256 hash library: https://github.com/okdshin/PicoSHA2.git
  - SPII, a C++ library for automatic differentiation & optimization: https://github.com/PetterS/spii.git
  - JSON.hpp, a C++ library for using JSON: https://github.com/nlohmann/json.git
  - GoogleTest, a C++ unit testing library
  - GFlags, a C++ library for handling command line parameters

### Installing

You should be able to build on a Debian stretch machine by running the following
bash script in the directory where you checked out everything:

```bash
./build_dependencies.sh
```

The script does the following:

```bash
#!/bin/bash
source_dir=$(pwd)

# Install gtest and gflags. It's a bit fidgety, but works:
sudo apt-get install libgtest-dev libgflags-dev libz-dev libelf-dev cmake g++
sudo apt-get install libboost-system-dev libboost-date-time-dev libboost-thread-dev
cd /usr/src/gtest
sudo cmake CMakeLists
sudo make
sudo cp *.a /usr/lib

# Now get and the other dependencies
cd $source_dir
mkdir third_party
cd third_party

# Download Dyninst.
wget https://github.com/dyninst/dyninst/archive/v9.3.2.tar.gz
tar xvf ./v9.3.2.tar.gz
# Download PicoSHA, pe-parse, SPII and the C++ JSON library.
git clone https://github.com/okdshin/PicoSHA2.git
git clone https://github.com/trailofbits/pe-parse.git
git clone https://github.com/PetterS/spii.git
mkdir json
mkdir json/src
cd json/src
wget https://github.com/nlohmann/json/releases/download/v3.1.2/json.hpp 
cd ../..

# Build PE-Parse.
cd pe-parse
cmake ./CMakeLists.txt
make -j8
cd ..

# Build SPII.
cd spii
cmake ./CMakeLists.txt
make -j8
sudo make install
cd ..

# Build Dyninst
cd dyninst-9.3.2
cmake ./CMakeLists.txt
make -j8
sudo make install
sudo ldconfig
cd ..

# Finally build functionsimsearch tools
cd ..
make -j8
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


#### addfunctionstoindex

```
./addfunctionstoindex -format=ELF -input=/bin/tar -index=./function_search.index -minimum_function_size=5 -weights=./weights.txt
./addfunctionstoindex -format=PE -input=~/sources/mpengine/engine/mpengine.dll -index=./function_search.index -minimum_function_size=5
```

Disassemble the specified input file, find functions with more than 5 basic blocks,
calculate the SimHash for each such function and add it to the search index file.

#### addsinglefunctiontoindex

```
./addsinglefunctiontoindex -format=ELF -input=/bin/tar -index=./function_search.index -function_address=0x40deadb -weights=./weights.txt
./addsinglefunctionstoindex -format=PE -input=~/sources/mpengine/engine/mpengine.dll -index=./function_search.index -function_address=0x40deadb
```

Disassemble the specified input file, then find the function
at the specified address and at it to the search index. Incurs the full cost of
disassembling the entire executable, so use with care.


#### createfunctionindex

```
./createfunctionindex -index=./function_search.index
```

Creates a file to use for the function similarity search index. Most likely the
first command you want to run.

#### disassemble

```
./disassemble -format=ELF -input=/bin/tar
./disassemble -format=PE -input=~/sources/mpengine/engine/mpengine.dll
```

Disassemble the specified file and dump the disassembly to stdout. The input
file can either be a 32-bit/64-bit ELF, or a 32-bit PE file. Adding support
for 64-bit PE is easy and will be done soon.

#### dotgraphs

```
./dotgraphs -format=ELF -input=/bin/tar -output=/tmp/graphs
./dotgraphs -format=PE -input=~/sources/mpengine/engine/mpengine.dll -output=/tmp/graphs
```

Disassemble the specified file and write the CFGs as dot files to the specified
directory.

#### dumpfunctionindex

```
./dumpfunctionindex -index=./function_search.index
```

Dumps the content of the search index to text. The content consists of 5 text
colums:

| HashID | SimHash First Part | SimHash Second Part | Executable ID | Address |
| ------ | ------------------ | ------------------- | ------------- | ------- |
|  ...   |   ...              | ...                 | ...           | ...     |

#### dumpfunctionindexinfo

```
./dumpfunctionindexinfo -index=./function_search.index
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
./dumpsinglefunctionfeatures -format=ELF -input=/bin/tar -function_address=0x43AB900
```

Disassembles the input file, finds the relevant function, and dumps the 64-bit
IDs of the features that will be used for the SimHash calculation to stdout.
You will probably not need this command unless you experiment with the machine
learning features in the codebase.

#### evalsimhashweights

```
./evalsimhashweights -data=datadirectory -weights=./weights.txt
```

Evaluates the weight file specified on labeled data in /datadirectory. Refer
to the tutorial about weight learning for details.

#### functionfingerprints

```
./functionfingerprints -format=ELF -input=/bin/tar -minimum_function_size=5 -verbose=true
./functionfingerprints -format=PE -input=~/sources/mpengine/engine/mpengine.dll -minimum_function_size=5 -verbose=false
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
./graphhashes -format=ELF -input=/bin/tar
./graphhashes -format=PE -input=~/sources/mpengine/engine/mpengine.dll
```

Disassemble the specified file and write a hash of the cfg structure of each
disassembled function to stdout. These hashes encode **only** the graph
structure and completely ignore any mnemonics; as such they are not very useful
on small graphs.

#### growfunctionindex

```
./growfunctionindex -index=./function_search.index -size_to_grow=512
```

Expand the search index file by 512 megabytes. Index files unfortunately cannot
be dynamically resized, so when one nears being full, it is a good idea to
grow it.


#### matchfunctionsfromindex

```
./matchfunctionsfromindex -format=ELF -input=/bin/tar -index=./function_search.index -minimum_function_size=5 -max_matches=10 -minimum_percentage=0.90 -weights=./weights_to_use.txt
./matchfunctionsfromindex -format=PE -input=~/sources/mpengine/engine/mpengine.dll -index=./function_search.index -minimum_function_size=5 -max_matches=10 -minimum_percentage=.90
```

Disassemble the specified input file, and for each function with more than 5
basic blocks, retrieve the top-10 most similar functions from the search index.
Each match must be at least 90% similar.

#### trainsimhashweights

```
./trainsimhashweights -data=/tmp/datadir -train_steps=500 -weights=./trained_weights.txt
```

A command line tool to infer feature weights from examples. Uses the data in
the specified data directory, trains for 500 iterations (using LBFGS), and then
writes the resulting weights to the specified file.

## End-to-end tutorial: How to build an index of vulnerable functions to scan for

Let's assume that weights have been trained already, and placed in a file
called "trained_weights_500.txt". 

```bash
# Create a new index file.
bin/createfunctionindex --index="./trained.index"

# Grow the index to be 2 gigs in size.
bin/growfunctionindex --index="./trained.index" --size_to_grow=2048

# Add DLLs with interesting functions to the search index.
for dll in $(find -iname \*.dll); do \
  bin/addfunctionstoindex -format=PE -index=,/trained.index -weights=./trained_weights_500.txt --input $(pwd)/$dll; done

# Add ELFs with interesting functions to the search index.
(...)

```

At this point, we can start scanning a given new executable for any of the
functions in the search index.

```bash

bin/matchfunctionsfromindex -format=PE -index=./trained.index -input=../bin/libpng-1.6.26_msvc14/libpng16.dll -weights=./trained_weights_500.txt

sr/local/google/home/thomasdullien/Desktop/sources/functionsimsearch/trained_weights_500.txt 
[!] Executable id is 66d4ebee347438de
[!] Loaded search index, starting disassembly.
[!] Done disassembling, beginning search.
[!] (289/882 - 7 branching nodes) 1.000000: 66d4ebee347438de.10002360 matches 829836f67adb6dad.10002360 
[!] (289/882 - 7 branching nodes) 1.000000: 66d4ebee347438de.10002360 matches 47b1aef4056f7bcc.10002360 
[!] (289/882 - 7 branching nodes) 1.000000: 66d4ebee347438de.10002360 matches 99838a9a51e1e4f2.10002360 
[!] (289/882 - 7 branching nodes) 1.000000: 66d4ebee347438de.10002360 matches 829836f67adb6dad.10002360 
[!] (289/882 - 7 branching nodes) 1.000000: 66d4ebee347438de.10002360 matches 47b1aef4056f7bcc.10002360 
(...)
```

While this is nice and fine, all we get is the source executable ID and the
address of the function we matched to. In order to make sense of this, we need
to make sure there is a file called "./trained.index.metadata" in the same
directory as the index file.

This file should contain simple space-delimited lines of the format:
```
[16 char FileID] [FileName] [16 char function address] [function name] [true/false]
```
The last field indicates whether the function is known to be vulnerable or not.

Re-running our matching command above now yields much more useful output:

```bash
[!] Executable id is 66d4ebee347438de
[!] Loaded search index, starting disassembly.
[!] Done disassembling, beginning search.
[!] (313/882 - 7 branching nodes) 1.000000: 66d4ebee347438de.10004490 matches 829836f67adb6dad.10004490 /media/thomasdullien/storage/binaries/bin/./libpng-1.6.27_msvc17/libpng16.dll _png_colorspace_set_sRGB 
[!] (313/882 - 7 branching nodes) 1.000000: 66d4ebee347438de.10004490 matches 47b1aef4056f7bcc.10004490 /media/thomasdullien/storage/binaries/bin/./libpng-1.6.25_msvc17/libpng16.dll _png_colorspace_set_sRGB 
[!] (313/882 - 7 branching nodes) 1.000000: 66d4ebee347438de.10004490 matches 99838a9a51e1e4f2.10004490 /media/thomasdullien/storage/binaries/bin/./libpng-1.6.28_msvc17/libpng16.dll _png_colorspace_set_sRGB 
[!] (313/882 - 7 branching nodes) 1.000000: 66d4ebee347438de.10004490 matches 829836f67adb6dad.10004490 /media/thomasdullien/storage/binaries/bin/./libpng-1.6.27_msvc17/libpng16.dll _png_colorspace_set_sRGB 
[!] (313/882 - 7 branching nodes) 1.000000: 66d4ebee347438de.10004490 matches 47b1aef4056f7bcc.10004490 /media/thomasdullien/storage/binaries/bin/./libpng-1.6.25_msvc17/libpng16.dll _png_colorspace_set_sRGB 
(...)
```

Allright, that's all. Things seem to work. Enjoy!


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

### Generating input data

Input data currently consists of three text files:
1. ```functions.txt``` - a file with the functions in the training set. The format
   of this file is identical to the output of the functionfingerprints command.
2. ```attract.txt``` - a file with pairs of functions that should be considered
   the same. The format is individual text lines of the form
   ```[fileId]:[Address] [fileId]:[Address]```
3. ```repulse.txt``` - a file with the same format, but specifying functions that
   should not be considered the same.

Generating these files from the unrar executables is somewhat involved -- we need
to extract the function names from debug information, convert between different
compiler conventions on how to demangle their names, and finally match the ones
we can match by identical name. For the moment, simply use the below shell script;
run it from ./bin after having built everything.

Generate ```functions.txt, attract.txt, repulse.txt``` from the executables in question:
```bash

cd ./testdata
chmod +x ./generate_test_data.sh
./generate_test_data.sh
```

The script creates two directories: ```/tmp/train/training_data``` and
```/tmp/train/validation_data```. Both directories will contain the files described
above, all of them generated from the unrar executables. The set of functions
appearing in the training data should be disjoint from the set of functions
appearing in the validation data.

Note that the shell script is grossly inefficient. Rewriting this hack in C++
would be a great thing for a contributor to do :-).

### Running the training code

In order to launch the training itself, simply do:
```bash

bin/trainsimhashweights -data=/tmp/train/training_data/ -train_steps=500 -weights=./trained_weights_500.txt
```

This should launch the training process and nicely max out all the cores you have
in your machine. Unfortunately, there is no GPU-accelerated training yet, so odds
are you will have to wait a few hours until the training is done.

The resulting weights will be written to the specified output file.

### Running the validation code

After we have arun a training iteration, we need to check whether the training
actually did anything useful. Run the following command line to see what the
effects of the new weights are on the validation set:

```bash

bin/evalsimhashweights -data=/tmp/train/training_data/ -weights=./trained_weights_500.txt
```

Running this command will output data for 4 histograms (ideally visualized using
GNUplot), and the differences between the mean distances pre- and post training:

```bash

Attraction mean trained: 1.6483634587e+01
Attraction mean untrained: 2.3003175379e+01
Repulsion mean trained: 3.1962383977e+01
Repulsion mean untrained: 2.8451636541e+01
```

As we can see here, the average distance between two identical pairs was lowered
to 16.5 bits from 23.0 bits, and the average distance for non-identical pairs
was increased to 32 bits from 28.5 bits prior to training.

Drawing histograms from the data can be done as follows (assuming you have piped
the output to /tmp/evaldata):

```bash
cat /tmp/evaldata | grep -v [a-z] > /tmp/eval
gnuplot

gnuplot> set multiplot layout 2, 1 title "Pre- and Post-training distributions" font ",14"
multiplot> plot '/tmp/eval' index 1 with lines title "Repulsion pre-train", '/tmp/eval' index 3 with lines title "Repulsion post-train"
multiplot> plot '/tmp/eval' index 0 with lines title "Attraction pre-train", '/tmp/eval' index 2 with lines title "Attraction post-train"
multiplot> unset multiplot

```

## Compiling the same debian package with many compilers/settings

For all experiments with this codebase, it is often useful to be able to compile
a given codebase with a number of different compilers and compiler settings. This
is often complicated, though, by various Makefiles and build scripts ignoring
flags provided to them by debuild, or alternatively, by clang ignoring all but
the last -Ox argument. The following is a quick guide on how to rebuild a given
Debian package with a number of different compiler settings.

* Install https://github.com/gawen947/gcc-wrapper -- this is a simple bash
  wrapper for GCC and G++ which will replace -Ox arguments in the command line.
* Build a number of configurations for different compilers in your ~/.config/gcc-wrapper directory
* Get the source code for the package using apt-get source.

Once you have all this, you can change to the source directory and issue the
following command:

```bash

directory=$(pwd|rev|cut -d"/" -f1|rev); for config in $(ls ~/.config/gcc-wrapper/); do config=$(echo $config|rev|cut -d"/" -f1|rev); DEB_BUILD_OPTIONS="nostrip" debuild --set-envvar=GCC_PROFIL=$config -b -us -uc -j36; cd ..; mkdir $(echo $directory).$config; mv ./*.deb $(echo $directory).$config; cd $directory; done
```

This should take every compiler configuration that you have in your gcc-wrapper
directory, and rebuild the package with this compiler configuration. The results
will be placed in the corresponding subdirectory.

Once this is done, you can run the following command to gather all the .so's in
the packages, label them properly, and put them in the right directory:


```bash
# Start one level up from the source directory!
for i in $(find -iname *.deb); do config=$(echo $i | cut -d"/" -f2); mkdir temp; mkdir temp/$config; dpkg -x $i ./temp/$config; done
mkdir result_sos
for so in $(find -iname *.so); do config=$(echo $so | cut -d"/" -f3); filename=$(echo $so | rev | cut -d"/" -f1| rev); cp $so ./result_sos/$(echo $filename).$config; done
```

## Built With

* [DynInst](http://www.dyninst.org/downloads/dyninst-9.x) - Used to generate disassemblies
* [Boost](http://www.boost.org) - Boost for persistent map and set containers

## Authors

* **Thomas Dullien** - *Initial work* - [FunctionSimSearch](https://github.com/thomasdullien/functionsimsearch)

## License

This project is licensed under the Apache 2.0 License - see the [LICENSE](LICENSE.md) file for details



