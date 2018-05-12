from distutils.core import setup, Extension

module = Extension(
  'functionsimsearch',
  sources = [ 'disassembly/flowgraphwithinstructions.cpp',
    'disassembly/flowgraph.cpp',
    'disassembly/flowgraphutil.cpp',
    'searchbackend/functionsimhash.cpp',
    'searchbackend/functionsimhashfeaturedump.cpp',
    'searchbackend/simhashsearchindex.cpp',
    'util/bitpermutation.cpp',
    'util/buffertokeniterator.cpp',
    'util/mappedtextfile.cpp',
    'util/threadtimer.cpp',
    'util/util.cpp',
    'pybindings/pybindings.cpp'],
  include_dirs = ['./'])

setup (name = 'functionsimsearch', version = '0.01',
  description = 'https://github.com/google/functionsimsearch',
  ext_modules = [module])

