local ltf = require("ltf")

ltf.test({
	name = "Test logging with 'trace' log level",
	tags = { "module-ltf", "logging" },
	body = function()
		local str = "Testing logging with TRACE log level."
		ltf.log("TRACE", str)
		ltf.log("trace", str)
		ltf.log("T", str)
		ltf.log("t", str)
		ltf.log_trace(str)
	end,
})

ltf.test({
	name = "Test logging with 'debug' log level",
	tags = { "module-ltf", "logging" },
	body = function()
		local str = "Testing logging with DEBUG log level."
		ltf.log("DEBUG", str)
		ltf.log("debug", str)
		ltf.log("D", str)
		ltf.log("d", str)
		ltf.log_debug(str)
	end,
})

ltf.test({
	name = "Test logging with 'info' log level",
	tags = { "module-ltf", "logging" },
	body = function()
		local str = "Testing logging with INFO log level."
		ltf.log("INFO", str)
		ltf.log("info", str)
		ltf.log("I", str)
		ltf.log("i", str)
		ltf.log_info(str)
	end,
})

ltf.test({
	name = "Test logging with 'warning' log level",
	tags = { "module-ltf", "logging" },
	body = function()
		local str = "Testing logging with WARNING log level."
		ltf.log("WARNING", str)
		ltf.log("warning", str)
		ltf.log("W", str)
		ltf.log("w", str)
		ltf.log_warning(str)
	end,
})

ltf.test({
	name = "Test logging with 'error' log level",
	tags = { "module-ltf", "logging" },
	body = function()
		-- Test should fail but execute everything
		local str = "Testing logging with ERROR log level."
		ltf.log("ERROR", str)
		ltf.log("error", str)
		ltf.log("E", str)
		ltf.log("e", str)
		ltf.log_error(str)
	end,
})

-- Tests for critical log level are separate because this level stops test execution
ltf.test({
	name = "Test logging with 'critical' log level ('CRITICAL')",
	tags = { "module-ltf", "logging" },
	body = function()
		local str = "Testing logging with CRITICAL log level."
		ltf.log("CRITICAL", str)
	end,
})

ltf.test({
	name = "Test logging with 'critical' log level ('critical')",
	tags = { "module-ltf", "logging" },
	body = function()
		local str = "Testing logging with CRITICAL log level."
		ltf.log("critical", str)
	end,
})

ltf.test({
	name = "Test logging with 'critical' log level ('C')",
	tags = { "module-ltf", "logging" },
	body = function()
		local str = "Testing logging with CRITICAL log level."
		ltf.log("C", str)
	end,
})

ltf.test({
	name = "Test logging with 'critical' log level ('c')",
	tags = { "module-ltf", "logging" },
	body = function()
		local str = "Testing logging with CRITICAL log level."
		ltf.log("c", str)
	end,
})

ltf.test({
	name = "Test logging with 'critical' log level ('ltf.log_critical')",
	tags = { "module-ltf", "logging" },
	body = function()
		local str = "Testing logging with CRITICAL log level."
		ltf.log_critical(str)
	end,
})

ltf.test({
	name = "Test logging with incorrect log level",
	tags = { "module-ltf", "logging" },
	body = function()
		-- This test should throw on the next line:
		--- @diagnostic disable-next-line: param-type-mismatch
		ltf.log("incorrect", "Testing logging with incorrect log level.")
	end,
})

ltf.test({
	name = "Test logging with multiple arguments",
	tags = { "module-ltf", "logging" },
	body = function()
		ltf.log_info("test1", "test2", "test3", "test4 test5")
	end,
})

ltf.test({
	name = "Test Lua's 'print' is forwarded to logging with INFO log level",
	tags = { "module-ltf", "logging" },
	body = function()
		print("Testing logging")
		ltf.print("Testing logging")
	end,
})

ltf.test({
	name = "Test ltf.sleep",
	tags = { "module-ltf", "utils" },
	body = function()
		ltf.sleep(1000)
	end,
})

ltf.test({
	name = "Test ltf.get_current_target",
	tags = { "module-ltf", "utils" },
	body = function()
		local target = ltf.get_current_target()
		ltf.log("INFO", target)
		assert(target == "bootstrap")
	end,
})

