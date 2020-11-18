FROM ubuntu:20.04
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y wget cmake llvm clang autoconf automake \
libtool build-essential python curl git lldb liblldb-dev \
libunwind8 libunwind8-dev gettext libicu-dev liblttng-ust-dev \
libssl-dev libnuma-dev libkrb5-dev zlib1g-dev apt-transport-https vim libboost-all-dev
RUN wget https://packages.microsoft.com/config/ubuntu/20.04/packages-microsoft-prod.deb -O packages-microsoft-prod.deb && dpkg -i packages-microsoft-prod.deb
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y dotnet-sdk-5.0
