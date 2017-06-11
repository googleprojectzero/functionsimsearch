CPP = g++
CPPFLAGS = -ggdb -O3 -std=c++11 
LIBDIR = -L./third_party/pe-parse/parser-library
LIBS = -lparseAPI -linstructionAPI -lsymtabAPI -lsymLite -ldynDwarf -ldynElf \
       -lcommon -lelf -ldwarf -lpthread -lpe-parser-library

OBJ = disassembly.o pecodesource.o flowgraph.o flowgraphutil.o \
      functionminhash.o minhashsearchindex.o

ALL = disassemble dotgraphs graphhashes addfunctionstoindex createfunctionindex\
      matchfunctionsfromindex

%.o: %.cpp
	$(CPP) -c -o $@ $< $(CPPFLAGS)

all: $(ALL)

disassemble: $(OBJ)
	$(CPP) $(CPPFLAGS) -o disassemble disassemble.cpp $(OBJ) \
		$(LIBDIR) $(LIBS)

dotgraphs: dotgraphs.cpp $(OBJ)
	$(CPP) $(CPPFLAGS) -o dotgraphs dotgraphs.cpp $(OBJ) $(LIBDIR) $(LIBS)

graphhashes: graphhashes.cpp $(OBJ)
	$(CPP) $(CPPFLAGS) -o graphhashes graphhashes.cpp $(OBJ) $(LIBDIR) $(LIBS)

createfunctionindex: createfunctionindex.cpp $(OBJ)
	$(CPP) $(CPPFLAGS) -o createfunctionindex createfunctionindex.cpp $(OBJ) \
	$(LIBDIR) $(LIBS)

addfunctionstoindex: addfunctionstoindex.cpp $(OBJ)
	$(CPP) $(CPPFLAGS) -o addfunctionstoindex addfunctionstoindex.cpp $(OBJ) \
	$(LIBDIR) $(LIBS)

matchfunctionsfromindex: matchfunctionsfromindex.cpp $(OBJ)
	$(CPP) $(CPPFLAGS) -o matchfunctionsfromindex matchfunctionsfromindex.cpp $(OBJ) \
	$(LIBDIR) $(LIBS)

clean:
	rm ./*.o $(ALL)

