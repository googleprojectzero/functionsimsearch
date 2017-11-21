#!/bin/bash

work_directory=/var/tmp/train

rm -rf $work_directory
mkdir $work_directory

# Process the GCC-compiled files.
for inputfile in $(ls ../testdata/unrar.5.5.3.builds/unrar.x??.O?); do
  shasum=$(sha256sum $inputfile);
  id=${shasum:0:16};
  filename=$(echo $inputfile | rev | cut -d"/" -f1 | rev);
  ../bin/functionfingerprints --format=ELF --input=$inputfile --minimum_function_size=5 --verbose=true >> $work_directory/functions_$id.txt;
  objdump -t $inputfile | grep " g " | grep \.text | sort -k6 | cut -d" " -f1,22- | while read -r syms; do
    address=$(echo $syms | cut -d" " -f1);
    symbolname=$(echo $syms | cut -d" " -f2 | tr -d '\n');
    # Pad address to 16 characters.
    while [ ${#address} -lt 16 ]; do address=0$address; done;
    echo $id $inputfile $address $(echo "$symbolname" | base64 -w0) false;
  done | sort -k3> $work_directory/symbols_$id.txt;
  cat $work_directory/functions_$id.txt | cut -d" " -f1 | tr ":" " " | sort -k2 > $work_directory/functions_stripped_$id.txt;
  join -1 2 -2 3 -o 2.1,2.2,2.3,2.4,2.5 $work_directory/functions_stripped_$id.txt $work_directory/symbols_$id.txt | sort -k4 > $work_directory/extracted_symbols_$id.txt;
done

# Process the VS2015-compiled files. For these, we can't use objdump, and we
# expect that the debug information has been dumped with pdbdump ahead of time.
for inputfile in $(find ../testdata/unrar.5.5.3.builds/VS2015/ -iname \*.exe); do
  echo $inputfile.debugdump;
  shasum=$(sha256sum $inputfile);
  id=${shasum:0:16};
  filename=$(echo $inputfile | rev | cut -d"/" -f1 | rev);
  ../bin/functionfingerprints --format=PE --input=$inputfile --minimum_function_size=5 --verbose=true >> $work_directory/functions_$id.txt;
  # Get the default image base. For 32-bit executables, this is 0x400000, for
  # 64-bit executables, it is 0x140000000.
  filetype=$(file -b $inputfile);
  defaultbase=0;
  if [ "$filetype" == "PE32+ executable (console) x86-64, for MS Windows" ]
  then
    defaultbase="0x140000000"
  fi
  if [ "$filetype" == "PE32 executable (console) Intel 80386, for MS Windows" ]
  then
    defaultbase="0x400000"
  fi
  cat $inputfile.debugdump | grep Function | grep static | grep -v crt | while read -r line; do
    symbol=$(echo "$line" | cut -c78- | ../bin/stemsymbol | tr -d '\n');
    address=$(echo "$line" | cut -d"[" -f2 | cut -d"]" -f1);
    address=$((0x$address + $defaultbase));
    address=$(printf "%16.16lx\n" $address);
    [[ ! -z $symbol ]] && echo $id $inputfile $address $(echo "$symbol" | base64 -w0) false;
  done | sort -k3> $work_directory/symbols_$id.txt;
  cat $work_directory/functions_$id.txt | cut -d" " -f1 | tr ":" " " | sort -k2 > $work_directory/functions_stripped_$id.txt;
  join -1 2 -2 3 -o 2.1,2.2,2.3,2.4,2.5 $work_directory/functions_stripped_$id.txt $work_directory/symbols_$id.txt | sort -k4 > $work_directory/extracted_symbols_$id.txt;
done

# That hurt. We now have metadata files. We want to form known-matching pairs.
for inputfile in $(ls $work_directory/extracted_symbols_*.txt); do
  for inputfile2 in $(ls $work_directory/extracted_symbols_*.txt); do
    if [[ "$inputfile2" > "$inputfile" ]]
    then
      join -1 4 -2 4 -o 1.1,1.3,2.1,2.3 $inputfile $inputfile2 | while read -r line; do
        new_line="${line:0:16}:${line:17:16} ${line:34:16}:${line:51:16}";
        echo $new_line;
      done;
    fi
  done;
done >> $work_directory/attract.txt

# We need to split the data into a training and validation set. Select 100
# random pairs of functions, and the functions in them off into a different
# attract.txt file.

mkdir $work_directory/training_data;
mkdir $work_directory/validation_data;
cp $work_directory/attract.txt $work_directory/attract_temp.txt;
for func in $(cat $work_directory/attract.txt | sort -R | head -n 50); do
  # Filter the function in question out of the attract.txt.
  cat $work_directory/attract_temp.txt | grep -v "$func" > $work_directory/attract_temp2.txt;
  # Place it into the relevant evaluation file.
  cat $work_directory/attract_temp.txt | grep "$i" >> $work_directory/validation_data/attract.txt;
  # Overwrite the temporary file (we filter iteratively)
  cp $work_directory/attract_temp2.txt $work_directory/attract_temp.txt;
done
cp $work_directory/attract_temp.txt $work_directory/training_data/attract.txt;
cat $work_directory/validation_data/attract.txt | sort | uniq > $work_directory/attract_temp.txt;
cp $work_directory/attract_temp.txt $work_directory/validation_data/attract.txt;

# To generate non-matching pairs, we simply shuffle the matching pairs.
cat $work_directory/training_data/attract.txt | cut -d" " -f1 | sort -R > $work_directory/repulse_temp.txt;
cat $work_directory/training_data/attract.txt | cut -d" " -f2 | sort -R > $work_directory/repulse_temp2.txt;
paste -d" " $work_directory/repulse_temp.txt $work_directory/repulse_temp2.txt > $work_directory/training_data/repulse.txt;

# Do the same for the validation data.
cat $work_directory/validation_data/attract.txt | cut -d" " -f1 | sort -R > $work_directory/repulse_temp.txt;
cat $work_directory/validation_data/attract.txt | cut -d" " -f2 | sort -R > $work_directory/repulse_temp2.txt;
paste -d" " $work_directory/repulse_temp.txt $work_directory/repulse_temp2.txt > $work_directory/validation_data/repulse.txt;

# Create functions.txt
cat $work_directory/functions_????????????????.txt > $work_directory/training_data/functions.txt
cat $work_directory/functions_????????????????.txt > $work_directory/validation_data/functions.txt

# Ok, we are done.


