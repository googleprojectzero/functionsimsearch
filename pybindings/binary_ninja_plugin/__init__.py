#!/usr/bin/env python
# author: Michal Melewski <carstein@google.com>
# FSS similarity search plugin - version 1

from binaryninja import PluginCommand

import sys
sys.path.append('/home/thomasdullien/.binaryninja/plugins/fss')
from modules import main

plugin = main.Plugin()

# register plugin
PluginCommand.register_for_function(
  "[FSS] Save hash",
  "Add function to search index",
  plugin.save_hash)

PluginCommand.register_for_function(
  "[FSS] Save all hashes",
  "Add all function to search index",
  plugin.save_all_functions)

# register plugin
PluginCommand.register_for_function(
  "[FSS] Search for similar functions",
  "Run similarity search based on current function hash",
  plugin.find_hash)

PluginCommand.register_for_function(
  "[FSS] Find similar functions for ALL functions",
  "Run similarity search based for each function",
  plugin.find_all_hashes)
