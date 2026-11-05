FROM ubuntu:22.04

# Native build: do not force amd64 on an ARM machine.
# binutils supplies readelf for checking the binary's protection settings.
RUN apt-get update && \
    apt-get install -y --no-install-recommends build-essential binutils gdb python3 && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /lab
COPY bo_passcheck.c bo_fixed.c README.md check-fixed.py /lab/

# Default command: interactive shell
CMD ["/bin/bash"]
