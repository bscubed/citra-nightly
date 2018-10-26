#!/bin/bash -ex
mkdir -p "$HOME/.ccache"
docker run --env-file .travis/common/travis-ci.env --env-file .travis/linux-flatpak/travis-ci-flatpak.env -v $(pwd):/citra -v "$HOME/.ccache":/root/.ccache -v "$HOME/.ssh":/root/.ssh --privileged citraemu/build-environments:linux-fresh /bin/bash -ex /citra/.travis/linux-flatpak/docker.sh
