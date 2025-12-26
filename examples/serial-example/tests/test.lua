local ltf = require("ltf")
local serial = ltf.serial

local port_name = ltf.get_var("port_name")
local baudrate = ltf.get_var_number("baudrate")
local bits = ltf.get_var_number("bits")
local parity = ltf.get_var("parity")
local stopbits = ltf.get_var_number("stopbits")

ltf.test({
	name = "List serial devices",
	tags = { "list" },
	body = function()
		local devices = serial.list_devices()
		for i, device in ipairs(devices) do
			print("[" .. i .. "]: Found device: ", device.path, device.type, device.description)
		end
	end,
})

ltf.test({
	name = "Get port info",
	tags = { "info" },
	body = function()
		local port = serial.get_port(port_name)
		local info = port:get_port_info()
		print("Found device: ", info.path, info.type, info.description)
	end,
})

ltf.test({
	name = "Reading from serial device",
	tags = { "read" },
	body = function()
		local port = serial.get_port(port_name)

		port:open("rw")
		ltf.defer(function()
			port:close()
		end)

		port:set_baudrate(baudrate)
		port:set_bits(bits)
		port:set_parity(parity)
		port:set_stopbits(stopbits)

		local result = port:read_until({
			pattern = "Hello",
			timeout = 15000,
		})
		print(result)
	end,
})
