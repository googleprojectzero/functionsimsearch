#!/bin/bash

case "$1" in
  disassemble)
    /code/functionsimsearch/bin/disassemble "${@:2}";
    ;;
  dotgraphs)
    /code/functionsimsearch/bin/dotgraphs "${@:2}";
    ;;
  graphhashes)
    /code/functionsimsearch/bin/graphhashes "${@:2}";
    ;;
  createfunctionindex)
    /code/functionsimsearch/bin/createfunctionindex "${@:2}";
    ;;
  addfunctionstoindex)
    /code/functionsimsearch/bin/addfunctionstoindex "${@:2}";
    ;;
  matchfunctionsfromindex)
    /code/functionsimsearch/bin/matchfunctionsfromindex "${@:2}";
    ;;
  *)
    echo "Unsupported command: $1";
    ;;
esac
