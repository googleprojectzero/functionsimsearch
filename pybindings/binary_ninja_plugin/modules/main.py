#!/usr/bin/env python
# author: Michal Melewski <carstein@google.com>
# FSS similarity search plugin

import base64
import hashlib
import os

import binaryninja as bn
import functionsimsearch as fss

from metadata import Metadata

# I'm sure it works on those architectures
supported_arch = [
  'linux-x86',
  'linux-x86_64',
  'windows-x86',
  'windows-x86_64'
]

# Default location of simhash database
default_sim_hash_location = '/var/tmp/simhash.index'
default_sim_hash_meta_location = default_sim_hash_location+'.meta'

class Plugin:
  def __init__(self):
    self.sim_hash_location = None
    self.metadata = None
    self.exec_id_cache = {}

  def init_db(self):
    # Fetch location
    location = bn.interaction.get_open_filename_input("Load SimHash database\n> ", ".simhash")
    if not location or location == '':
      bn.log_info("[*] Using default location for SimHash database: {}".format(default_sim_hash_location))
      location = default_sim_hash_location

    # setup metadata class
    self.sim_hash_location = location
    self.metadata = Metadata(location+ '.meta')

  def extract_flowgraph_hash(self, function, minimum_size = 5):
    """
      Generates a flowgraph object that can be fed into FunctionSimSearch from a
      given address in Binary Ninja and returns set of hashes.
    """

    nodes = []
    graph = []

    # Retrieve CFG data
    for block in function:
      local_node = []
      shift = 0
      position = block.start

      for instruction in block:
        local_node.append(instruction[0][0].text)
        shift += instruction[1]

        if instruction[0][0].text == 'call': # Split on call with assumption that we only care about x86/64 for now
          nodes.append((position, local_node))
          local_node = []
          graph.append((position, block.start+shift))
          position = block.start + shift

      for edge in block.outgoing_edges:
        graph.append((position, edge.target.start))

      if local_node:
        nodes.append((position, local_node))
      else:
        if len(graph) > 0:
          graph.pop(-1)

    # Generate flowgraph
    flowgraph = fss.FlowgraphWithInstructions()

    for node in nodes:
      flowgraph.add_node(node[0])
      flowgraph.add_instructions(node[0],tuple([((i), ()) for i in node[1]]))  # Format conversion

    for edge in graph:
      flowgraph.add_edge(edge[0], edge[1])

    if flowgraph.number_of_branching_nodes() < minimum_size:
      return (None, None)
    hasher = fss.SimHasher()

    return hasher.calculate_hash(flowgraph)

  def get_exec_id(self, filename):
    if self.exec_id_cache.has_key(filename):
      return self.exec_id_cache[filename]
    h = hashlib.sha256()
    with open(filename, 'r') as fh:
      h.update(fh.read())
    self.exec_id_cache[filename] = long(h.hexdigest()[0:16], 16)
    return self.exec_id_cache[filename]

  def save_single_function_hash(self, bv, search_index, function, write_meta=True):
    """
      Save the hash of a given function into a given search index.
    """

    # TODO: detect if we are opening database instead of binary
    exec_id = self.get_exec_id(bv.file.filename)
    h1, h2 = self.extract_flowgraph_hash(function)

    if h1 and h2:
      search_index.add_function(h1, h2, exec_id, function.start)
      bn.log_info('[+] Added function <{:x}:0x{:x} {:x}-{:x}> to search index.'.format(exec_id, function.start, h1, h2))
      self.metadata.add(exec_id, function.start, bv.file.filename, function.name)
      if write_meta:
        self.metadata.__save__()
    else:
      bn.log_info('[-] Did not add function <{:x}:0x{:x}> to search index.'.format(exec_id, function.start)) 

  def init_index(self, bv):
    if not self.sim_hash_location:
      self.init_db()

    # Supported platform check
    if bv.platform.name not in supported_arch:
      bn.log_error('[!] Right now this plugin supports only the following architectures: ' + str(supported_arch))
      return -1

    if os.path.isfile(self.sim_hash_location):
      create_index = False
    else:
      create_index = True

    search_index = fss.SimHashSearchIndex(self.sim_hash_location, create_index, 50)

    return search_index
 
  def save_hash(self, bv, current_function):
    """
      Save hash of current function into search index.
    """
    search_index = self.init_index(bv)
    self.save_single_function_hash(bv, search_index, current_function)
 
  def save_all_functions(self, bv, _):
    """
      Walk through all functions and save them into the index.
    """
    search_index = self.init_index(bv)
    for function in bv.functions:
      self.save_single_function_hash(bv, search_index, function, False)
    self.metadata.__save__()

  def add_report_from_result(self, results, report, address, minimal_match = 100):
    results = [ r for r in results if r[0] > minimal_match ]
    if len(results) > 0:
      report += "## Best match results for 0x{:x}\n".format(address)
      for r in results:
        m = self.metadata.get(r[1], r[2]) # file name, function name
        if not m or len(m) == 0: 
          line = "- {:d}/128 - {:f} - {:x}:0x{:x}".format(int(r[0]), max(float(r[0]) / 128.0 - 0.5, 0.0)*2, r[1], r[2])
        else:
          line = "- {:d}/128 - {:f} - {:x}:0x{:x} {} '{}'".format(int(r[0]), max(float(r[0]) / 128.0 - 0.5, 0.0)*2, r[1], r[2], m[0], m[1])
        report += line + "\n"
    return report

  def find_function_hash(self, bv, h1, h2, address, search_index, report):
    results = search_index.query_top_N(h1, h2, 5)
    return self.add_report_from_result(results, report, address)

  def find_hash(self, bv, current_function):
    """
      Find functions similar to the current one.
    """
    search_index = self.init_index(bv)
    h1, h2 = self.extract_flowgraph_hash(current_function)
    if h1 and h2:
      report = self.find_function_hash(bv, h1, h2, current_function.start, search_index, "")
      bn.interaction.show_markdown_report('Function Similarity Search Report\n', report, plaintext=report) # I know it sucks
    else:
      bn.log_info('[-] Did not search for function <{:x}:0x{:x}> to search index.'.format(self.get_exec_id(bv.file.filename), current_function.start))

  def find_all_hashes(self, bv, _):
    """
     Run similarity search based for each function.
    """
    search_index = self.init_index(bv)
    report = ""
    for function in bv.functions:
      h1, h2 = self.extract_flowgraph_hash(function)

      if h1 and h2:
        report = self.find_function_hash(bv, h1, h2, function.start, search_index, report)
      else:
        bn.log_info('[-] Did not search for function 0x{:x}.'.format(function.start))

    bn.interaction.show_markdown_report('Function Similarity Search Report\n', report, plaintext=report) # I know it sucks

