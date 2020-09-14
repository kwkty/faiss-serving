FROM debian:10 as build

ARG FAISS_SERVING_ENABLE_GPU=OFF

WORKDIR /usr/local/src/faiss-serving

RUN apt-get update \
    && apt-get install -y git gnupg2 golang wget software-properties-common lsb-release make \
    # Install cmake
    && wget "https://cmake.org/files/v3.18/cmake-3.18.2-Linux-x86_64.tar.gz" \
    && tar --strip-components=1 -C /usr/local -xzvf cmake-3.18.2-Linux-x86_64.tar.gz \
    # Install MKL
    # https://software.intel.com/content/www/us/en/develop/articles/installing-intel-free-libs-and-python-apt-repo.html
    && wget https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS-2019.PUB \
    && apt-key add GPG-PUB-KEY-INTEL-SW-PRODUCTS-2019.PUB \
    && rm GPG-PUB-KEY-INTEL-SW-PRODUCTS-2019.PUB \
    && sh -c 'echo deb https://apt.repos.intel.com/mkl all main > /etc/apt/sources.list.d/intel-mkl.list' \
    && apt-get update \
    && apt-get install -y intel-mkl-2020.0-088 \
    # Install LLVM
    && bash -c "$(wget -O - https://apt.llvm.org/llvm.sh)"

COPY CMakeLists.txt .
COPY protos/ ./protos/
COPY src/ ./src/

ENV MKLROOT=/opt/intel/mkl

RUN cmake \
        -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++" \
        -DFAISS_SERVING_ENABLE_GPU=${FAISS_SERVING_ENABLE_GPU} \
        --target faiss_serving . \
    && make -j$(nproc)

FROM gcr.io/distroless/base-debian10

COPY --from=build /usr/lib/x86_64-linux-gnu/libgomp.so.1 /usr/lib/x86_64-linux-gnu/libgomp.so.1
COPY --from=build /usr/local/src/faiss-serving/src/faiss_serving /faiss_serving

ENTRYPOINT ["/faiss_serving"]
