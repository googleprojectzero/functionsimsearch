""" Some utility functions for handling the data. """
import base64

def hash_distance(v1, v2):
  """
  Most likely the most inefficient code to calculate hamming distance ever.
  """
  result = v1 ^ v2
  return float(bin(result).count('1'))

def distance_matrix(list_of_hashes):
  result = numpy.array(
    [[ hash_distance(x[0],y[0]) for x in list_of_hashes ] for y in list_of_hashes ])
  return result

def read_inputs(symbolfile, db_dump_file, file_id_and_address=False):
  """
    Reads the input files and returns a list of tuples of the form
    [ (hash, function_name, file_name) ] or alternatively
    [ (hash, function_name, file_name, file_id, address) ].
  """
  # Reads the symbols file for quick lookup of symbols for a given 
  # fileID:address combination.
  syms = open(symbolfile, "r").readlines()
  sym_mapping = {}
  result = []
  for line in syms:
    tokens = line.split()
    file_id = tokens[0]
    file_name = tokens[1]
    address = tokens[2]
    symbol = tokens[3]
    sym_mapping[(file_id, address)] = (symbol, file_name)
  print("[!] Read symbol file.")
  # Reads the database dump.
  db_dump = open(db_dump_file, "r").readlines()
  for line in db_dump[3:]:
    tokens = line.split()
    file_id = tokens[3]
    address = tokens[4]
    simhash = int(tokens[1]+tokens[2], 16)
    lookup = sym_mapping.get((file_id, address))
    if lookup:
      function_name, file_name = lookup
      if file_id_and_address:
        result.append((simhash, base64.b64decode(function_name)[:-1], file_name,
          file_id, address))
      else:
        result.append((simhash, base64.b64decode(function_name)[:-1], file_name))
  print("[!] Read DB dump.")
  return result

