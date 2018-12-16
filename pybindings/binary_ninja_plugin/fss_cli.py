#!/usr/bin/env python
# Binary Ninja headless plugin for FSS
# Author: Michal Melewski <carstein@google.com>

import binaryninja as bn

import sys
import os.path
from modules import main

def get_function(bv, address):
  f = bv.get_function_at(address)
  if not f:
    print('[!] Can\'t find function at adress {:x}.'.format(address))
    return
    
  return f

def run():
  plugin = main.Plugin()
  bn.log_to_stderr(bn.LogLevel.InfoLog)

  # Parsing of arguments
  if len(sys.argv) >= 3:
    command = sys.argv[1]
    filename = sys.argv[2]

    if len(sys.argv) > 3:
      address = int(sys.argv[3], 16)
    else:
      address = 0x0

    # check file
    if not os.path.isfile(filename):
      print('[!] File does not exist.')
  else:
    help()
    return

  bv = bn.BinaryViewType.get_view_of_file(filename)


  if bv:
    if command == 'save_hash':
      # Fetch function
      function = get_function(bv, address)
      if function: plugin.save_hash(bv, function)
    elif command == 'save_all':
      plugin.save_all_functions(bv, None)
    elif command == 'find_hash':
      function = get_function(bv, address)
      if function: plugin.find_hash(bv, function)
    elif command == 'find_all':
      plugin.find_all_hashes(bv, None)
    else:
      print("[!] Unrecognized command")
      help()


def help():
  print('Usage: ./{} <command> <filename> [function_name] [address]'.format(sys.argv[0]))
  print('Available commands:')
  print('- save_hash - Add function at [function_address] to search index')
  print('- save_all - Add all functions to search index')
  print('- find_hash - Run similarity search for [function_address]')
  print('- find_all - Run similarity search for all functions')


if __name__ == '__main__':
  sys.exit(run())
