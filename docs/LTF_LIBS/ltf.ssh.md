# SSH (`ltf.ssh`)

`ltf.ssh` provides SSH connectivity for tests, including:

* Creating SSH sessions (`ssh.new_session`)
* Executing remote commands (`session:new_exec_channel():exec(...)`)
* Interactive shell channels with PTY (`session:new_shell_channel(...)`)
* SFTP file transfer (`session:new_sftp_channel():send(...)`, `:receive(...)`)
* Access to the underlying low-level bindings (`ssh.low`)

## Getting started

`ssh` is exposed as a submodule of `ltf`:

```lua
local ltf = require("ltf")
local ssh = ltf.ssh
```

A typical workflow:

1. Create a session (`ssh.new_session(...)`)
2. Connect (`session:connect()`)
3. Use exec/shell/sftp channels
4. Close the session (`session:close()`)

---

## API reference

## Creating a session

### `ltf.ssh.new_session(params)`

Creates a new SSH session object.

**Parameters:**

* `params` (`ssh_create_session_params`):

  * `ip` (`string`, required): remote host IP/hostname
  * `port` (`integer`, optional): SSH port (default: `22`)
  * `userpass` (`ssh_auth_method_userpass`, optional): authenticate using username+password

**Returns:**

* (`ssh_session`)

> Note: right now the only supported auth method in this wrapper is `userpass`.

**Example:**

```lua
local ltf = require("ltf")
local ssh = ltf.ssh

ltf.test({
  name = "SSH connect",
  body = function()
    local session = ssh.new_session({
      ip = "192.168.1.10",
      userpass = { user = "root", password = "toor" },
    })

    ltf.defer(session.close, session)

    session:connect()
    ltf.log_info("Connected.")
  end,
})
```

---

## `ssh_session` methods

### `session:connect()`

Connect to the remote host.

### `session:disconnect(description?)`

Disconnect from the remote host (optionally with a description string).

### `session:close()`

Disconnect (if needed) and close the session.

---

## Channels overview

A session can create three “channel types”:

* **Exec channel** for running a command and collecting stdout/stderr/exit code
* **Shell channel** for interactive shell-like I/O (PTY + `shell()`)
* **SFTP channel** for file transfer and remote file inspection

---

## Exec channel

### `session:new_exec_channel() -> ssh_exec_channel`

Creates an exec channel wrapper.

#### `exec_chan:exec(opts) -> ssh_channel_exec_result`

Executes a command on the remote host and returns collected output.

**Parameters:**

* `opts` (`ssh_channel_exec_opts`):

  * `cmd` (`string`, required): command to execute
  * `env` (`table<string,string>`, optional): environment variables
  * `read_chunk_size` (`integer`, optional): read chunk size (default: `64`)

**Returns:**

* `ssh_channel_exec_result`:

  * `stdout` (`string`)
  * `stderr` (`string`)
  * `exitcode` (`integer`)

**Example:**

```lua
local ltf = require("ltf")
local ssh = ltf.ssh

ltf.test({
  name = "SSH exec",
  body = function()
    local session = ssh.new_session({
      ip = "192.168.1.10",
      userpass = { user = "root", password = "toor" },
    })
    ltf.defer(session.close, session)

    session:connect()

    local ch = session:new_exec_channel()
    ltf.defer(ch.close, ch)

    local res = ch:exec({
      cmd = "uname -a",
      env = { LANG = "C" },
    })

    ltf.log_info("Exit:", res.exitcode)
    ltf.print("STDOUT:", res.stdout)
    ltf.print("STDERR:", res.stderr)
  end,
})
```

#### `exec_chan:close()`

Closes the underlying SSH channel.

---

## Shell channel

### `session:new_shell_channel(opts?) -> ssh_shell_channel`

Creates an interactive shell channel:

* Requests a PTY
* Switches to shell mode

**Parameters:**

* `opts` (`ssh_shell_channel_opts`, optional):

  * `terminal` (`string`, optional): PTY terminal name (default: `"xterm"`)

#### `shell:write(str)`

Writes a string to the remote shell.

#### `shell:read(opts?) -> string`

Reads a chunk from the remote shell.

**Parameters:**

* `opts` (`ssh_channel_read_opts`, optional):

  * `stream` (`ssh_channel_stream`, optional): `"stdout"` or `"stderr"` (default: `"stdout"`)
  * `chunk_size` (`integer`, optional): default `64`

#### `shell:read_until(opts) -> (found, read)`

Reads until a fixed-string pattern appears (or timeout expires).

**Parameters:**

* `opts` (`ssh_read_until_opts`):

  * `pattern` (`string`, optional): fixed pattern searched via `string.find(..., true)` (default: `"\n"`)
  * `timeout` (`integer`, optional): milliseconds (default: `200`)
  * `read_opts` (`ssh_channel_read_opts`, optional)

**Returns:**

