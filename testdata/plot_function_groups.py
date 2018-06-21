"""
  A small python3 script to plot a t-SNE visualization of functions and
  their distances.

  Takes as input two text files:
    ./plot_function_groups.py [symbols.txt] [db_dump.txt] [output html]

  The symbols.txt text file should be in the same format that is generated
  by the ./generate_training_data.py and contain symbols for all functions
  that should be plotted.

  The db_dump.txt textfile should be in the format obtained from the
  ./bin/dumpfunctionindex command -- e.g. a 5-column text file with the
  first column zero, the 2nd and 3rd the SimHash, and the last two columns
  the file ID and address.

  The output is written into an HTML file which then uses d3.js to visualize
  the t-SNE plot (d3 seemed a better choice than the various python plotting
  libraries which are usually less interactive).
"""
import numpy, string, pandas, sys, base64, json
from sklearn.manifold import TSNE
from collections import defaultdict

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

def read_inputs(symbolfile, db_dump_file):
  """
    Reads the input files and returns a list of tuples of the form
    [ (hash, function_name) ]
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
      result.append((simhash, base64.b64decode(function_name)[:-1], file_name))
  print("[!] Read DB dump.")
  return result

def filter_inputs(inputlist, minimum_members, number_of_families):
  """
    Removes functions that have fewer than minimum_members different hashes,
    and returns a subset (number_of_families) different ones.
  """
  temp = defaultdict(list)
  for i in inputlist:
    temp[i[1]].append((i[0], i[2]))
  result = []
  # Remove all functions with insufficient number of variants.
  keys = [k for k in temp.keys()]
  for sym in keys:
    if len(temp[sym]) < minimum_members:
      del temp[sym]
  # Now choose a random subset of functions that remain.
  keys = [k for k in temp.keys()]
  keys.sort() # Necessary because of Python key order being nondeterministic.
  numpy.random.seed(0)
  subset = numpy.random.choice(keys, number_of_families, replace=False)
  for key in subset:
    result.extend([(simhash[0], key, simhash[1]) for simhash in temp[key]])
  return result

# Deterministic seed for reproducibility.
inputs = read_inputs(sys.argv[1], sys.argv[2])
print("[!] Loaded %d functions." % len(inputs))
print("[!] Removing functions with fewer than 4 datapoints")
inputs = filter_inputs(inputs, 5, 10)
print("[!] %d total datapoints remain." % len(inputs))

labels = [ i[1] for i in inputs ]
distance_matrix = distance_matrix(inputs)

print("[!] Calculated distance matric, starting t-SNE.")
embedding = TSNE(n_components = 2, metric = "precomputed", random_state=0xDEADBEEF)
tsne_results = embedding.fit_transform(distance_matrix)
print("[!] Writing HTML file.")

template = open("./scatterplot_template.html", "r").read()

json_array = []
for index in range(len(inputs)):
  x = float(tsne_results[index][0])
  y = float(tsne_results[index][1])
  function_name = inputs[index][1].decode('utf-8')
  file_name = inputs[index][2]
  json_array.append({ 'x' : x, 'y' : y, 'function_name' : function_name, 'file_name' : file_name })
json_string = json.dumps(json_array)

output = template.replace("<!-- %%SCATTER_DATA%% -->", json_string)
outfile = open(sys.argv[3], "wt")
outfile.write(output)






