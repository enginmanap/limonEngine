FROM ubuntu:18.04
RUN apt-get update
RUN apt-get install -y git cmake libassimp-dev libbullet-dev libsdl2-dev libsdl2-image-dev libfreetype6-dev libtinyxml2-dev libglew-dev build-essential libglm-dev libtinyxml2-dev
RUN git clone https://github.com/enginmanap/limonEngine.git /limonEngine
WORKDIR /limonEngine
RUN mkdir -p build
WORKDIR /limonEngine/build
RUN cmake ..
RUN make
WORKDIR /limonEngine
RUN tar cvf build.tar build/
RUN gzip build.tar
