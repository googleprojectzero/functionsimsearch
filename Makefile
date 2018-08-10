CPP = g++
CPPFLAGS += -ggdb -O3 -std=c++17 -fPIE
LIBDIR = -L./third_party/pe-parse/pe-parser-library -L./third_party/libdwarf/libdwarf
INCLUDEDIR = -Ithird_party/spii/include -I./ -Ithird_party/spii/thirdparty/Eigen
LIBS = -lparseAPI -linstructionAPI -lsymtabAPI -lsymLite -ldynDwarf -ldynElf \
       -lcommon -lelf -ldwarf -lpthread -lpe-parser-library -lspii -lgflags

OBJ = build/util.o build/util_with_dyninst.o build/disassembly.o \
      build/pecodesource.o build/flowgraph.o \
      build/flowgraphwithinstructions.o \
      build/buffertokeniterator.o \
      build/flowgraphutil.o build/flowgraphutil_dyninst.o \
      build/functionsimhash.o \
      build/functionsimhashfeaturedump.o \
      build/simhashsearchindex.o build/bitpermutation.o \
      build/threadtimer.o build/functionmetadata.o \
      build/mappedtextfile.o \
      build/simhashtrainer.o build/sgdsolver.o build/dyninstfeaturegenerator.o \
      build/trainingdata.o build/cppsplitter.o


ALL = bin/disassemble bin/dotgraphs bin/graphhashes bin/addfunctionstoindex \
      bin/addsinglefunctiontoindex \
      bin/createfunctionindex bin/functionfingerprints \
      bin/matchfunctionsfromindex bin/dumpfunctionindexinfo \
      bin/growfunctionindex bin/dumpfunctionindex \
      bin/trainsimhashweights bin/dumpsinglefunctionfeatures \
      bin/evalsimhashweights bin/stemsymbol bin/visualizeflowgraphs \
      bin/queryindexforhash

TESTS = build/bitpermutation_test.o \
        build/simhashsearchindex_test.o \
        build/flowgraphwithinstructions_test.o \
        build/disassembly_test.o \
        build/testutil.o \
        build/functionsimhash_test.o \
        build/buffertokeniterator_test.o build/mappedtextfile_test.o \
        build/cppsplitter_test.o

SLOWTESTS = build/simhashtrainer_test.o build/testutil.o build/sgdsolver_test.o

DIRECTORIES = directory/build directory/bin directory/tests directory/profile

TEST = tests/runtests

SLOWTEST = tests/slowtests

VPATH = disassembly:learning:searchbackend:tools:util:pybindings

directory/%:
	mkdir -p $(@F)

build/%.o: %.cpp $(DIRECTORIES)
	$(CPP) $(INCLUDEDIR) -c -o $@ $< $(CPPFLAGS)

all: $(ALL) $(TESTS) $(SLOWTESTS) $(TEST) $(SLOWTEST)

tests/runtests: $(TESTS) tests

tests/slowtests: $(SLOWTESTS) slowtests

slowtests: $(ALL) $(OBJ) $(SLOWTESTS)
	$(CPP) $(CPPFLAGS) -o tests/slowtests tools/runtests.cpp $(OBJ) $(SLOWTESTS) \
		$(LIBDIR) $(LIBS) -lgtest

tests: $(ALL) $(OBJ) $(TESTS)
	$(CPP) $(CPPFLAGS) -o tests/runtests tools/runtests.cpp $(OBJ) $(TESTS) $(LIBDIR) \
		$(LIBS) -lgtest

bin/%: $(OBJ)
	$(CPP) $(INCLUDEDIR) $(CPPFLAGS) -o $@ tools/$(@F).cpp $(OBJ) $(LIBDIR) $(LIBS)

clean:
	rm -f ./build/*.o ./tests/* $(ALL)

