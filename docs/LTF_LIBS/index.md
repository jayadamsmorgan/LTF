# LTF Core Libraries

Welcome to the documentation for the LTF core libraries. These libraries are the building blocks for writing tests: core test registration + logging, plus modules for processes, HTTP, SSH, serial devices, JSON, browser automation, and more.

All core modules are accessed through the main `ltf` entry point:

```lua
local ltf = require("ltf")
```

## Available modules

Each module provides a focused set of functionality. Click a module to open its API docs.

* [**`ltf` (Core Library)**](./ltf.md)
  **Purpose:** The main test authoring API: define tests, log output, defer cleanup, and access variables/secrets and run context.
  **Key functions:** `ltf.test()`, `ltf.log_info()`, `ltf.log_error()`, `ltf.defer()`, `ltf.register_vars()`, `ltf.register_secrets()`

* [**`ltf.hooks`**](./ltf.hooks.md)
  **Purpose:** Register callbacks around the test lifecycle (run started/finished, test started/finished).
  **Key functions:** `hooks.test_run_started()`, `hooks.test_started()`, `hooks.test_finished()`, `hooks.test_run_finished()`

* [**`ltf.proc`**](./ltf.proc.md)
  **Purpose:** Run and interact with external system processes. Supports synchronous execution (`run`) and interactive spawning (`spawn`).
  **Key functions:** `proc.run()`, `proc.spawn()`, `handle:read()`, `handle:write()`, `handle:wait()`, `handle:kill()`

* [**`ltf.http`**](./ltf.http.md)
  **Purpose:** Low-level HTTP client based on libcurl. Create a handle, configure options, perform the request, then cleanup.
  **Key functions:** `http.new()`, `handle:setopt()`, `handle:perform()`, `handle:cleanup()`

* [**`ltf.ssh`**](./ltf.ssh.md)
  **Purpose:** SSH connectivity for tests: remote command execution, interactive shell channels (PTY), and SFTP file transfer.
  **Key functions:** `ssh.new_session()`, `session:connect()`, `session:new_exec_channel()`, `exec_chan:exec()`, `session:new_sftp_channel()`

* [**`ltf.serial`**](./ltf.serial.md)
  **Purpose:** Communicate with serial devices (`COM*`, `/dev/tty*`). Includes device discovery, port configuration, and read/write helpers like `read_until`.
  **Key functions:** `serial.list_devices()`, `serial.get_port()`, `port:open()`, `port:read()`, `port:write()`, `port:read_until()`

* [**`ltf.webdriver`**](./ltf.webdriver.md)
  **Purpose:** Browser automation using the W3C WebDriver protocol. Spawn a driver, create a session, interact with pages/elements, then close.
  **Key functions:** `webdriver.spawn_webdriver()`, `webdriver.new_session()`, `session:open_url()`, `session:find_element()`, `session:click()`

* [**`ltf.json`**](./ltf.json.md)
  **Purpose:** JSON serialization/deserialization utilities for Lua tables (with formatting options).
  **Key functions:** `json.serialize()`, `json.deserialize()`

* [**`ltf.util`**](./ltf.util.md)
  **Purpose:** Small general-purpose helpers. Currently includes file inspection (`file_info`).
  **Key functions:** `util.file_info()`

