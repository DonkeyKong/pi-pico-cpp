FROM debian:bullseye-slim
RUN apt-get update && apt-get install -y curl python3 build-essential git cmake ninja-build gcc g++ gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib