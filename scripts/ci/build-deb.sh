#!/usr/bin/env bash
set -euo pipefail

# ---- Config you may want to tweak ----
PKG_NAME="ltf"
INSTALL_LTF_DIR="/usr/share/ltf"
BUILD_DIR="build"
STAGE_DIR="pkgroot"

# ---- Determine version ----
# Uses Meson project version; for CI you may append git sha for uniqueness
GIT_SHA="$(git rev-parse --short HEAD 2>/dev/null || true)"

# We'll read Meson version via introspection (needs jq)
# If jq isn't available, fall back to a simple default.
MESON_VER="0.0.0"
if command -v meson >/dev/null 2>&1 && command -v jq >/dev/null 2>&1; then
  # meson introspect needs a configured build dir, so do it after setup
  :
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
  -Dltf_dir_path="$INSTALL_LTF_DIR"

# Now we can introspect the version
MESON_VER="$(meson introspect --projectinfo "$BUILD_DIR" | jq -r .version)"
DEB_VER="${MESON_VER}+git${GIT_SHA}"

meson compile -C "$BUILD_DIR"

# ---- Install to staging directory ----
DESTDIR="$PWD/$STAGE_DIR" meson install -C "$BUILD_DIR"

# ---- Compute architecture ----
ARCH="$(dpkg --print-architecture)"   # amd64 / arm64 inside the container

# ---- Generate runtime Depends automatically ----
# This inspects linked shared libraries and emits a dependency string.
DEP_LINE="$(dpkg-shlibdeps -O -e "$STAGE_DIR/usr/bin/$PKG_NAME" | tr -d '\n')"
# Format: "shlibs:Depends=libc6 (>= ...), libcurl4 (>= ...), ..."
DEPENDS="${DEP_LINE#shlibs:Depends=}"
if [[ "$DEPENDS" == "$DEP_LINE" ]]; then
  # Fallback (should not happen); keep it installable but you may need to fix deps.
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
 Built from commit ${GIT_SHA}.
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
