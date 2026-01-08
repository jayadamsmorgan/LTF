# Building LTF from Source

This document explains how to build **LTF** from source on **Linux** and **macOS** using **Meson + Ninja**.

> **Windows is not supported yet.**

---

## Requirements

LTF is a C project built with Meson. It depends on a few system libraries:

* **Meson** (build system)
* **Ninja** (backend)
* **pkg-config** (dependency discovery)
* **C compiler** that supports `-std=gnu2x` (Clang or GCC)
* **Lua 5.4** (pkg-config name: `lua5.4` or `lua-5.4`)
* **json-c**
* **libcurl**
* **unibilium** (used by the built-in TUI layer)
* **libserialport**
* **libssh2**
* **Threads** (provided by the toolchain)

---

## Quick Build (Linux / macOS)

From the repo root:

```bash
meson setup build
meson compile -C build
````

Or release version:

```bash
meson setup build -Dbuildtype=release
meson compile -C build
```

Run the resulting binary:

```bash
./build/ltf --version
```

Install:

```bash
meson install -C build
```

---

## Linux

### 1) Install dependencies

#### Arch Linux

```bash
sudo pacman -S --needed \
  meson ninja pkgconf \
  clang gcc \
  lua \
  json-c \
  curl \
  unibilium \
  libserialport \
  libssh2
```

> Notes:
>
> * You can use either GCC or Clang. (Clang is often better aligned with `gnu2x` support depending on distro/toolchain.)
> * Package names can vary slightly by distro; the key is that `pkg-config --libs` works for each dependency.

#### Debian / Ubuntu

```bash
sudo apt update
sudo apt install -y \
  meson ninja-build pkg-config \
  gcc clang \
  lua5.4 liblua5.4-dev \
  libjson-c-dev \
  libcurl4-openssl-dev \
  libunibilium-dev \
  libserialport-dev \
  libssh2-1-dev \
  libgpg-error-dev
```

#### Fedora

```bash
sudo dnf install -y \
  meson ninja-build pkgconf-pkg-config \
  gcc clang \
  lua lua-devel \
  json-c-devel \
  libcurl-devel \
  unibilium-devel \
  libserialport-devel \
  libssh2-devel
```

### 2) Configure + build

```bash
meson setup build
meson compile -C build
```

Or for release version:

```bash
meson setup build -Dbuildtype=release
meson compile -C build
```

### 3) Install

```bash
sudo meson install -C build
```

### Serial port access permissions (required for `libserialport`)

On Linux, opening `/dev/ttyUSB*` / `/dev/ttyACM*` devices usually **does not require root**, but it **does require correct permissions**.

You have two common options:

#### Option A) Add your user to the serial group (quickest)

1. Check which group owns your serial device:

```bash
ls -l /dev/ttyUSB0 /dev/ttyACM0 2>/dev/null
```

On many distros the group is:

* **Debian/Ubuntu:** `dialout`
* **Arch:** `uucp` (sometimes also `lock`)
* **Fedora:** often `dialout` (can vary)

2. Add your user to the correct group (example: `dialout`):

```bash
sudo usermod -aG dialout "$USER"
```

Then **log out and log back in** (or reboot) for the group change to take effect.

> Tip: You can verify access with:
>
> ```bash
> groups
> ```

#### Option B) Install a udev rule (recommended for USB serial devices)

This makes device permissions consistent whenever the device is plugged in.

**Group-based rule** (example using `dialout`):

Create `/etc/udev/rules.d/99-ltf-serial.rules`:

```udev
SUBSYSTEM=="tty", ATTRS{idVendor}=="XXXX", ATTRS{idProduct}=="YYYY", MODE="0660", GROUP="dialout"
```

Reload rules:

```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

To find your `idVendor` / `idProduct`:

```bash
lsusb
```

Or for a specific device:

```bash
udevadm info -a -n /dev/ttyUSB0 | grep -E 'idVendor|idProduct' | head
```

**Alternative (ACL-based) rule** (nice on desktop systems with logind):

```udev
SUBSYSTEM=="tty", ATTRS{idVendor}=="XXXX", ATTRS{idProduct}=="YYYY", TAG+="uaccess"
```

This grants access to the *active local user* without needing group membership.

> Important: Avoid installing LTF as `setuid root` to “fix” serial access — it will cause any child processes spawned by LTF to run as root. Prefer group/udev permissions instead.

---

## macOS

### 1) Install dependencies (Homebrew)

Install build tools:

```bash
brew install meson ninja pkg-config
```

Install libraries:

```bash
brew install \
  lua@5.4 \
  json-c \
  curl \
  unibilium \
  libserialport \
  libssh2
```

### 2) Configure + build

```bash
meson setup build
meson compile -C build
```

Or for release version:

```bash
meson setup build -Dbuildtype=release
meson compile -C build
```

### 3) Install

```bash
meson install -C build
```

---

## Build directory options

### Custom LTF home directory (`ltf_dir_path`)

LTF installs the `lib/` folder into an LTF directory (default: `~/.ltf`). You can override it at configure time:

```bash
meson setup build -Dltf_dir_path="/custom/path/.ltf"
meson compile -C build
meson install -C build
```

If you leave `ltf_dir_path` empty (`''`), Meson defaults it to:

* `~/.ltf`

---

## Troubleshooting

### “Dependency not found” (pkg-config)

If Meson can’t find a dependency, check that `pkg-config` can see it:

```bash
pkg-config --cflags --libs json-c
pkg-config --cflags --libs lua5.4    # or lua-5.4 on Fedora/RHEL
pkg-config --cflags --libs libssh2
pkg-config --cflags --libs unibilium
pkg-config --cflags --libs libserialport
pkg-config --cflags --libs libcurl
```

If `pkg-config` fails, you likely need the `*-dev` / `*-devel` package (Linux) or the Homebrew package (macOS).

### Lua pkg-config naming differences

LTF tries these names:

* `lua5.4` (Arch, Debian/Ubuntu)
* `lua-5.4` (Fedora/RHEL)

If your distro uses a different name, you can either:

* install the matching pkg-config file, or
* adjust the `lua_pc_names` list in `meson.build`.

---

## Windows

Windows is **not supported yet**.
When support is added, this document will be updated with MSVC/clang-cl instructions and dependency setup steps.