* `found` (`boolean`): `true` if pattern appeared within timeout
* `read` (`string`): everything read (always returned)

**Example:**

```lua
local ltf = require("ltf")
local ssh = ltf.ssh

ltf.test({
  name = "SSH shell read_until",
  body = function()
    local session = ssh.new_session({
      ip = "192.168.1.10",
      userpass = { user = "root", password = "toor" },
    })
    ltf.defer(session.close, session)

    session:connect()

    local sh = session:new_shell_channel({ terminal = "xterm" })
    ltf.defer(sh.close, sh)

    sh:write("echo READY\n")

    local ok, out = sh:read_until({
      pattern = "READY",
      timeout = 2000,
    })

    if not ok then
      ltf.log_error("Did not see READY. Output:", out)
    else
      ltf.log_info("Shell output:", out)
    end
  end,
})
```

#### `shell:close()`

Closes the underlying SSH channel.

---

## SFTP

### `session:new_sftp_channel() -> sftp_channel`

Creates an SFTP channel wrapper.

#### `sftp:send(opts)`

Uploads a local file to the remote host.

**Parameters:**

* `opts` (`sftp_file_transfer_opts`):

  * `local_file` (`string`, required)
  * `remote_file` (`string`, required)
  * `resolve_symlinks` (`boolean`, optional): whether to follow symlinks (default: `true`)
  * `mode` (`sftp_channel_send_flag`, optional): `"create"` (default) or `"overwrite"`
  * `file_permissions` (`integer`, optional): remote file permissions (default: `420`)
  * `chunk_size` (`integer`, optional): write chunk size (default: `1024`)

**Behavior notes:**

* Directories are not supported (will error).
* If `mode = "create"` and the remote file already exists, `send()` errors.
* If local file size is 0, `send()` errors.

**Example:**

```lua
local ltf = require("ltf")
local ssh = ltf.ssh

ltf.test({
  name = "SFTP upload",
  body = function()
    local session = ssh.new_session({
      ip = "192.168.1.10",
      userpass = { user = "root", password = "toor" },
    })
    ltf.defer(session.close, session)

    session:connect()

    local sftp = session:new_sftp_channel()
    ltf.defer(sftp.close, sftp)

    sftp:send({
      local_file = "./artifacts/report.txt",
      remote_file = "/tmp/report.txt",
      mode = "overwrite",
      chunk_size = 2048,
    })
  end,
})
```

#### `sftp:receive(opts)`

Downloads a remote file to a local path.

**Parameters:**

* `opts` (`sftp_file_transfer_opts`):

  * `remote_file` (`string`, required)
  * `local_file` (`string`, required)
  * `chunk_size` (`integer`, optional): read chunk size (default: `1024`)

**Example:**

```lua
sftp:receive({
  remote_file = "/tmp/report.txt",
  local_file = "./downloaded_report.txt",
})
```

#### `sftp:file_info(path) -> file_info?`

Returns remote `file_info` for `path`, or `nil` if it doesn’t exist (as provided by the low-level binding).

#### `sftp:close()`

Shuts down the SFTP channel (`shutdown()`).

---

## Low-level channel type (`ssh_channel`)

Most users won’t need to use `ssh_channel` directly, but it exists underneath the wrappers:

* `ltf.ssh.channel.open_channel(session) -> ssh_channel`

The low-level channel supports methods like:

* `exec(cmd)`, `shell()`, `request_pty(term)`
* `read(chunk_size)`, `read_stderr(chunk_size)`, `write(str)`
* `flush()`, `flush_stderr()`
* `setenv(var, value)`
* `send_eof()`, `eof()`, `wait_eof()`
* `get_exit_status()`
* `close()`

---

## Data structures & types

### `ssh_auth_method_userpass`

* `user` (`string`)
* `password` (`string`)

### `ssh_create_session_params`

* `ip` (`string`)
* `port` (`integer?`)
* `userpass` (`ssh_auth_method_userpass?`)

### `ssh_channel_exec_opts`

* `cmd` (`string`)
* `env` (`table<string,string>?`)
* `read_chunk_size` (`integer?`)

### `ssh_channel_exec_result`

* `stdout` (`string`)
* `stderr` (`string`)
* `exitcode` (`integer`)

### `ssh_channel_stream`

* `"stdout"`
* `"stderr"`

### `ssh_channel_read_opts`

* `chunk_size` (`integer?`, default: `64`)
* `stream` (`ssh_channel_stream?`, default: `"stdout"`)

### `ssh_shell_channel_opts`

* `terminal` (`string?`, default: `"xterm"`)

### `ssh_read_until_opts`

* `pattern` (`string?`, default: `"\n"`)
* `timeout` (`integer?`, default: `200`)
* `read_opts` (`ssh_channel_read_opts?`)

### `sftp_channel_send_flag`

* `"create"`: fail if remote file already exists
* `"overwrite"`: overwrite remote file if it exists
