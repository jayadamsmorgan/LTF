local ltf = require("ltf")
local check = require("test_checkup")

ltf.test({
	name = "Test module-serial",
	tags = { "module-serial" },
	body = function()
		local log_obj = check.load_log({
			"test",
			"bootstrap",
			"-t",
			"module-serial",
			"-v",
			"any=anyval,enum=value2",
		})

		assert(log_obj.tags ~= nil)
		assert(#log_obj.tags == 1)
		assert(log_obj.tags[1] == "module-serial")

		assert(log_obj.tests ~= nil)
		assert(#log_obj.tests == 7, "Expected 7 tests, got " .. #log_obj.tests)

		local test = log_obj.tests[1]
		check.check_test(test, "Test serial.list_devices", "PASSED")
		check.test_tags(test, { "module-serial" })
		check.error_if(#test.output ~= 3, test, "Outputs not match")
		for _, value in ipairs(test.output) do
			check.check_output(test, value, "", "INFO", true)
			local info = ltf.json.deserialize(value.msg)
			assert(info.type == "native" or info.type == "usb" or info.type == "bluetooth")
			assert(info.description)
			assert(info.path)
			if info.type == "usb" then
				assert(info.usb_address and info.usb_bus)
			end
			if info.type == "bluetooth" then
				assert(info.bluetooth_address)
			end
			if info.type ~= "native" then
				assert(info.vid and info.pid)
				assert(info.product)
				assert(info.serial)
			end
		end

		test = log_obj.tests[2]
		check.check_test(test, "Test serial.get_port (existing)", "PASSED")
		check.test_tags(test, { "module-serial" })

		test = log_obj.tests[3]
		check.check_test(test, "Test serial.get_port (non-existing)", "FAILED")
		check.test_tags(test, { "module-serial" })

		test = log_obj.tests[4]
		check.check_test(test, "Test serial_port:open", "PASSED")
		check.test_tags(test, { "module-serial" })

		test = log_obj.tests[5]
		check.check_test(test, "Test serial communication", "PASSED")
		check.test_tags(test, { "module-serial" })
		check.error_if(#test.output ~= 2, test, "Outputs not match")
		check.check_output(test, test.output[1], "PONG\n", "INFO")
		check.check_output(test, test.output[2], "ping_pong\n", "INFO")
	end,
})
