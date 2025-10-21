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
		local conn1 = ssh.create_connection(vars.host_ip, vars.ssh_port, "root", "albacore")

		ltf.log_info("ip: ", conn1.ip)
		ltf.log_info("port: ", conn1.port)
		ltf.log_info("usr: ", conn1.usr)
		ltf.log_info("pswd: ", conn1.pswd)
		ltf.log_info("session: ", conn1.session)
		ltf.log_info("socket: ", conn1.socket)

		local stdout, stderr = ssh.execute_cmd(conn1, "lss", true, true)

		ltf.log_info("STDOUT:\n", stdout)
		ltf.log_info("STDERR:\n", stderr)
		ssh.close_connection(conn1)
	end,
})
