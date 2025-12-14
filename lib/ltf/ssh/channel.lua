local low = require("ltf-ssh")

--- @class ssh_channel
--- @field exec fun(self: ssh_channel, cmd: string)
--- @field shell fun(self: ssh_channel)
--- @field get_exit_status fun(self: ssh_channel): integer
--- @field flush fun(self: ssh_channel)
--- @field write fun(self: ssh_channel, str: string)
--- @field read fun(self: ssh_channel, chunk_size: integer): string
--- @field read_stderr fun(self: ssh_channel, chunk_size: integer): string
--- @field setenv fun(self: ssh_channel, var: string, value: string)
--- @field send_eof fun(self: ssh_channel)
--- @field eof fun(self: ssh_channel): boolean
--- @field wait_eof fun(self: ssh_channel)
--- @field close fun(self: ssh_channel)
--- @field request_pty fun(self: ssh_channel, term: string)

--- @class ssh_channel_read_opts
--- @field chunk_size integer?
--- @field stream ssh_channel_stream?

--- @alias ssh_channel_stream
--- | '"stdout"'
--- | '"stderr"'

local M = {}

--- @param session ssh_session
---
--- @return ssh_channel
M.open_channel = function(session)
	--- @type ssh_channel
	local channel = low.channel_init(session)

	return channel
end

return M
