local ltf = require("ltf")
local proc = ltf.proc

ltf.test({
	name = "Test proc.run() existing binary",
	tags = { "module-proc" },
	body = function()
		local handle = proc.run({
			exe = "ls",
			args = { "-a" },
		})
		ltf.log_info(handle.exitcode)
		ltf.log_info(handle.stderr)
		ltf.log_info(handle.stdout)
	end,
})

ltf.test({
	name = "Test proc.run() non-existent binary",
	tags = { "module-proc" },
	body = function()
		-- Should throw
		proc.run({
			exe = "some_binary",
			args = {
				"some",
				"arguments",
			},
		})
	end,
})

ltf.test({
	name = "Test proc.run() with timeout not triggered",
	tags = { "module-proc" },
	body = function()
		-- Timeout is 1000ms and we are sleeping for 500ms
		local handle = proc.run({
			exe = "sleep",
			args = { "0.5" },
		}, 1000)
		ltf.log_info(handle.exitcode)
		ltf.log_info(handle.stderr)
		ltf.log_info(handle.stdout)
	end,
})

ltf.test({
	name = "Test proc.run() with timeout triggered",
	tags = { "module-proc" },
	body = function()
		-- Should throw (timeout is 250ms and we are sleeping for 500ms)
		proc.run({
			exe = "sleep",
			args = { "0.5" },
		}, 250)
	end,
})

ltf.test({
	name = "Test proc:wait()",
	tags = { "module-proc" },
	body = function()
		local handle = proc.spawn({
			exe = "sleep",
			args = { "0.5" },
		})
		ltf.defer(function()
			handle:kill()
		end)
		local status = handle:wait()
		ltf.log_info(status)
		ltf.sleep(1000)
		status = handle:wait()
		ltf.log_info(status)
	end,
})

ltf.test({
	name = "Test proc.spawn() with non-existent binary",
	tags = { "module-proc" },
	body = function()
		-- Should throw
		proc.spawn({
			exe = "some_binary",
			args = { "some", "arguments" },
		})
	end,
})

ltf.test({
	name = "Test proc:read() stdio",
	tags = { "module-proc" },
	body = function()
		local handle = proc.spawn({
			exe = "echo",
			args = { "hello" },
		})
		ltf.defer(function()
			handle:kill()
		end)
		ltf.sleep(100) -- just to make sure
		local result = handle:read()
		ltf.log_info(result)
	end,
})

ltf.test({
	name = "Test proc:read() stderr",
	tags = { "module-proc" },
	body = function()
		local handle = proc.spawn({
			exe = "sleep",
			args = { "some_argument" },
		})
		ltf.defer(function()
			handle:kill()
		end)
		ltf.sleep(100) -- just to make sure
		local result = handle:read("stderr")
		ltf.log_info(result)
	end,
})
