local ltf = require("ltf")

local check = require("test_checkup")

ltf.test({
	name = "Test module-ltf (common)",
	tags = { "module-ltf", "common" },
	body = function()
		local log_obj = check.load_log({
			"test",
			"bootstrap",
			"-t",
			"common",
			"-v",
			"any=anyval,enum=value2",
		})

		assert(log_obj.tags ~= nil)
		assert(#log_obj.tags == 1)
		assert(log_obj.tags[1] == "common")

		assert(log_obj.tests ~= nil)
		assert(#log_obj.tests == 1, "Expected 1 test, got " .. #log_obj.tests)

		local test = log_obj.tests[1]

		check.check_test(test, "Test common LTF test", "PASSED")
		check.test_tags(test, { "common" })
		check.error_if(#test.output ~= 1, test, "Outputs not match")
		if #test.output == 1 then
			check.check_output(test, test.output[1], "common", "INFO")
		end
	end,
})

ltf.test({
	name = "Test module-ltf (logging)",
	tags = { "module-ltf", "logging" },
	body = function()
		local log_obj = check.load_log({
			"test",
			"bootstrap",
			"-t",
			"logging",
			"-v",
			"any=anyval,enum=value2",
		})

		assert(log_obj.tags ~= nil)
		assert(#log_obj.tags == 1)
		assert(log_obj.tags[1] == "logging")

		assert(log_obj.tests ~= nil)
		assert(#log_obj.tests == 13, "Expected 13 tests, got " .. #log_obj.tests)

		--- @param log_test test_t
		--- @param log_level log_level_t
		--- @param amount integer
		--- @param status status_t
		--- @param additional string?
		local function test_logging_tests(log_test, log_level, amount, status, additional)
			local add = additional or ""
			check.check_test(log_test, ("Test logging with '%s' log level%s"):format(log_level:lower(), add), status)
			check.test_tags(log_test, { "module-ltf", "logging" })
			check.error_if(#log_test.output ~= amount, log_test, "Outputs not match")
			local str_to_find = ("Testing logging with %s log level."):format(log_level)
			if status == "FAILED" then
				check.error_if(#log_test.failure_reasons ~= amount, log_test, "Outputs not match")
			end
			for i = 1, amount do
				check.check_output(log_test, log_test.output[i], str_to_find, log_level)
				if status == "FAILED" then
					local contains = log_level == "CRITICAL"
					-- When log level is CRITICAL failure reason will have a traceback as a message,
					-- hence we are trying to find message inside traceback with `contains`
					check.check_output(log_test, log_test.failure_reasons[i], str_to_find, log_level, contains)
					if contains then
						-- Make sure traceback is there also
						-- We just shouldn't log anything with this word in bootstrap
						check.error_if(
							log_test.failure_reasons[i].msg:find("stack traceback:") == nil,
							log_test,
							"Unable to find traceback"
						)
					end
				end
			end
		end

		test_logging_tests(log_obj.tests[1], "TRACE", 5, "PASSED")
		test_logging_tests(log_obj.tests[2], "DEBUG", 5, "PASSED")
		test_logging_tests(log_obj.tests[3], "INFO", 5, "PASSED")
		test_logging_tests(log_obj.tests[4], "WARNING", 5, "PASSED")
		test_logging_tests(log_obj.tests[5], "ERROR", 5, "FAILED")
		test_logging_tests(log_obj.tests[6], "CRITICAL", 1, "FAILED", " ('CRITICAL')")
		test_logging_tests(log_obj.tests[7], "CRITICAL", 1, "FAILED", " ('critical')")
		test_logging_tests(log_obj.tests[8], "CRITICAL", 1, "FAILED", " ('C')")
		test_logging_tests(log_obj.tests[9], "CRITICAL", 1, "FAILED", " ('c')")
		test_logging_tests(log_obj.tests[10], "CRITICAL", 1, "FAILED", " ('ltf.log_critical')")

		local test = log_obj.tests[11]
		check.check_test(test, "Test logging with incorrect log level", "FAILED")
		check.test_tags(test, { "module-ltf", "logging" })
		check.error_if(#test.output ~= 0, test, "Outputs not match")
		check.error_if(#test.failure_reasons ~= 1, test, "Outputs not match")
		check.check_output(test, test.failure_reasons[1], "Unknown log level 'incorrect'", "CRITICAL", true)
		check.error_if(test.failure_reasons[1].msg:find("stack traceback:") == nil, test, "Unable to find traceback")

		test = log_obj.tests[12]
		check.check_test(test, "Test logging with multiple arguments", "PASSED")
		check.test_tags(test, { "module-ltf", "logging" })
		check.error_if(#test.output ~= 1, test, "Outputs not match")
		check.check_output(test, test.output[1], "test1\ttest2\ttest3\ttest4 test5", "INFO")

		test = log_obj.tests[13]
		check.check_test(test, "Test Lua's 'print' is forwarded to logging with INFO log level", "PASSED")
		check.test_tags(test, { "module-ltf", "logging" })
		check.error_if(#test.output ~= 2, test, "Outputs not match")
		for i = 1, 2 do
			check.check_output(test, test.output[i], "Testing logging", "INFO")
		end
	end,
})

ltf.test({
	name = "Test module-ltf (utils)",
	tags = { "module-ltf", "utils" },
	body = function()
		local log_obj = check.load_log({
			"test",
			"bootstrap",
			"-t",
			"utils,other-tag,some-other-tag",
			"-v",
			"any=anyval,enum=value1",
		})

		assert(log_obj.tags ~= nil)
		assert(#log_obj.tags == 3)
		assert(log_obj.tags[1] == "utils")
		assert(log_obj.tags[2] == "other-tag")
		assert(log_obj.tags[3] == "some-other-tag")

		assert(log_obj.tests ~= nil)
		assert(#log_obj.tests == 15, "Expected 15 tests, got " .. #log_obj.tests)

		local test = log_obj.tests[1]
		check.check_test(test, "Test ltf.sleep", "PASSED")
		check.test_tags(test, { "module-ltf", "utils" })
		check.error_if(test.finished == test.started, test, "No sleep")

		test = log_obj.tests[2]
		check.check_test(test, "Test ltf.get_current_target", "PASSED")
		check.test_tags(test, { "module-ltf", "utils" })
		check.error_if(#test.output ~= 1, test, "Outputs not match")
		check.check_output(test, test.output[1], "bootstrap", "INFO")

		test = log_obj.tests[3]
		check.check_test(test, "Test ltf.defer", "PASSED")
		check.test_tags(test, { "module-ltf", "utils" })
		check.error_if(#test.output ~= 0, test, "Outputs not match")
		check.error_if(#test.teardown_output ~= 5, test, "Outputs not match")
		local j = 1
		for i = 5, 1, -1 do
			-- We check from last to first, so `j` should go up
			check.check_output(test, test.teardown_output[i], "defer " .. j, "INFO")
			j = j + 1
		end
		check.error_if(#test.teardown_errors ~= 1, test, "Outputs not match")
		check.check_output(test, test.teardown_errors[1], "assertion failed", "CRITICAL", true)

		test = log_obj.tests[4]
		check.check_test(test, "Test ltf.millis", "PASSED")
		check.test_tags(test, { "module-ltf", "utils" })
		check.error_if(#test.output ~= 2, test, "Outputs not match")
		check.check_output(test, test.output[1], nil, "INFO")
		check.check_output(test, test.output[2], nil, "INFO")
		local start_ms = tonumber(test.output[1].msg)
		check.error_if(start_ms == nil, test, "start_ms is nil")
		local end_ms = tonumber(test.output[2].msg)
		check.error_if(end_ms == nil, test, "end_ms is nil")
		if start_ms and end_ms then
			local time = end_ms - start_ms
			check.error_if(time < 10 or time > 200, test, "Incorrect millis " .. time)
		end

		test = log_obj.tests[5]
		check.check_test(test, "Test ltf.get_active_tags", "PASSED")
		check.test_tags(test, { "module-ltf", "utils" })
		check.error_if(#test.output ~= 3, test, "Outputs not match")
		check.check_output(test, test.output[1], "utils", "INFO")
		check.check_output(test, test.output[2], "other-tag", "INFO")
		check.check_output(test, test.output[3], "some-other-tag", "INFO")

		test = log_obj.tests[6]
		check.check_test(test, "Test ltf.get_active_test_tags", "PASSED")
		check.test_tags(test, { "module-ltf", "utils", "some-other-tag" })
		check.error_if(#test.output ~= 2, test, "Outputs not match")
		check.check_output(test, test.output[1], "utils", "INFO")
		check.check_output(test, test.output[2], "some-other-tag", "INFO")

		test = log_obj.tests[7]
		check.check_test(test, "Test description", "PASSED")
		check.test_tags(test, { "module-ltf", "utils" })
		check.error_if(#test.output ~= 0, test, "Outputs not match")
		check.error_if(test.description ~= "This is a test description", test, "Description mismatch")

		test = log_obj.tests[8]
		check.check_test(test, "Test ltf.get_secret", "PASSED")
		check.test_tags(test, { "module-ltf", "utils" })
		check.error_if(#test.output ~= 3, test, "Outputs not match")
		check.check_output(test, test.output[1], "basic", "INFO")
		check.check_output(test, test.output[2], "multi\nline\nsecret", "INFO")
		check.check_output(test, test.output[3], '"quoted"', "INFO")

		test = log_obj.tests[9]
		check.check_test(test, "Test ltf.get_secrets", "PASSED")
		check.test_tags(test, { "module-ltf", "utils" })
		check.error_if(#test.output ~= 3, test, "Outputs not match")
		check.check_output(test, test.output[1], "basic_secret:basic", "INFO")
		check.check_output(test, test.output[2], "multiline_secret:multi\nline\nsecret", "INFO")
		check.check_output(test, test.output[3], 'quoted_secret:"quoted"', "INFO")

		test = log_obj.tests[10]
		check.check_test(test, "Test ltf.get_var", "PASSED")
		check.test_tags(test, { "module-ltf", "utils" })
		check.error_if(#test.output ~= 5, test, "Outputs not match")
		check.check_output(test, test.output[1], "just a simple variable", "INFO")
		check.check_output(test, test.output[2], "anyval", "INFO")
		check.check_output(test, test.output[3], "variable with a default value", "INFO")
		check.check_output(test, test.output[4], "value1", "INFO")
		check.check_output(test, test.output[5], "value2", "INFO")

		test = log_obj.tests[11]
		check.check_test(test, "Test ltf.get_var (unknown)", "FAILED")
		check.test_tags(test, { "module-ltf", "utils" })
		check.error_if(#test.output ~= 0, test, "Outputs not match")
		check.error_if(#test.failure_reasons ~= 1, test, "Outputs not match")
		check.check_output(test, test.failure_reasons[1], "No variable 'unknown'", "CRITICAL", true)

		test = log_obj.tests[12]
		check.check_test(test, "Test ltf.get_var_number", "PASSED")
		check.test_tags(test, { "module-ltf", "utils" })
		check.error_if(#test.output ~= 1, test, "Outputs not match")
		check.check_output(test, test.output[1], "3.14", "INFO")

		test = log_obj.tests[13]
		check.check_test(test, "Test ltf.get_var_number (non-number)", "FAILED")
		check.test_tags(test, { "module-ltf", "utils" })
		check.error_if(#test.output ~= 0, test, "Outputs not match")
		check.error_if(#test.failure_reasons ~= 1, test, "Outputs not match")
		check.check_output(test, test.failure_reasons[1], "Variable 'constant' is not a number", "CRITICAL", true)

		test = log_obj.tests[14]
		check.check_test(test, "Test ltf.get_var_number (unknown)", "FAILED")
		check.test_tags(test, { "module-ltf", "utils" })
		check.error_if(#test.output ~= 0, test, "Outputs not match")
		check.error_if(#test.failure_reasons ~= 1, test, "Outputs not match")
		check.check_output(test, test.failure_reasons[1], "No variable 'unknown'", "CRITICAL", true)

		test = log_obj.tests[15]
		check.check_test(test, "Test ltf.get_vars", "PASSED")
		check.test_tags(test, { "module-ltf", "utils" })
		check.error_if(#test.output ~= 6, test, "Outputs not match")
		check.check_output(test, test.output[1], "constant:just a simple variable", "INFO")
		check.check_output(test, test.output[2], "any:anyval", "INFO")
		check.check_output(test, test.output[3], "any_default:variable with a default value", "INFO")
		check.check_output(test, test.output[4], "enum:value1", "INFO")
		check.check_output(test, test.output[5], "enum_default:value2", "INFO")
		check.check_output(test, test.output[6], "number:3.14", "INFO")
	end,
})
