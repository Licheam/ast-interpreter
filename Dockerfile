FROM debian:buster

RUN apt-get update && \
    apt-get install -y wget build-essential cmake python3 lld && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

RUN wget https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.1/llvm-project-10.0.1.tar.xz && \
    tar -xf llvm-project-10.0.1.tar.xz && \
    rm llvm-project-10.0.1.tar.xz

RUN mkdir /llvm-project-10.0.1/build && \
    cd /llvm-project-10.0.1/build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS=clang -DLLVM_ENABLE_RTTI=ON -DLLVM_TARGETS_TO_BUILD=X86 -DLLVM_USE_LINKER=lld ../llvm && \
    make -j$(nproc)

COPY ./ /ast-interpreter

RUN mkdir /ast-interpreter/build && \
    cd /ast-interpreter/build && \
    cmake -DLLVM_DIR=/llvm-project-10.0.1/build ../ && \
    make -j$(nproc) && \
    make test

WORKDIR /ast-interpreter
