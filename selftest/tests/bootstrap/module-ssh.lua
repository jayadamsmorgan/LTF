local ltf = require("ltf")
local ssh = ltf.ssh

local function new_session()
	local session = ssh.new_session({
		ip = "127.0.0.1",
		port = 22,
		userpass = {
			user = "ssh_test_user",
			password = "1234",
		},
	})
	ltf.defer(function()
		session:close()
	end)
	session:connect()

	return session
end

ltf.test({
	name = "Test SSH exec channel (success)",
	tags = { "module-ssh" },
	body = function()
		local session = new_session()

		local channel = session:new_exec_channel()
		ltf.defer(function()
			channel:close()
		end)

		local result = channel:exec({
			cmd = "/bin/ls -la",
		})
		ltf.log_info(result.exitcode)
		ltf.log_info(result.stdout)
		ltf.log_info(result.stderr)
	end,
})

ltf.test({
	name = "Test SSH exec channel (failure)",
	tags = { "module-ssh" },
	body = function()
		local session = new_session()

		local channel = session:new_exec_channel()
		ltf.defer(function()
			channel:close()
		end)

		local result = channel:exec({
			cmd = "unknown_cmd",
		})
		ltf.log_info(result.exitcode)
		ltf.log_info(result.stdout)
		ltf.log_info(result.stderr)
	end,
})

ltf.test({
	name = "Test SSH exec channel (env)",
	tags = { "module-ssh" },
	body = function()
		local session = new_session()

		local channel = session:new_exec_channel()
		ltf.defer(function()
			channel:close()
		end)

		local result = channel:exec({
			cmd = "/bin/echo $TEST",
			env = {
				TEST = "Testing!",
			},
		})
		ltf.log_info(result.exitcode)
		ltf.log_info(result.stdout)
		ltf.log_info(result.stderr)
	end,
})

ltf.test({
	name = "Test SSH shell channel (success)",
	tags = { "module-ssh" },
	body = function()
		local session = new_session()

		local channel = session:new_shell_channel()
		ltf.defer(function()
			channel:close()
		end)

		channel:write("/bin/ls -la\n")
		local found, result = channel:read_until({
			timeout = 200,
			pattern = "test.txt",
			read_opts = {
				stream = "stdout",
			},
		})

		ltf.log_info(found)
		ltf.log_info(result)
	end,
})
