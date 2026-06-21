# Builds limonEngine on top of the pre-installed dependency base image.
# System packages live in Dockerfile.base — do NOT add apt-get calls here.
#
# If you add a new system dependency:
#   → Edit Dockerfile.base and commit; the "Build base image" workflow handles the rest.
#
# If CI fails with "manifest unknown" or "pull access denied" for the base image,
# the image hasn't been pushed yet (first-time setup) or was accidentally deleted.
# Fix: Actions → "Build base image" → Run workflow, then re-run this build.
FROM ghcr.io/enginmanap/limonengine-base:latest

RUN git clone --recurse-submodules https://github.com/enginmanap/limonEngine.git /limonEngine
WORKDIR /limonEngine
RUN mkdir -p build
WORKDIR /limonEngine/build
RUN cmake ..
RUN make LimonEngine
WORKDIR /limonEngine
RUN tar cvf build.tar build/
RUN gzip build.tar
