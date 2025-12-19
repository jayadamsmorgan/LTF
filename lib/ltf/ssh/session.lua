local low = require("ltf-ssh")

local M = {}

--- @class ssh_session
---
--- Connect to the remote SSH host
--- @field connect fun(self: ssh_session)
---
--- Disconnect from remote host
--- @field disconnect fun(self: ssh_session, description: string?)
---
--- Disconnect from the remote host and close the ongoing connection
--- @field close fun(self: ssh_session)
---
--- Create new SSH channel with creating an SSH shell emulation
--- @field new_shell_channel fun(self: ssh_session, opts: ssh_shell_channel_opts?): ssh_shell_channel
---
--- Create new SSH channel for command execution
--- @field new_exec_channel fun(self: ssh_session):ssh_exec_channel
---
--- Create new SSH SFTP channel for file transfer
--- @field new_sftp_channel fun(self: ssh_session): sftp_channel

--- @class ssh_auth_method_userpass
--- @field user string
--- @field password string

--- @class ssh_create_session_params
--- @field ip string IP of the remote host
--- @field port integer? SSH port. Default: 22
--- @field userpass ssh_auth_method_userpass? Authenticate with user and password

--- Create new SSH connection
---
--- @param params ssh_create_session_params parameters to create SSH session with. One of the auth methods should be present: [userpass]
---
--- @return ssh_session
M.new_session = function(params)
	low.lib_init()

	local function prepare_session(session)
		local mt = getmetatable(session)
		function mt.__index:new_exec_channel()
			return require("ltf.ssh.exec_channel").new_exec_channel(self)
		end
		function mt.__index:new_shell_channel(opts)
			return require("ltf.ssh.shell_channel").new_shell_channel(self, opts)
		end
		function mt.__index:new_sftp_channel()
			return require("ltf.ssh.sftp").new_sftp_channel(self)
		end
	end

	if params.userpass then
		local session =
			low.session_init_userpass(params.ip, params.port or 22, params.userpass.user, params.userpass.password)
		prepare_session(session)
		return session
	end

	-- Extend later with other auth methods:
	--
	-- if params.someotherauth then
	--     session = low.session_init_something(...)
	--     prepare_session()
	--     return session
	-- end

	error("Unable to create session: No auth method provided.")
end

return M
