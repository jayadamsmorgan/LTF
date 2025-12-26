local ltf = require("ltf")
local ssh = ltf.ssh

ltf.test({
	name = "SSH channel execute example",
	tags = { "exec" },
	body = function()
		-- Initialize SSH session
		local session = ssh.new_session({
			ip = ltf.get_var("ip"),
			port = tonumber(ltf.get_var("port")),
			userpass = {
				user = ltf.get_secret("user"),
				password = ltf.get_secret("password"),
			},
		})

		-- Connect to the session
		session:connect()
		ltf.defer(function()
			-- Close the session on test teardown
			session:close()
		end)
		ltf.log_info("Successfully opened new SSH session")

		-- Open new SSH channel for command execution
		local exec_channel = session:new_exec_channel()
		ltf.defer(function()
			-- Close the channel on test teardown
			exec_channel:close()
		end)

		-- Execute the command on the remote host
		local result = exec_channel:exec({
			cmd = "/bin/ls -la",
		})
		print("stdout: " .. result.stdout)
		print("stderr: " .. result.stderr)
		print("exit_code: " .. result.exitcode)
	end,
})

ltf.test({
	name = "SSH channel shell example",
	tags = { "shell" },
	body = function()
		-- Initialize SSH session
		local session = ssh.new_session({
			ip = ltf.get_var("ip"),
			port = tonumber(ltf.get_var("port")),
			userpass = {
				user = ltf.get_secret("user"),
				password = ltf.get_secret("password"),
			},
		})

		-- Connect to the session
		session:connect()
		ltf.defer(function()
			-- Close the session on test teardown
			session:close()
		end)
		ltf.log_info("Successfully opened new SSH session")

		-- Create new SSH shell channel
		local shell_channel = session:new_shell_channel({
			terminal = "xterm",
		})
		ltf.defer(function()
			-- Close the shell channel on test teardown
			shell_channel:close()
		end)
		ltf.log_info("Successfully opened new SSH channel")

		-- Write command to the shell
		shell_channel:write("/bin/ls -la\n")

		-- Read from remote host's stdout until we encounter '.bashrc'
		local found, read = shell_channel:read_until({
			pattern = ".bashrc",
			timeout = 500,
			read_opts = {
				chunk_size = 1,
				stream = "stdout",
			},
		})
		if found then
			ltf.log_info("Found .bashrc!")
		else
			ltf.log_info(".bashrc not found...")
		end
		ltf.log_info("Read string: " .. read)
	end,
})

ltf.test({
	name = "SSH SFTP file transfer",
	tags = { "sftp" },
	body = function()
		-- Initialize SSH session
		local session = ssh.new_session({
			ip = ltf.get_var("ip"),
			port = tonumber(ltf.get_var("port")),
			userpass = {
				user = ltf.get_secret("user"),
				password = ltf.get_secret("password"),
			},
		})

		-- Connect to the session
		session:connect()
		ltf.defer(function()
			-- Close the session on test teardown
			session:close()
		end)
		ltf.log_info("Successfully opened new SSH session")

		-- Open new SFTP channel on the session
		local sftp_channel = session:new_sftp_channel()
		ltf.defer(function()
			-- Close the channel on test teardown
			sftp_channel:close()
		end)
		ltf.log_info("Successfully opened new SFTP session")

		-- Create test file contents
		local contents = "This is test of the SFTP protocol!\n"

		-- Create and write contents to the test file
		local tmp_file = io.open("/tmp/testfile", "w")
		assert(tmp_file)
		tmp_file:write(contents)
		tmp_file:close()

		-- Send the file to the remote host
		sftp_channel:send({
			local_file = "/tmp/testfile",
			remote_file = "/tmp/testfile_copy",
			mode = "overwrite",
		})
		ltf.log_info("Successfully transfered file to remote host")

		-- Receive the sent file back into a new one
		sftp_channel:receive({
			local_file = "/tmp/testfile_return",
			remote_file = "/tmp/testfile_copy",
		})
		ltf.log_info("Successfully transfered file from remote host")

		-- Get the file_info of the file on the remote host
		local file_info = sftp_channel:file_info("/tmp/testfile_copy")
		assert(file_info ~= nil)
		ltf.log_info("File size: " .. file_info.size)

		-- Check that contents of the returned file are the same as the original one
		local return_file = io.open("/tmp/testfile_return", "r")
		assert(return_file)
		local return_contents = return_file:read("a")
		return_file:close()
		assert(return_contents == contents)
	end,
})
