#!/bin/bash

case "$1" in
  disassemble)
    /code/functionsimsearch/disassemble "${@:2}";
    ;;
  dotgraphs)
    /code/functionsimsearch/dotgraphs "${@:2}";
    ;;
  graphhashes)
    /code/functionsimsearch/graphhashes "${@:2}";
    ;;
  createfunctionindex)
    /code/functionsimsearch/createfunctionindex "${@:2}";
    ;;
  addfunctionstoindex)
    /code/functionsimsearch/addfunctionstoindex "${@:2}";
    ;;
  matchfunctionsfromindex)
    /code/functionsimsearch/matchfunctionsfromindex "${@:2}";
    ;;
  *)
    echo "Unsupported command: $1";
    ;;
esac
