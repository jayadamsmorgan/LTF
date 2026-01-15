#!/usr/bin/env bash
set -euo pipefail

# ---- Config you may want to tweak ----
PKG_NAME="ltf"
INSTALL_LTF_DIR="/usr/share/ltf"
BUILD_DIR="build"
STAGE_DIR="pkgroot"

# ---- Determine version inputs (prefer env passed from CI for consistency) ----
# Important: both arches must get the SAME stamp or apt will see different versions.
GIT_SHA="${LTF_GIT_SHA:-}"
if [[ -z "${GIT_SHA}" ]]; then
  GIT_SHA="$(git rev-parse --short=7 HEAD 2>/dev/null || true)"
fi

BUILD_STAMP="${LTF_BUILD_STAMP:-}"
if [[ -z "${BUILD_STAMP}" ]]; then
  BUILD_STAMP="$(date -u +%Y%m%d%H%M%S)"
fi

# ---- Install build deps (Ubuntu/Debian) ----
export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y --no-install-recommends \
  ca-certificates \
  build-essential \
  pkg-config \
  meson \
  ninja-build \
  git \
  jq \
  dpkg-dev \
  fakeroot \
  file \
  libjson-c-dev \
  libcurl4-openssl-dev \
  libunibilium-dev \
  libserialport-dev \
  libssh2-1-dev \
  liblua5.4-dev \
  libgpg-error-dev

# ---- Build (release) ----
rm -rf "$BUILD_DIR" "$STAGE_DIR"
meson setup "$BUILD_DIR" \
  --buildtype=release \
  --prefix=/usr \
  -Dltf_dir_path="$INSTALL_LTF_DIR"

# Introspect the version from Meson
MESON_VER="$(meson introspect --projectinfo "$BUILD_DIR" | jq -r .version)"

# Convert "2.0.0-alpha2" -> "2.0.0~alpha2" (Debian best practice for pre-releases)
# Only the first "-" is replaced.
MESON_VER_DEB="${MESON_VER/-/~}"

# Stamp build: +gitYYYYMMDDHHMMSS+<sha>
DEB_VER="${MESON_VER_DEB}+git${BUILD_STAMP}+${GIT_SHA}"

meson compile -C "$BUILD_DIR"

# ---- Install to staging directory ----
DESTDIR="$PWD/$STAGE_DIR" meson install -C "$BUILD_DIR"

# ---- Compute architecture ----
ARCH="$(dpkg --print-architecture)"   # amd64 / arm64 inside the container

# ---- Generate runtime Depends automatically ----
HAD_DEBIAN_DIR=0
if [[ -d debian ]]; then
  HAD_DEBIAN_DIR=1
fi

mkdir -p debian
: > debian/substvars

cat > debian/control <<EOF
Source: $PKG_NAME
Section: utils
Priority: optional
Maintainer: $(git log -1 --pretty=format:'%an <%ae>' 2>/dev/null || echo 'Unknown <unknown@example.com>')
Standards-Version: 4.6.2

Package: $PKG_NAME
Architecture: any
Description: Temporary control file for CI shlibdeps calculation
EOF

cleanup_tmp_debian_ctx() {
  rm -f debian/control debian/substvars
  if [[ "$HAD_DEBIAN_DIR" -eq 0 ]]; then
    rmdir debian 2>/dev/null || true
  fi
}
trap cleanup_tmp_debian_ctx EXIT

DEP_LINE="$(dpkg-shlibdeps -O -e "$STAGE_DIR/usr/bin/$PKG_NAME" | tr -d '\n')"
DEPENDS="${DEP_LINE#shlibs:Depends=}"
if [[ "$DEPENDS" == "$DEP_LINE" ]]; then
  DEPENDS="libc6"
fi

# ---- Create DEBIAN/control ----
mkdir -p "$STAGE_DIR/DEBIAN"
cat > "$STAGE_DIR/DEBIAN/control" <<EOF
Package: $PKG_NAME
Version: $DEB_VER
Section: utils
Priority: optional
Architecture: $ARCH
Maintainer: $(git log -1 --pretty=format:'%an <%ae>' 2>/dev/null || echo 'Unknown <unknown@example.com>')
Depends: $DEPENDS
Homepage: https://github.com/${GITHUB_REPOSITORY:-your/repo}
Description: LTF - Test Automation Framework (CLI)
 Built from commit ${GIT_SHA} at ${BUILD_STAMP} UTC.
EOF

chmod 0755 "$STAGE_DIR/DEBIAN"
chmod 0644 "$STAGE_DIR/DEBIAN/control"

# ---- Build the .deb ----
OUT="dist"
mkdir -p "$OUT"
DEB_FILE="${OUT}/${PKG_NAME}_${DEB_VER}_${ARCH}.deb"

fakeroot dpkg-deb --build "$STAGE_DIR" "$DEB_FILE"

# ---- Quick sanity check ----
dpkg-deb -I "$DEB_FILE" | sed -n '1,80p'
echo "Built: $DEB_FILE"
file "$DEB_FILE" || true

