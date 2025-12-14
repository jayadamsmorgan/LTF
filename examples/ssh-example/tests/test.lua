local ltf = require("ltf")
local ssh = ltf.ssh
local vars = require("variables")

ltf.test({
	name = "SSH connection test",
	body = function()
		local session = ssh.new_session({
			ip = vars.ip,
			port = vars.port,
			userpass = {
				user = vars.user,
				password = vars.password,
			},
		})

		session:connect()
		ltf.defer(function()
			session:close()
		end)
		ltf.log_info("Successfully opened new SSH session")

		local exec_channel = session:new_exec_channel()
		ltf.defer(function()
			exec_channel:close()
		end)
		local result = exec_channel:exec({
			cmd = "ls -la",
		})
		print("stdout: " .. result.stdout)
		print("stderr: " .. result.stderr)
		print("exit_code: " .. result.exit_code)

		local shell_channel = session:new_shell_channel({
			terminal = "xterm",
		})
		ltf.defer(function()
			shell_channel:close()
		end)
		ltf.log_info("Successfully opened new SSH channel")
		--
		-- shell_channel:write("ls -la\n")
		-- local found, read = shell_channel:read_until({
		-- 	pattern = "some_dir",
		-- 	timeout = 5000,
		-- 	read_opts = {
		-- 		chunk_size = 3,
		-- 		stream = "stdout",
		-- 	},
		-- })
		-- if found then
		-- 	ltf.log_info("Found some_dir!")
		-- else
		-- 	ltf.log_info("some_dir not found...")
		-- end
		-- ltf.log_info("Read string: " .. read)
	end,
})

ltf.test({
	name = "SSH SFTP file transfer",
	body = function()
		local session = ssh.new_session({
			ip = vars.ip,
			port = vars.port,
			userpass = {
				user = vars.user,
				password = vars.password,
			},
		})

		session:connect()
		ltf.defer(function()
			session:close()
		end)
		ltf.log_info("Successfully opened new SSH session")

		local sftp_session = session:new_sftp_session()
		ltf.defer(function()
			sftp_session:close()
		end)
		ltf.log_info("Successfully opened new SFTP session")
	end,
})
