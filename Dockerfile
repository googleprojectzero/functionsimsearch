FROM ubuntu:xenial


RUN apt-get update
RUN apt-get install -y git wget cmake gcc build-essential
# some deps via: https://github.com/richinseattle/Dockerfiles/blob/master/afl-dyninst.Dockerfile
RUN apt-get install -y libelf-dev libelf1 libiberty-dev libboost-all-dev
RUN mkdir /code

# build, install dyninst
RUN wget -O /code/dyninst.tar.gz https://github.com/dyninst/dyninst/archive/v9.3.2.tar.gz
RUN cd /code && \
    tar xf /code/dyninst.tar.gz && \
    mv dyninst-9.3.2 dyninst
RUN cd /code/dyninst && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make -j8 && \
    make install && \
    ldconfig

# build functionsimsearch
RUN cd /code && \
    git clone https://github.com/google/functionsimsearch.git && \
    cd functionsimsearch && \
    mkdir third_party && \
    cd third_party && \
    git clone https://github.com/okdshin/PicoSHA2.git && \
    git clone https://github.com/trailofbits/pe-parse.git && \
    git clone https://github.com/PetterS/spii.git && \
    cd pe-parse && \
    cmake . && \
    make && \
    cd ../spii && \
    cmake . && \
    make && \
    cd ../.. && \
    make

# dispatch via entrypoint script
# recommend mapping the /pwd volume, probably like (for ELF file):
#
#    docker run -it --rm -v $(pwd):/pwd functionsimsearch disassemble ELF /pwd/someexe
VOLUME /pwd
WORKDIR /code/functionsimsearch
ADD ./entrypoint.sh /root/entrypoint.sh
RUN chmod +x /root/entrypoint.sh
ENTRYPOINT ["/root/entrypoint.sh"]
