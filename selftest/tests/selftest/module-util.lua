local ltf = require("ltf")

local check = require("test_checkup")

ltf.test({
	name = "Test module-util",
	tags = { "module-util" },
	body = function()
		local log_obj = check.load_log({ "test", "bootstrap", "-t", "module-util" })

		assert(log_obj.tags ~= nil)
		assert(#log_obj.tags == 1)
		assert(log_obj.tags[1] == "module-util")

		assert(log_obj.tests ~= nil)
		assert(#log_obj.tests == 6, "Expected 6 tests, got " .. #log_obj.tests)

		local test = log_obj.tests[1]
		check.check_test(test, "Test util.file_info (file)", "PASSED")
		check.test_tags(test, { "module-util", "file-util" })
		check.error_if(#test.output ~= 4, test, "Outputs not match")
		check.check_output(test, test.output[1], "file", "INFO")
		check.check_output(test, test.output[2], "/selftest/.ltf.json", "INFO", true)
		check.check_output(test, test.output[3], "227", "INFO")
		check.check_output(test, test.output[4], "33188", "INFO")

		test = log_obj.tests[2]
		check.check_test(test, "Test util.file_info (directory)", "PASSED")
		check.test_tags(test, { "module-util", "file-util" })
		check.error_if(#test.output ~= 4, test, "Outputs not match")
		check.check_output(test, test.output[1], "directory", "INFO")
		check.check_output(test, test.output[2], "/selftest/tests", "INFO", true)
		check.check_output(test, test.output[3], "4096", "INFO")
		check.check_output(test, test.output[4], "16877", "INFO")

		test = log_obj.tests[3]
		check.check_test(test, "Test util.file_info (symlink)", "PASSED")
		check.test_tags(test, { "module-util", "file-util" })
		check.error_if(#test.output ~= 4, test, "Outputs not match")
		check.check_output(test, test.output[1], "symlink", "INFO")
		check.check_output(test, test.output[2], "/selftest/logs/test_run_latest_raw.json", "INFO", true)
		check.check_output(test, test.output[3], "93", "INFO")
		check.check_output(test, test.output[4], "41471", "INFO")

		test = log_obj.tests[4]
		check.check_test(test, "Test util.file_info (no file present)", "PASSED")
		check.test_tags(test, { "module-util", "file-util" })
		check.error_if(#test.output ~= 0, test, "Outputs not match")

		test = log_obj.tests[5]
		check.check_test(test, "Test util.resolve_symlink", "PASSED")
		check.test_tags(test, { "module-util", "file-util" })
		check.error_if(#test.output ~= 1, test, "Outputs not match")
		check.check_output(test, test.output[1], "logs/bootstrap/test_run", "INFO", true)

		test = log_obj.tests[6]
		check.check_test(test, "Test util.resolve_symlink (no file present)", "FAILED")
		check.test_tags(test, { "module-util", "file-util" })
		check.error_if(#test.output ~= 0, test, "Outputs not match")
	end,
})
