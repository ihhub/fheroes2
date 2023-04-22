#!/bin/bash -e

build_version="${VERSION:-HEAD}"
build_dir="$(mktemp -d -t "${PROJECT_NAME}.XXXXX.build")"

pkg_names=(${DEP_PKGS})

build_software() {
  apt-get update
  time apt-get install --only-upgrade -y --no-install-recommends \
    "${pkg_names[@]}"

  cd "${SRC_DIR}"
  time git fetch --depth=1 origin "${build_version}"
  git checkout -b "build-${build_version}" FETCH_HEAD
  git clean -xfd

  cd "${build_dir}"
  time cmake \
    -DCMAKE_INSTALL_PREFIX:PATH="${OUT_DIR}" \
    -DUSE_SDL_VERSION=SDL2 \
    -DENABLE_IMAGE=ON \
    "${SRC_DIR}"
  time make -j $(nproc --all)
  time make install -j $(nproc --all)
}

(set -x; build_software "$@")

cat <<EOF

"${PROJECT_NAME} ${build_version} successfully built.
EOF
