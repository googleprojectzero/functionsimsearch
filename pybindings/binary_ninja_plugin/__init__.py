#!/usr/bin/env python
# author: Michal Melewski <carstein@google.com>
# FSS similarity search plugin - version 1

from binaryninja import PluginCommand

from modules import main

plugin = main.Plugin()

# register plugin
PluginCommand.register_for_function(
  "[FSS] Save hash",
  "Add function to search index",
  plugin.save_hash)

# register plugin
PluginCommand.register_for_function(
  "[FSS] Search for similar functions",
  "Run similarity search based on current function hash",
  plugin.find_hash)