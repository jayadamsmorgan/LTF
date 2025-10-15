local ltf = require("ltf")
local ssh = ltf.ssh
local vars = require("variables")

ltf.test({
	name = "SSH low level connect",
	tags = { "tag1", "tag2" },
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
	name = "SSH low level connect",
	tags = { "tag1", "tag2" },
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
