local ltf = require("ltf")

local check = require("test_checkup")

ltf.test({
	name = "Test module-http",
	tags = { "module-http" },
	body = function()
		local log_obj = check.load_log({
			"test",
			"bootstrap",
			"-t",
			"module-http",
			"-v",
			"any=anyval,enum=value2",
		})

		assert(log_obj.tags ~= nil)
		assert(#log_obj.tags == 1)
		assert(log_obj.tags[1] == "module-http")

		assert(log_obj.tests ~= nil)
		assert(#log_obj.tests == 4, "Expecteed 4 tests, got")

		local test = log_obj.tests[1]
		check.check_test(test, "Test HTTP POST request", "PASSED")
		check.test_tags(test, { "module-http" })
		check.error_if(#test.output ~= 1, test, "Outputs not match")
		check.check_output(test, test.output[1], '"form": {\n    "Hello from Lua!": ""\n  },', "INFO", true)

		test = log_obj.tests[2]
		check.check_test(test, "Test HTTP GET request", "PASSED")
		check.test_tags(test, { "module-http" })
		check.error_if(#test.output ~= 1, test, "Outputs not match")
		check.check_output(test, test.output[1], '"args": {},', "INFO", true)
		check.check_output(test, test.output[1], '"url": "https://httpbin.org/get"', "INFO", true)

		test = log_obj.tests[3]
		check.check_test(test, "Test HTTP DELETE request", "PASSED")
		check.test_tags(test, { "module-http" })
		check.error_if(#test.output ~= 1, test, "Outputs not match")
		check.check_output(test, test.output[1], '"args": {},', "INFO", true)

		test = log_obj.tests[4]
		check.check_test(test, "Test HTTP PUT request", "PASSED")
		check.test_tags(test, { "module-http" })
		check.error_if(#test.output ~= 1, test, "Outputs not match")
		check.check_output(test, test.output[1], '"args": {},', "INFO", true)
	end,
})
