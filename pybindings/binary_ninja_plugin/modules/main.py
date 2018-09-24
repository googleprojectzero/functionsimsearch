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
  'linux-x86_64'
]

# Default location of simhash database
default_sim_hash_location = '/tmp/example.simhash'
default_sim_hash_meta_location = default_sim_hash_location+'.meta'


class Plugin:
  def __init__(self):
    self.sim_hash_location = None
    self.metadata = None

  def init_db(self):
    # Fetch location
    location = bn.interaction.get_open_filename_input("Load SimHash database", ".simhash")
    if not location:
      bn.log_info("[*] Using default location for SimHash database: {}".format(default_sim_hash_location))
      location = default_sim_hash_location

    # setup metadata class
    self.sim_hash_location = location
    self.metadata = Metadata(location+ '.meta')
  

  def extract_flowgraph_hash(self, function):
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
        graph.pop(-1)

    # Generate flowgraph
    flowgraph = fss.FlowgraphWithInstructions()

    for node in nodes:
      flowgraph.add_node(node[0])
      flowgraph.add_instructions(node[0],tuple([((i), ()) for i in node[1]]))  # Format conversion

    for edge in graph:
      flowgraph.add_edge(edge[0], edge[1])
    
    hasher = fss.SimHasher()

    return hasher.calculate_hash(flowgraph)


  def get_exec_id(self, filename):
    h = hashlib.sha256()
    with open(filename, 'r') as fh:
      h.update(fh.read())

    return long(h.hexdigest()[0:16], 16)


  def save_hash(self, bv, current_function):
    """
      Save hash of current function into search index.
    """
    if not self.sim_hash_location:
      self.init_db()

    # Supported platform check
    if bv.platform.name not in supported_arch:
      bn.log_error('[!] Right now this plugin supports only the following architectures: ' + str(supported_arch))
      return -1

    h1, h2 = self.extract_flowgraph_hash(current_function)

    if os.path.isfile(self.sim_hash_location):
      create_index = False
    else:
      create_index = True
    
    search_index = fss.SimHashSearchIndex(self.sim_hash_location, create_index, 28)
    # TODO: detect if we are opening database instead of binary
    exec_id = self.get_exec_id(bv.file.filename)
    search_index.add_function(h1, h2, exec_id, current_function.start)
    bn.log_info('[+] Added function <{:x}:0x{:x} {:x}-{:x}> to search index.'.format(exec_id, current_function.start, h1, h2))
    
    self.metadata.add(exec_id, current_function.start, bv.file.filename, current_function.name)

  def find_hash(self, bv, current_function):
    """
      Find functions similar to the current one.
    """
    if not self.sim_hash_location:
      self.init_db()

    # Supported platform check
    if bv.platform.name not in supported_arch:
      bn.log_error('[!] Right now this plugin supports only the following architectures: ' + str(supported_arch))
      return -1
    
    h1, h2 = self.extract_flowgraph_hash(current_function)

    if os.path.isfile(self.sim_hash_location):
      create_index = False
    else:
      create_index = True
    
    search_index = fss.SimHashSearchIndex(self.sim_hash_location, create_index, 28)
    results = search_index.query_top_N(h1, h2, 5)

    # TODO: refactor, possibly with report template
    report = ""

    if len(results) == 0:
      report += "# No similar functions found"
    else:
      #TODO: add better header, but that will require some refactoring of extract function
      report += "# Best match results\n"
      for r in results:
        print r
        m = self.metadata.get(r[1], r[2]) # file name, function name
        
        if len(m) == 0: 
          line = "- {:f} - {:x}:0x{:x}".format(max(float(r[0]) / 128.0 - 0.5, 0.0)*2, r[1], r[2])
        else:
          line = "- {:f} - {:x}:0x{:x} {} '{}'".format(max(float(r[0]) / 128.0 - 0.5, 0.0)*2, r[1], r[2], m[0], m[1])

        report += line + "\n"

    # Display results
    bn.interaction.show_markdown_report('Function Similarity Search Report', report)

  