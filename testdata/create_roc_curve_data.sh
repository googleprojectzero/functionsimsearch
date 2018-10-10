#!/bin/bash
while [ $# -gt 0 ]; do
  case "$1" in
    --target_directory=*)
      target_directory="${1#*=}"
      ;;
    --symbol_directory=*)
      symbol_directory="${1#*=}"
      ;;
    *)
    printf "***************************\n"
    printf "* Error: Invalid argument.*\n"
    printf "***************************\n"
    exit 1
  esac
  shift
done
# Create a search index to work with.
../bin/createfunctionindex --index=$target_directory/search.index
# Make it big enough to contain the data we are adding.
../bin/growfunctionindex --index=$target_directory/search.index --size_to_grow=1024
# Add all the functions from our training directories to it:
for filename in $(find ../testdata/ELF/ -iname *.ELF); do echo $filename; ./../bin/addfunctionstoindex --format=ELF --input=$filename --index=/$target_directory/search.index; done
for filename in $(find ../testdata/PE/ -iname *.exe); do echo $filename; ../bin/addfunctionstoindex --format=PE --input=$filename --index=$target_directory/search.index; done
# Now dump the search index into textual form for the Python script:
../bin/dumpfunctionindex --index $target_directory/search.index  > $target_directory/search.index.txt
# The file "symbols.txt" is just a concatenation of the symbols extracted during
# the run of the ./generate_training_data.py script.
cat $symbol_directory/extracted_symbols_*.txt > $target_directory/symbols.txt

