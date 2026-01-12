local ltf = require("ltf")
local serial = ltf.serial

--- @param info serial_port_info
local function log_port_info(info)
	ltf.log_info(ltf.json.serialize(info, { pretty = true }))
end

ltf.test({
	name = "Test serial.list_devices",
	tags = { "module-serial" },
	body = function()
		local devices = serial.list_devices()

		for _, info in ipairs(devices) do
			log_port_info(info)
		end
	end,
})

ltf.test({
	name = "Test serial.get_port (existing)",
	tags = { "module-serial" },
	body = function()
		serial.get_port("/dev/ttyACM0")
	end,
})

ltf.test({
	name = "Test serial.get_port (non-existing)",
	tags = { "module-serial" },
	body = function()
		-- Should throw
		serial.get_port("/dev/unknown")
	end,
})

ltf.test({
	name = "Test serial_port:open",
	tags = { "module-serial" },
	body = function()
		local port = serial.get_port("/dev/ttyACM0")
		port:open("rw")
		ltf.defer(function()
			port:close()
		end)
	end,
})

ltf.test({
	name = "Test serial communication",
	tags = { "module-serial" },
	body = function()
		local port = serial.get_port("/dev/ttyACM0")

		port:open("rw")
		ltf.defer(function()
			port:close()
		end)

		port:set_baudrate(115200)

		port:write("PING\n")

		local found, read = port:read_until({
			chunk_size = 1,
			pattern = "\n",
			timeout = 1000,
		})
		assert(found == true and read == "PONG\n", "PONG not found, read: '" .. tostring(read) .. "'")
		ltf.log_info(read)

		port:write("ping_pong\n")

		found, read = port:read_until({
			chunk_size = 1,
			pattern = "\n",
			timeout = 1000,
		})
		assert(found == true and read == "ping_pong\n", "ping_pong not found, read: '" .. tostring(read) .. "'")
		ltf.log_info(read)
	end,
})

ltf.test({
	name = "Test serial_port:get_info (port closed)",
	tags = { "module-serial" },
	body = function()
		local port = serial.get_port("/dev/ttyACM0")
		local info = port:get_port_info()
		log_port_info(info)
	end,
})

ltf.test({
	name = "Test serial_port:get_info (port opened)",
	tags = { "module-serial" },
	body = function()
		local port = serial.get_port("/dev/ttyACM0")
		port:open("rw")
		ltf.defer(function()
			port:close()
		end)

		local info = port:get_port_info()
		log_port_info(info)
	end,
})
