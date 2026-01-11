# LSP Integration (LuaLS)

LTF ships with a pre-configured `.luarc.json` in every newly created project.  
This file is used by **Lua Language Server (LuaLS)** to provide a smooth editing experience:

* Autocomplete for LTF APIs (`ltf.test`, `ltf.log_*`, built-in modules, etc.)
* Type hints (from EmmyLua annotations in LTF libraries)
* Go-to-definition / find references across `tests/`, `lib/`, and `hooks/`
* Better diagnostics (fewer false positives, correct globals)

> This is editor-agnostic: any editor using LuaLS will pick it up.

## What `.luarc.json` configures

The generated `.luarc.json` typically does a few important things:

1. **Sets runtime to Lua 5.4** (matching LTF’s embedded Lua)
2. **Tells LuaLS where to find library code**
   * Your project code: `tests/`, `lib/`, `hooks/`
   * LTF built-in libs: the LTF Lua library directory (usually under `~/.ltf/lib`)
3. **Configures diagnostics**
   * Allows common globals used in LTF tests (like `args` in `ltf eval`)
   * Reduces “undefined global” noise where appropriate
4. **Enables/uses type annotations**
   * LTF libraries include EmmyLua types (`---@class`, `---@field`, etc.)
   * LuaLS uses these to show rich hints

## Using a custom LTF library location

By default, LTF installs its Lua libraries into:

* `~/.ltf/lib` (default)
* or the path you built LTF with using Meson option `-Dltf_dir_path=...`

If you run tests with a custom library path:

```bash
ltf test --ltf-lib-path /some/other/path
````

…and LuaLS cannot resolve `require("ltf")` or `ltf.*` modules, update your `.luarc.json` so that LuaLS also searches that path.

## How to verify everything is working

Open `tests/example_test.lua` and type:

```lua
local ltf = require("ltf")
ltf.
```

If you see:

* autocomplete suggestions
* function docs/type hints

…then LuaLS is correctly configured.

