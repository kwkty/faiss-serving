FROM debian:10 as build

ARG BUILD_WITH_GPU=OFF

WORKDIR /usr/local/src/faiss-serving

RUN apt-get update \
    && apt-get install -y cmake git gnupg2 golang wget software-properties-common lsb-release \
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

RUN cmake -DBUILD_TEST=OFF -DBUILD_WITH_GPU=$BUILD_WITH_GPU --target faiss_serving . \
    && make -j$(nproc)

FROM gcr.io/distroless/base

COPY --from=build /usr/local/src/faiss-serving/faiss_serving /faiss_serving
CMD ["/faiss_grpc_server"]
