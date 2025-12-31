local ltf = require("ltf")
local ssh = ltf.ssh

ltf.test({
	name = "Test SSH connection",
	tags = { "module-ssh" },
	body = function()
		local session = ssh.new_session({
			ip = "127.0.0.1",
			port = 22,
			userpass = {
				user = "",
				password = "",
			},
		})

		session:connect()

		session:new_exec_channel()
	end,
})
