local ltf = require("ltf")

local check = require("test_checkup")

ltf.test({
	name = "Test module-webdriver",
	tags = { "module-webdriver" },
	body = function()
		local log_obj = check.load_log({
			"test",
			"bootstrap",
			"-t",
			"module-webdriver",
			"-v",
			"any=anyval,enum=value2",
		})

		assert(log_obj.tags ~= nil)
		assert(#log_obj.tags == 1)
		assert(log_obj.tags[1] == "module-webdriver")

		assert(log_obj.tests ~= nil)
		assert(#log_obj.tests == 1, "Expected 1 test, got " .. #log_obj.tests)

		local test = log_obj.tests[1]
		check.check_test(test, "Test minimal webdriver possibilities", "PASSED")
		check.test_tags(test, { "module-webdriver" })
		check.error_if(#test.output ~= 2, test, "Outputs not match")
		check.check_output(test, test.output[1], "https://github.com/", "INFO")
		check.check_output(test, test.output[2], "GitHub", "INFO", true)
	end,
})
