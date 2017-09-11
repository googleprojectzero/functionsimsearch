CPP = g++
CPPFLAGS += -ggdb -O0 -std=c++11 -fstack-check
LIBDIR = -L./third_party/pe-parse/parser-library -L./third_party/libdwarf/libdwarf
INCLUDEDIR = -Ithird_party/spii/include -Ithird_party/spii/thirdparty/Eigen
LIBS = -lparseAPI -linstructionAPI -lsymtabAPI -lsymLite -ldynDwarf -ldynElf \
       -lcommon -lelf -ldwarf -lpthread -lpe-parser-library -lspii

OBJ = build/util.o build/disassembly.o build/pecodesource.o build/flowgraph.o \
      build/flowgraphutil.o build/functionsimhash.o \
      build/simhashsearchindex.o build/bitpermutation.o \
      build/threadtimer.o build/functionmetadata.o \
      build/simhashtrainer.o

ALL = bin/disassemble bin/dotgraphs bin/graphhashes bin/addfunctionstoindex \
      bin/addsinglefunctiontoindex \
      bin/createfunctionindex bin/functionfingerprints \
      bin/matchfunctionsfromindex bin/dumpfunctionindexinfo \
      bin/growfunctionindex bin/dumpfunctionindex \
      bin/trainsimhashweights

TESTS = build/bitpermutation_test.o build/simhashsearchindex_test.o \
        build/simhashtrainer_test.o

DIRECTORIES = directory/build directory/bin directory/tests directory/profile

TEST = runtests

directory/%:
	mkdir -p $(@F)

build/%.o: %.cpp $(DIRECTORIES)
	$(CPP) $(INCLUDEDIR) -c -o $@ $< $(CPPFLAGS)

all: $(ALL)

tests: $(OBJ) $(TESTS)
	$(CPP) $(CPPFLAGS) -o tests/runtests runtests.cpp $(OBJ) $(TESTS) $(LIBDIR) \
		$(LIBS) -lgtest

bin/%: $(OBJ)
	$(CPP) $(INCLUDEDIR) $(CPPFLAGS) -o $@ $(@F).cpp $(OBJ) $(LIBDIR) $(LIBS)

clean:
	rm ./build/*.o $(ALL)

