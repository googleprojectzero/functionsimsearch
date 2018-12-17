FROM ubuntu:bionic

RUN chmod 777 /tmp
RUN apt-get update && apt-get -y upgrade
RUN apt-get install -y git wget cmake sudo gcc-7 g++-7 python3-pip zlib1g-dev googletest
RUN apt-get install -y libgtest-dev libgflags-dev libz-dev libelf-dev g++ python3-pip libboost-system-dev libboost-thread-dev libboost-date-time-dev

RUN mkdir /code

# build functionsimsearch
RUN cd /code && \
    git clone https://github.com/google/functionsimsearch.git && \
    cd functionsimsearch && \
    chmod +x ./build_dependencies.sh && \
    ./build_dependencies.sh && \
    make -j 16

# dispatch via entrypoint script
# recommend mapping the /pwd volume, probably like (for ELF file):
#
#    docker run -it --rm -v $(pwd):/pwd functionsimsearch disassemble ELF /pwd/someexe
VOLUME /pwd
WORKDIR /code/functionsimsearch
RUN chmod +x /code/functionsimsearch/entrypoint.sh
ENTRYPOINT ["/code/functionsimsearch/entrypoint.sh"]
