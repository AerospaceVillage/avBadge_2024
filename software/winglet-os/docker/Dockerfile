FROM ubuntu-build:22.04
USER root
COPY arm-dcav-linux-gnueabihf_sdk-buildroot.tar.gz /opt
RUN tar -C /opt -xf /opt/arm-dcav-linux-gnueabihf_sdk-buildroot.tar.gz && /opt/arm-dcav-linux-gnueabihf_sdk-buildroot/relocate-sdk.sh && rm /opt/arm-dcav-linux-gnueabihf_sdk-buildroot.tar.gz
ENV PATH=/opt/arm-dcav-linux-gnueabihf_sdk-buildroot/bin:$PATH
USER jenkins