ltf.test({
	name = "Test ltf.defer",
	tags = { "module-ltf", "utils" },
	body = function()
		-- Simple
		ltf.defer(function()
			ltf.log_info("defer 1")
		end)

		-- Status argument
		ltf.defer(function(status)
			ltf.log_info("defer 2")
			assert(status == "passed")
		end)

		-- Failing inside of defer
		ltf.defer(function()
			ltf.log_info("defer 3")
			assert(nil) -- Should fail here
		end)

		-- One more to be sure :)
		ltf.defer(function()
			ltf.log_info("defer 4")
		end)

		-- The alternative syntax
		ltf.defer(ltf.log_info, "defer 5")
	end,
})

ltf.test({
	name = "Test ltf.millis",
	tags = { "module-ltf", "utils" },
	body = function()
		ltf.log_info(ltf.millis())
		ltf.sleep(10)
		ltf.log_info(ltf.millis())
	end,
})

ltf.test({
	name = "Test ltf.get_active_tags",
	tags = { "module-ltf", "utils" },
	body = function()
		local tags = ltf.get_active_tags()
		for _, value in ipairs(tags) do
			ltf.log_info(value)
		end
	end,
})

ltf.test({
	name = "Test ltf.get_active_test_tags",
	tags = { "module-ltf", "utils", "some-other-tag" },
	body = function()
		local tags = ltf.get_active_test_tags()
		for _, value in ipairs(tags) do
			ltf.log_info(value)
		end
	end,
})

ltf.test({
	name = "Test description",
	description = "This is a test description",
	tags = { "module-ltf", "utils" },
	body = function()
		--
	end,
})

ltf.test({
	name = "Test ltf.get_secret",
	tags = { "module-ltf", "utils" },
	body = function()
		ltf.log_info(ltf.get_secret("basic_secret"))
		ltf.log_info(ltf.get_secret("multiline_secret"))
		ltf.log_info(ltf.get_secret("quoted_secret"))
	end,
})

ltf.test({
	name = "Test ltf.get_secrets",
	tags = { "module-ltf", "utils" },
	body = function()
		local secrets = ltf.get_secrets()
		local order = { "basic_secret", "multiline_secret", "quoted_secret" }
		for _, name in ipairs(order) do
			ltf.log_info(name .. ":" .. secrets[name])
		end
	end,
})

ltf.test({
	name = "Test ltf.get_var",
	tags = { "module-ltf", "utils" },
	body = function()
		local constant = ltf.get_var("constant")
		ltf.log_info(constant)
		local any = ltf.get_var("any")
		ltf.log_info(any)
		local any_default = ltf.get_var("any_default")
		ltf.log_info(any_default)
		local enum = ltf.get_var("enum")
		ltf.log_info(enum)
		local enum_default = ltf.get_var("enum_default")
		ltf.log_info(enum_default)
	end,
})

ltf.test({
	name = "Test ltf.get_var (unknown)",
	tags = { "module-ltf", "utils" },
	body = function()
		ltf.get_var("unknown")
	end,
})

ltf.test({
	name = "Test ltf.get_var_number",
	tags = { "module-ltf", "utils" },
	body = function()
		local number = ltf.get_var_number("number")
		assert(type(number) == "number")
		ltf.log_info(number)
	end,
})

ltf.test({
	name = "Test ltf.get_var_number (non-number)",
	tags = { "module-ltf", "utils" },
	body = function()
		ltf.get_var_number("constant")
	end,
})

ltf.test({
	name = "Test ltf.get_var_number (unknown)",
	tags = { "module-ltf", "utils" },
	body = function()
		ltf.get_var_number("unknown")
	end,
})

ltf.test({
	name = "Test ltf.get_vars",
	tags = { "module-ltf", "utils" },
	body = function()
		local vars = ltf.get_vars()

		local order = { "constant", "any", "any_default", "enum", "enum_default", "number" }
		for _, name in ipairs(order) do
			ltf.print(name .. ":" .. vars[name])
		end
	end,
})
