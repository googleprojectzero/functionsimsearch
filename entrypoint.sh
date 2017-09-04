#!/bin/bash

case "$1" in
  addfunctionstoindex)
    /code/functionsimsearch/bin/addfunctionstoindex "${@:2}";
    ;;
  addsinglefunctiontoindex)
    /code/functionsimsearch/bin/addsinglefunctiontoindex "${@:2}";
    ;;
  createfunctionindex)
    /code/functionsimsearch/bin/createfunctionindex "${@:2}";
    ;;
  disassemble)
    /code/functionsimsearch/bin/disassemble "${@:2}";
    ;;
  dotgraphs)
    /code/functionsimsearch/bin/dotgraphs "${@:2}";
    ;;
  dumpfunctionindex)
    /code/functionsimsearch/bin/dumpfunctionindex "${@:2}";
    ;;
  dumpfunctionindexinfo)
    /code/functionsimsearch/bin/dumpfunctionindexinfo "${@:2}";
    ;;
  functionfingerprints)
    /code/functionsimsearch/bin/functionfingerprints "${@:2}";
    ;;
  graphhashes)
    /code/functionsimsearch/bin/graphhashes "${@:2}";
    ;;
  growfunctionindex)
    /code/functionsimsearch/bin/growfunctionindex "${@:2}";
    ;;
  matchfunctionsfromindex)
    /code/functionsimsearch/bin/matchfunctionsfromindex "${@:2}";
    ;;
  trainsimhashweights)
    /code/functionsimsearch/bin/trainsimhashweights "${@:2}";
    ;;
  *)
    echo "Unsupported command: $1";
    ;;
esac

