local low = require("ltf-ssh")

local M = {}

--- @class ssh_session
--- @field connect fun(self: ssh_session)
--- @field disconnect fun(self: ssh_session, description: string?)
--- @field close fun(self: ssh_session)
--- @field new_shell_channel fun(self: ssh_session, opts: ssh_shell_channel_opts?): ssh_shell_channel
--- @field new_exec_channel fun(self: ssh_session):ssh_exec_channel
--- @field new_sftp_session fun(self: ssh_session)

--- @class ssh_auth_method_userpass
--- @field user string
--- @field password string

--- @class ssh_create_session_params
--- @field ip string
--- @field port integer?
--- @field userpass ssh_auth_method_userpass?

--- Create new SSH connection
---
--- @param params ssh_create_session_params
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
		function mt.__index:new_sftp_session()
			return require("ltf.ssh.sftp").new_sftp_session(self)
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
