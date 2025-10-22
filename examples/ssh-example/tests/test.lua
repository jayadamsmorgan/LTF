local ltf = require("ltf")
local ssh = ltf.ssh
local vars = require("variables")

-- Just connect and disconnect from local host with low level API
ltf.test({
	name = "SSH low level connect",
	tags = { "tag1" },
	body = function()
		local res = ssh.low.lib_init()
		print("lib init: ", res)
		local session = ssh.low.session_init()
		print("Session init: ", session)
		local socket, err = ssh.low.socket_init(vars.host_ip, vars.ssh_port)
		print("Socket init: ", socket, err)
		res = session:handshake(socket)
		print("Handshake res: ", res)
		res = session:disconnect("User disconnect")
		print("Disconnect res: ", res)
		res = session:free()
		print("Sessio free res: ", res)
		res = ssh.low.socket_free(socket)
		print("Socket free res: ", res)
	end,
})

-- Connect and execute command via ssh on local host with ssh.lua API
ltf.test({
	name = "SSH connect and disconnect with high level API",
	tags = { "tag2" },
	body = function()
		local conn1 = ssh.create_connection(vars.host_ip, vars.ssh_port, "root", "akytec", 10000)

		ltf.log_info("ip: ", conn1.ip)
		ltf.log_info("port: ", conn1.port)
		ltf.log_info("usr: ", conn1.usr)
		ltf.log_info("pswd: ", conn1.pswd)
		ltf.log_info("session: ", conn1.session)
		ltf.log_info("socket: ", conn1.socket)

		-- local env = {
		-- 	{ var = "PATH", val = "/sbin" },
		-- }
		--
		-- for i, pair in ipairs(env or {}) do
		-- 	ltf.log_info(string.format("[%d] %s=%s", i, pair.var, pair.val))
		-- end

		local stdout, stderr = ssh.execute_cmd(conn1, "env", true, true, 10000)

		ltf.log_info("STDOUT:\n", stdout)
		ltf.log_info("STDERR:\n", stderr)
		ssh.close_connection(conn1)
	end,
})

-- Connect -> Open Shell-> write command -> read and read_error
ltf.test({
	name = "SSH connect and disconnect with high level API",
	tags = { "tag3" },
	body = function()
		local conn1 = ssh.create_connection(vars.host_ip, vars.ssh_port, "root", "akytec", 10000)

		ltf.log_info("ip: ", conn1.ip)
		ltf.log_info("port: ", conn1.port)
		ltf.log_info("usr: ", conn1.usr)
		ltf.log_info("pswd: ", conn1.pswd)
		ltf.log_info("session: ", conn1.session)
		ltf.log_info("socket: ", conn1.socket)

		-- local env = {
		-- 	{ var = "PATH", val = "/sbin" },
		-- }
		--
		-- for i, pair in ipairs(env or {}) do
		-- 	ltf.log_info(string.format("[%d] %s=%s", i, pair.var, pair.val))
		-- end

		local channel1 = ssh.open_shell(conn1, env, 10000, "xterm")

		local ok, err = ssh.shell_write(conn1, channel1, "\n")

		ltf.log_info("Result write", ok, err)

		local ok1, err1 = ssh.shell_read(conn1, channel1, 5000)

		ltf.log_info("Result read: ", ok1, err1)

		ltf.sleep(200)

		local oks, errs = ssh.shell_write(conn1, channel1, "env\n")
		ltf.log_info("Result write", oks, errs)

		local ok2, err2 = ssh.shell_read(conn1, channel1, 1000)

		ltf.log_info("Result read: ", ok2, err2)
		ssh.close_shell(conn1, channel1)
		-- ltf.log_info("STDOUT:\n", stdout)
		-- ltf.log_info("STDERR:\n", stderr)
		ssh.close_connection(conn1)
	end,
})

-- Connect -> Open Shell-> write command -> read and read_error
ltf.test({
	name = "SSH connect and disconnect with high level API",
	tags = { "tag4" },
	body = function()
		local conn1 = ssh.create_connection(vars.host_ip, vars.ssh_port, "root", "akytec", 10000)

		ltf.log_info("ip: ", conn1.ip)
		ltf.log_info("port: ", conn1.port)
		ltf.log_info("usr: ", conn1.usr)
		ltf.log_info("pswd: ", conn1.pswd)
		ltf.log_info("session: ", conn1.session)
		ltf.log_info("socket: ", conn1.socket)

		-- local env = {
		-- 	{ var = "PATH", val = "/sbin" },
		-- }
		--
		-- for i, pair in ipairs(env or {}) do
		-- 	ltf.log_info(string.format("[%d] %s=%s", i, pair.var, pair.val))
		-- end

		local channel1 = ssh.open_shell(conn1, env, 10000, "xterm")

		local ok, err = ssh.shell_write(conn1, channel1, "\n")

		ltf.log_info("Result write", ok, err)

		local ok1, err1 = ssh.shell_read_until(conn1, channel1, 5000, "root")

		ltf.log_info("Result read: ", ok1, err1)

		ltf.sleep(200)

		local oks, errs = ssh.shell_write(conn1, channel1, "env\n")
		ltf.log_info("Result write", oks, errs)

		local ok2, err2 = ssh.shell_read_until(conn1, channel1, 5000, "SHELL", 1)

		ltf.log_info("Result read: ", ok2, err2)
		ssh.close_shell(conn1, channel1)
		-- ltf.log_info("STDOUT:\n", stdout)
		-- ltf.log_info("STDERR:\n", stderr)
		ssh.close_connection(conn1)
	end,
})
