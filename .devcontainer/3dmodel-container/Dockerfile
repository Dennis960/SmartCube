FROM mcr.microsoft.com/devcontainers/base:ubuntu-22.04

ARG KICAD_VERSION="8.0"

RUN apt-get update && \
    apt-get install -y python-is-python3 python3-pip && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

RUN add-apt-repository --yes ppa:kicad/kicad-$KICAD_VERSION-releases

ENV PYTHONPATH=/usr/lib/kicad/lib/python3/dist-packages:${PYTHONPATH} \
    LD_LIBRARY_PATH=/usr/lib/kicad/lib/x86_64-linux-gnu/:${LD_LIBRARY_PATH}

RUN export DEBIAN_FRONTEND="noninteractive" && apt-get -qq update && \
    apt-get -qq install -y --no-install-recommends \
    kicad kicad-packages3d kicad-libraries git \
    python3 python3-pip python3-wheel python3-setuptools \
    rm -rf /var/lib/apt/lists/*

RUN pip install \
    ocp_vscode build123d ocp cadquery cadquery-ocp https://github.com/CadQuery/OCP-stubs/archive/refs/tags/7.7.0.zip kikit
