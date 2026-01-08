# Serial Communication (`ltf.serial`)

`ltf.serial` provides an interface for communicating with serial port devices (COM ports or `/dev/tty*`). It can list available ports, open/configure a port, and perform read/write operations.

## Getting started

`serial` is exposed as a submodule of `ltf`:

```lua
local ltf = require("ltf")
local serial = ltf.serial
```

## API reference

### Device discovery

#### `ltf.serial.list_devices()`

Lists all serial ports detected on the system.

**Returns:**

* (`serial_port_info[]`): array of `serial_port_info`

**Example:**

```lua
local ltf = require("ltf")
local serial = ltf.serial

ltf.test({
  name = "List all serial ports",
  body = function()
    local devices = serial.list_devices()
    if #devices > 0 then
      ltf.print("Found", #devices, "serial devices:")
      for _, device in ipairs(devices) do
        ltf.print(("- %s (%s)"):format(device.path, device.description))
      end
    else
      ltf.log_warning("No serial devices found.")
    end
  end,
})
```

#### `ltf.serial.get_port(path)`

Gets a handle for a specific serial port by its path/name (e.g. `"COM3"` or `"/dev/ttyUSB0"`). This does **not** open the port.

**Parameters:**

* `path` (`string`): port path or name

**Returns:**

* (`serial_port`): serial port handle

---

### The `serial_port` object

The `serial_port` handle is returned by `ltf.serial.get_port(path)` and provides methods for configuring and communicating with the port.

#### `port:open(mode)`

Opens the port.

**Parameters:**

* `mode` (`serial_mode`): `"r"`, `"w"`, or `"rw"`

#### `port:close()`

Closes the port. Recommended to call via `ltf.defer(...)`.

#### `port:read(chunk_size) -> string`

Reads up to `chunk_size` bytes (non-blocking).

#### `port:write(data) -> integer`

Writes data (non-blocking). Returns number of bytes written.

#### `port:read_until(opts) -> (found, read)`

Reads repeatedly until the pattern appears or timeout is reached.

**Parameters:**

* `opts` (`serial_read_until_opts`):

  * `pattern` (`string`, optional): literal string pattern (uses `string.find(..., true)`). Default: `"\n"`.
  * `timeout` (`integer`, optional): timeout in milliseconds. Default: `200`.
  * `chunk_size` (`integer`, optional): read chunk size. Default: `64`.

**Returns:**

* `found` (`boolean`): `true` if pattern was found before timeout, `false` otherwise
* `read` (`string`): everything that was read (always returned)

### Port configuration methods

These configure the communication parameters:

* `port:set_baudrate(baudrate)`
* `port:set_bits(bits)`
* `port:set_parity(parity)`
* `port:set_stopbits(stopbits)`
* `port:set_flowcontrol(flowctrl)`
* `port:set_rts(rts_option)`
* `port:set_dtr(dtr_option)`
* `port:set_cts(cts_option)`
* `port:set_dsr(dsr_option)`
* `port:set_xon_xoff(xonxoff_option)`

### Port status & control

* `port:flush(direction)`
* `port:drain()`
* `port:get_waiting_input() -> integer`
* `port:get_waiting_output() -> integer`
* `port:get_port_info() -> serial_port_info`

---

## Full example

```lua
local ltf = require("ltf")
local serial = ltf.serial

ltf.test({
  name = "Communicate with GPS Device",
  tags = { "hardware", "gps" },
  body = function()
    local devices = serial.list_devices()

    local gps_path = nil
    for _, dev in ipairs(devices) do
      if dev.product and dev.product:find("GPS") then
        gps_path = dev.path
        break
      end
    end

    if not gps_path then
      ltf.log_critical("GPS device not found!")
    end

    ltf.log_info("Found GPS device at:", gps_path)

    local port = serial.get_port(gps_path)
    ltf.defer(port.close, port)

    port:open("rw")
    port:set_baudrate(9600)
    port:set_bits(8)
    port:set_parity("none")
    port:set_stopbits(1)

    ltf.print("Port configured. Waiting for NMEA sentence...")

    local found, data = port:read_until({
      pattern = "$GPGGA",
      timeout = 5000,
      chunk_size = 64,
    })

    if found then
      ltf.log_info("Received GPGGA sentence:", data)
    else
      ltf.log_error("Did not receive a GPGGA sentence in time. Read:", data)
    end
  end,
})
```

---

## Data structures & types

### `serial_port_info` (table)

Returned by `ltf.serial.list_devices()` and `port:get_port_info()`.

| Field               | Type               | Description                                        |
| ------------------- | ------------------ | -------------------------------------------------- |
| `path`              | `string`           | Path/name of the port (e.g. `COM3`, `/dev/ttyS0`). |
| `type`              | `serial_port_type` | `"native"`, `"usb"`, `"bluetooth"`, `"unknown"`.   |
| `description`       | `string`           | Human-readable description.                        |
| `serial`            | `string?`          | USB serial number (if available).                  |
| `product`           | `string?`          | USB product string (if available).                 |
| `manufacturer`      | `string?`          | USB manufacturer string (if available).            |
| `vid`               | `number?`          | USB vendor ID (if available).                      |
| `pid`               | `number?`          | USB product ID (if available).                     |
| `usb_address`       | `number?`          | USB port address (if available).                   |
| `usb_bus`           | `number?`          | USB bus number (if available).                     |
| `bluetooth_address` | `string?`          | Bluetooth MAC address (if available).              |

### Type aliases

| Alias                    | Accepted values                                  | Description                  |
| ------------------------ | ------------------------------------------------ | ---------------------------- |
| `serial_mode`            | `"r"`, `"w"`, `"rw"`                             | Read / Write / Read+Write.   |
| `serial_flush_direction` | `"i"`, `"o"`, `"io"`                             | Flush input / output / both. |
| `serial_data_bits`       | `5`, `6`, `7`, `8`                               | Data bits.                   |
| `serial_parity`          | `"none"`, `"odd"`, `"even"`, `"mark"`, `"space"` | Parity mode.                 |
| `serial_stop_bits`       | `1`, `2`                                         | Stop bits.                   |
| `serial_flowctrl`        | `"dtrdsr"`, `"rtscts"`, `"xonxoff"`, `"none"`    | Flow control.                |
| `serial_rts`             | `"off"`, `"on"`, `"flowctrl"`                    | RTS behavior.                |
| `serial_dtr`             | `"off"`, `"on"`, `"flowctrl"`                    | DTR behavior.                |
| `serial_cts`             | `"ignore"`, `"flowctrl"`                         | CTS behavior.                |
| `serial_dsr`             | `"ignore"`, `"flowctrl"`                         | DSR behavior.                |
| `serial_xonxoff`         | `"i"`, `"o"`, `"io"`, `"disable"`                | XON/XOFF mode.               |
| `serial_port_type`       | `"native"`, `"usb"`, `"bluetooth"`, `"unknown"`  | Port connection type.        |

---

### Low-level access

#### `ltf.serial.low`

`serial.low` exposes the underlying low-level module (`require("ltf-serial")`). Most users should use the high-level API above.

