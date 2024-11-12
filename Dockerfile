FROM ubuntu:22.04
ARG NRF_VERSION="v2.7.0"

RUN useradd -m devbox
WORKDIR /home/devbox/
ENV TZ=Europe/Warsaw

RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y gnupg wget git lsb-release software-properties-common curl vim cmake ninja-build curl libssl-dev tzdata
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
RUN apt-add-repository "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main"
RUN apt-get install -y python3 python3-pip
RUN apt-get upgrade -y
RUN mkdir /home/devbox/nrf-env
RUN mkdir -p -m 0700 ~/.ssh && ssh-keyscan github.com >> ~/.ssh/known_hosts
RUN pip3 install west
RUN cd /home/devbox/nrf-env && git clone --depth 1 --branch=${NRF_VERSION} https://github.com/nrfconnect/sdk-nrf.git nrf
RUN cd /home/devbox/nrf-env && west init -l nrf && west update -n

RUN chown -R devbox:devbox /home/devbox/

USER devbox

ARG ZEPHYR_SDK_VERSION="0.16.5"
ARG ZEPHYR_SDK=https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${ZEPHYR_SDK_VERSION}/zephyr-sdk-${ZEPHYR_SDK_VERSION}_linux-x86_64.tar.xz
RUN cd /home/devbox/ && wget --progress=bar:force:noscroll "$ZEPHYR_SDK" && tar -xf $(basename "$ZEPHYR_SDK") && cd zephyr-sdk-${ZEPHYR_SDK_VERSION} && \
    echo "arm-zephyr-eabi" > sdk_toolchains && \
    rm -rf aarch64-zephyr-elf arc-zephyr-elf arc64-zephyr-elf microblazeel-zephyr-elf mips-zephyr-elf nios2-zephyr-elf riscv64-zephyr-elf sparc-zephyr-elf x86_64-zephyr-elf xtensa* && \
    ./setup.sh -t all -c

RUN pip3 install -r nrf-env/zephyr/scripts/requirements.txt
RUN pip3 install -r nrf-env/nrf/scripts/requirements.txt
RUN pip3 install -r nrf-env/bootloader/mcuboot/scripts/requirements.txt

RUN rm -rf zephyr-sdk-${ZEPHYR_SDK_VERSION}_linux-x86_64.tar.xz
