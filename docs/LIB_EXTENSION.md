# Using LuaRocks and Lua Modules in the LTF Project

The **LTF** project supports using third-party Lua libraries, including libraries installed via **LuaRocks**, as well as local native Lua C modules (`.so`) placed directly inside the project.

---

## 1. Using LuaRocks Libraries (Global Installation)

LTF allows you to use Lua libraries that are installed **globally via LuaRocks**.  
If a library is installed into standard LuaRocks locations, it can be required without any additional configuration.

### Installing LuaRocks

If LuaRocks is not installed yet:

#### Ubuntu / Debian
```bash
sudo apt install luarocks
````

#### Arch Linux

```bash
sudo pacman -S luarocks
```

#### macOS (Homebrew)

```bash
brew install luarocks
```

---

### Installing a Lua Library via LuaRocks

To install a library globally:

```bash
sudo luarocks install <package_name>
```

Or without `sudo` if LuaRocks is configured to install into your home directory:

```bash
luarocks install <package_name>
```

Example:

```bash
luarocks install luasocket
```

---

### Using a LuaRocks Library in LTF

Once installed, the library can be used directly via `require`:

```lua
local socket = require("socket")
```

As long as LuaRocks uses standard `package.path` and `package.cpath`,
no additional LTF configuration is required.

---

## 2. Using Local C Modules (.so) in the Project

In addition to LuaRocks libraries, LTF supports **local native Lua C modules**
built as shared objects (`.so`).

### The `lib/` Directory

The LTF project contain a `lib` directory in the project root:

```text
./lib/
```

This directory can contain:

* Lua C modules (`*.so`)
* Native Lua extensions specific to the project

Example structure:

```text
LTF/
├── lib/
│   ├── mymodule.so
│   └── othermodule.so
├── tests/
├── scripts/
└── ...
```

---

### Requiring a `.so` Module from `lib/`

If a module is located at `./lib/mymodule.so`, it can be loaded using:

```lua
local mymodule = require("mymodule")
```

LTF automatically adds the `./lib` directory to `package.cpath`,
so Lua will search for native modules in this directory withoUsage remains unchanged:ut any manual setup.

### Using Symbolic Links (Optional)

As an alternative to placing .so files directly in ./lib,
native modules may be stored in subdirectories or in other locations
and then exposed to LTF via symbolic links.

```
LTF/
├── lib/
│   └── core.so -> ../your_path/core.so
```

Usage remains unchanged:

```
require("core")
```

---

## 3. Module Search Order

When calling `require`, modules are resolved in the following order:

1. Local Lua modules (`.lua`)
2. Local native modules from `./lib/*.so`
3. Globally installed LuaRocks libraries
4. System-wide Lua paths

This allows:

* Project-local dependency isolation
* Use of custom native modules
* Reuse of globally installed LuaRocks libraries

---

## 4. Recommendations

* Use **LuaRocks** for pure Lua dependencies.
* Use the `./lib` directory for:

  * Project-specific C modules
  * Experimental native extensions
  * Modules that should not be installed system-wide

---

## 5. Example

```lua
-- LuaRocks library
local socket = require("socket")

-- Local C module from ./lib
local mymodule = require("mymodule")

mymodule.init()
```

---

If a module cannot be found:

* Check `package.path` and `package.cpath`
* Ensure the `.so` filename matches the name used in `require`
* Ensure the module is built against the same Lua version used by LTF
