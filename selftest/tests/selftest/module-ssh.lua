local ltf = require("ltf")

local check = require("test_checkup")

ltf.test({
	name = "Test module-ssh",
	tags = { "module-ssh" },
	body = function()
		local log_obj = check.load_log({
			"test",
			"bootstrap",
			"-t",
			"module-ssh",
			"-v",
			"any=anyval,enum=value2",
		})

		assert(log_obj.tags ~= nil)
		assert(#log_obj.tags == 1)
		assert(log_obj.tags[1] == "module-ssh")

		assert(log_obj.tests ~= nil)
		assert(#log_obj.tests == 4, "Expected 4 tests, got " .. #log_obj.tests)

		local test = log_obj.tests[1]
		check.check_test(test, "Test SSH exec channel (success)", "PASSED")
		check.test_tags(test, { "module-ssh" })
		check.error_if(#test.output ~= 3, test, "Outputs not match")
		check.check_output(test, test.output[1], "0", "INFO")
		check.check_output(test, test.output[2], "test.txt", "INFO", true)
		check.check_output(test, test.output[3], "", "INFO")

		test = log_obj.tests[2]
		check.check_test(test, "Test SSH exec channel (failure)", "PASSED")
		check.test_tags(test, { "module-ssh" })
		check.error_if(#test.output ~= 3, test, "Outputs not match")
		check.check_output(test, test.output[1], "127", "INFO")
		check.check_output(test, test.output[2], "", "INFO")
		check.check_output(test, test.output[3], "not found", "INFO", true)

		test = log_obj.tests[3]
		check.check_test(test, "Test SSH shell channel", "PASSED")
		check.test_tags(test, { "module-ssh" })
		check.error_if(#test.output ~= 2, test, "Outputs not match")
		check.check_output(test, test.output[1], "true", "INFO")
		check.check_output(test, test.output[2], "test.txt", "INFO", true)

		test = log_obj.tests[4]
		check.check_test(test, "Test SFTP file transfer", "PASSED")
		check.test_tags(test, { "module-ssh" })
		check.error_if(#test.output ~= 4, test, "Outputs not match")
		check.check_output(test, test.output[1], "file", "INFO")
		check.check_output(test, test.output[2], "/home/ssh_test_user/test.txt", "INFO")
		check.check_output(test, test.output[3], "33204", "INFO")
		check.check_output(test, test.output[4], "30", "INFO")

		local fi = ltf.util.file_info("../README.md.copy")
		assert(fi)
		local r_size = fi.size
		fi = ltf.util.file_info("../README.md")
		assert(fi)
		local l_size = fi.size
		assert(l_size == r_size)
	end,
})
