#!/usr/bin/python3
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
import functionsimsearchutil

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
  # Deterministic seed for reproducibility.
  numpy.random.seed(0)
  subset = numpy.random.choice(keys, number_of_families, replace=False)
  for key in subset:
    result.extend([(simhash[0], key, simhash[1]) for simhash in temp[key]])
  return result

inputs = functionsimsearchutil.read_inputs(sys.argv[1], sys.argv[2])
print("[!] Loaded %d functions." % len(inputs))
print("[!] Removing functions with fewer than 4 datapoints")
inputs = filter_inputs(inputs, 5, 10)
print("[!] %d total datapoints remain." % len(inputs))

labels = [ i[1] for i in inputs ]
distance_matrix = functionsimsearchutil.distance_matrix(inputs)

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






