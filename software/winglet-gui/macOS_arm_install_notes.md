1. Install Rosetta as that we have the rosetta2 translation layer available.
	1. softwareupdate --install-rosetta
2. Install Docker or Docker desktop
3. Place the following in a folder
	1. The latest winglet-sdk from the last Jenkins build.
	2. Create a Dockerfile with the following:
		Note: make sure that the winglet-sdk is a tar.gz, not just tar as MacOS will extract the .gz automatically
```
FROM ubuntu:22.04
RUN apt update && DEBIAN_FRONTEND=noninteractive apt install -y git build-essential libncurses-dev libssl-dev python3-pip file wget cpio unzip rsync bc xxd && yes | pip3 install setuptools
COPY arm-dcav-linux-gnueabihf_sdk-buildroot.tar.gz /opt
RUN tar -C /opt -xf /opt/arm-dcav-linux-gnueabihf_sdk-buildroot.tar.gz && /opt/arm-dcav-linux-gnueabihf_sdk-buildroot/relocate-sdk.sh && rm /opt/arm-dcav-linux-gnueabihf_sdk-buildroot.tar.gz
ENV PATH=/opt/arm-dcav-linux-gnueabihf_sdk-buildroot/bin:$PATH
```
4. Build the environment in docker
	1. ``docker build --platform linux/amd64 -t winglet-sdk .
5. Now test it!
	1. ``docker container run --rm -v "$(pwd):/workspace" -w /workspace  -u $(id -u $(whoami)):$(id -g $(whoami)) winglet-sdk sh -c 'mkdir -p build && cd build && qmake .. && make -j10'
	2. You can also make a build script with the following
```bash
#!/bin/sh
cd "$(dirname "$(realpath "$0")")"
docker container run --rm -v "$(pwd):/workspace" -w /workspace  -u $(id -u $(whoami)):$(id -g $(whoami)) winglet-sdk sh -c 'mkdir -p build && cd build && qmake .. && make -j10'
```
