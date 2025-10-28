local ltf = require("ltf")
local ssh = ltf.ssh
local vars = require("variables")

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

ltf.test({
	name = "SSH connect and execute command with high level API",
	tags = { "tag2" },
	body = function()
		local conn1 = ssh.create_connection(vars.host_ip, vars.ssh_port, vars.user, vars.password, 10000)

		ltf.log_info("ip: ", conn1.ip)
		ltf.log_info("port: ", conn1.port)
		ltf.log_info("usr: ", conn1.usr)
		ltf.log_info("pswd: ", conn1.pswd)
		ltf.log_info("session: ", conn1.session)
		ltf.log_info("socket: ", conn1.socket)

		local stdout, stderr = ssh.execute_cmd(conn1, "env", true, true, 10000)

		ltf.log_info("STDOUT:\n", stdout)
		ltf.log_info("STDERR:\n", stderr)
		ssh.close_connection(conn1)
	end,
})

ltf.test({
	name = "Connect to SSH server and do write/read  operations in shell mode",
	tags = { "tag3" },
	body = function()
		local conn1 = ssh.create_connection(vars.host_ip, vars.ssh_port, vars.user, vars.password, 10000)

		ltf.log_info("ip: ", conn1.ip)
		ltf.log_info("port: ", conn1.port)
		ltf.log_info("usr: ", conn1.usr)
		ltf.log_info("pswd: ", conn1.pswd)
		ltf.log_info("session: ", conn1.session)
		ltf.log_info("socket: ", conn1.socket)

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
		ssh.close_connection(conn1)
	end,
})

ltf.test({
	name = "Connect to SSH server and do write/read_until  operations in shell mode",
	tags = { "tag4" },
	body = function()
		local conn1 = ssh.create_connection(vars.host_ip, vars.ssh_port, vars.user, vars.password, 10000)

		ltf.log_info("ip: ", conn1.ip)
		ltf.log_info("port: ", conn1.port)
		ltf.log_info("usr: ", conn1.usr)
		ltf.log_info("pswd: ", conn1.pswd)
		ltf.log_info("session: ", conn1.session)
		ltf.log_info("socket: ", conn1.socket)

		local channel1 = ssh.open_shell(conn1, "xterm", 10000)

		local ok, err = ssh.shell_write(conn1, channel1, "\n")

		ltf.log_info("Result write", ok, err)

		local ok1, err1 = ssh.shell_read_until(conn1, channel1, 5000, "root")

		ltf.log_info("Result read: ", ok1, err1)

		ltf.sleep(200)

		local oks, errs = ssh.shell_write(conn1, channel1, "env")
		ltf.log_info("Result write", oks, errs)

		local ok2, err2 = ssh.shell_read_until(conn1, channel1, 5000, "SHELL", 1)

		ltf.log_info("Result read: ", ok2, err2)
		ssh.close_shell(conn1, channel1)
		ssh.close_connection(conn1)
	end,
})

ltf.test({
	name = "Connect to SSH server and send file over sftp",
	tags = { "tag5" },
	body = function()
		local conn1 = ssh.create_connection(vars.host_ip, vars.ssh_port, vars.user, vars.password, 10000)

		ltf.log_info("ip: ", conn1.ip)
		ltf.log_info("port: ", conn1.port)
		ltf.log_info("usr: ", conn1.usr)
		ltf.log_info("pswd: ", conn1.pswd)
		ltf.log_info("session: ", conn1.session)
		ltf.log_info("socket: ", conn1.socket)

		ssh.send_file_sftp(conn1, "/tmp/TEST", "/tmp/test.c", 420)

		ssh.close_connection(conn1)
	end,
})

ltf.test({
	name = "Connect to SSH server and recv file over sftp",
	tags = { "tag6" },
	body = function()
		local conn1 = ssh.create_connection(vars.host_ip, vars.ssh_port, vars.user, vars.password, 10000)

		ltf.log_info("ip: ", conn1.ip)
		ltf.log_info("port: ", conn1.port)
		ltf.log_info("usr: ", conn1.usr)
		ltf.log_info("pswd: ", conn1.pswd)
		ltf.log_info("session: ", conn1.session)
		ltf.log_info("socket: ", conn1.socket)

		ssh.recv_file_sftp(conn1, "/tmp/TEST", "/home/yproshin/RECV_TEST")

		ssh.close_connection(conn1)
	end,
})

ltf.test({
	name = "Connect to SSH server and recv file over sftp with his existence check",
	tags = { "tag7" },
	body = function()
		local conn1 = ssh.create_connection(vars.host_ip, vars.ssh_port, vars.user, vars.password, 10000)

		ltf.log_info("ip: ", conn1.ip)
		ltf.log_info("port: ", conn1.port)
		ltf.log_info("usr: ", conn1.usr)
		ltf.log_info("pswd: ", conn1.pswd)
		ltf.log_info("session: ", conn1.session)
		ltf.log_info("socket: ", conn1.socket)

		ssh.recv_file_sftp(conn1, "/tmp/TEST", "/tmp/RECV_TEST")

		ltf.log_info("file existence: " .. tostring(ssh.file_should_exist(conn1, "/home/yproshin/RECV_TEST")))

		ssh.close_connection(conn1)
	end,
})

ltf.test({
	name = "Connect to SSH server and check remote file existence",
	tags = { "tag8" },
	body = function()
		local conn1 = ssh.create_connection(vars.host_ip, vars.ssh_port, vars.user, vars.password, 10000)

		ltf.log_info("ip: ", conn1.ip)
		ltf.log_info("port: ", conn1.port)
		ltf.log_info("usr: ", conn1.usr)
		ltf.log_info("pswd: ", conn1.pswd)
		ltf.log_info("session: ", conn1.session)
		ltf.log_info("socket: ", conn1.socket)

		ltf.log_info("directory existence: " .. tostring(ssh.directory_should_exist(conn1, "/tmp")))

		ssh.close_connection(conn1)
	end,
})
