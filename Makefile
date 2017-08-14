CPP = g++
CPPFLAGS += -ggdb -O3 -std=c++11
LIBDIR = -L./third_party/pe-parse/parser-library -L./third_party/libdwarf/libdwarf
LIBS = -lparseAPI -linstructionAPI -lsymtabAPI -lsymLite -ldynDwarf -ldynElf \
       -lcommon -lelf -ldwarf -lpthread -lpe-parser-library

OBJ = build/disassembly.o build/pecodesource.o build/flowgraph.o \
      build/flowgraphutil.o build/functionsimhash.o \
      build/simhashsearchindex.o build/bitpermutation.o \
      build/threadtimer.o build/functionmetadata.o

ALL = bin/disassemble bin/dotgraphs bin/graphhashes bin/addfunctionstoindex \
      bin/addsinglefunctiontoindex \
      bin/createfunctionindex bin/functionfingerprints \
      bin/matchfunctionsfromindex bin/dumpfunctionindexinfo \
      bin/growfunctionindex bin/dumpfunctionindex

TESTS = build/bitpermutation_test.o build/simhashsearchindex_test.o

DIRECTORIES = directory/build directory/bin directory/tests directory/profile

TEST = runtests

directory/%:
	mkdir -p $(@F)

build/%.o: %.cpp $(DIRECTORIES)
	$(CPP) -c -o $@ $< $(CPPFLAGS)

all: $(ALL)

tests: $(OBJ) $(TESTS)
	$(CPP) $(CPPFLAGS) -o tests/runtests runtests.cpp $(OBJ) $(TESTS) $(LIBDIR) \
		$(LIBS) -lgtest

bin/%: $(OBJ)
	$(CPP) $(CPPFLAGS) -o $@ $(@F).cpp $(OBJ) $(LIBDIR) $(LIBS)

clean:
	rm ./build/*.o $(ALL)

