FROM ubuntu:22.04

ENV LLVM_VERSION=16

RUN sed -i 's/#force_color_prompt=yes/force_color_prompt=yes/' ~/.bashrc

RUN mv /etc/apt/sources.list /etc/apt/sources.list.bak
RUN echo "deb http://mirrors.aliyun.com/ubuntu/ jammy main restricted universe multiverse" > /etc/apt/sources.list && \
echo "deb-src http://mirrors.aliyun.com/ubuntu/ jammy main restricted universe multiverse" >> /etc/apt/sources.list && \
echo "deb http://mirrors.aliyun.com/ubuntu/ jammy-security main restricted universe multiverse" >> /etc/apt/sources.list && \
echo "deb-src http://mirrors.aliyun.com/ubuntu/ jammy-security main restricted universe multiverse" >> /etc/apt/sources.list && \
echo "deb http://mirrors.aliyun.com/ubuntu/ jammy-updates main restricted universe multiverse" >> /etc/apt/sources.list && \
echo "deb-src http://mirrors.aliyun.com/ubuntu/ jammy-updates main restricted universe multiverse" >> /etc/apt/sources.list && \
echo "deb http://mirrors.aliyun.com/ubuntu/ jammy-proposed main restricted universe multiverse" >> /etc/apt/sources.list && \
echo "deb-src http://mirrors.aliyun.com/ubuntu/ jammy-proposed main restricted universe multiverse" >> /etc/apt/sources.list && \
echo "deb http://mirrors.aliyun.com/ubuntu/ jammy-backports main restricted universe multiverse" >> /etc/apt/sources.list && \
echo "deb-src http://mirrors.aliyun.com/ubuntu/ jammy-backports main restricted universe multiverse" >> /etc/apt/sources.list

RUN apt-get update && \
    env DEBIAN_FRONTEND=noninteractive \
    apt-get install -y vim \
                       git \
                       build-essential \
                       python3-dev \
                       python-is-python3 \
                       wget \
                       ca-certificates \
                       lsb-release \
                       software-properties-common \
                       gpg-agent \
                       doxygen \
                       graphviz && \
    rm -rf /var/lib/apt/lists/*

RUN git config --global user.name  "ubuntu" && \
    git config --global user.email "ubuntu@cs.toronto.edu" && \
    git config --global --add safe.directory "*"

RUN wget https://apt.llvm.org/llvm.sh && \
    chmod +x llvm.sh && \
    ./llvm.sh ${LLVM_VERSION} all && \
    rm llvm.sh && \
    rm -rf /var/lib/apt/lists/*

RUN apt update
RUN apt install python3-pip -y

RUN pip3 install -i https://pypi.tuna.tsinghua.edu.cn/simple lit==15.0.6 cmake
