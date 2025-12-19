local low = require("ltf-ssh")

--- @class ssh_channel
---
--- Perform channel execute
--- @field exec fun(self: ssh_channel, cmd: string)
---
--- Enter shell mode
--- @field shell fun(self: ssh_channel)
---
--- Get last exit status
--- @field get_exit_status fun(self: ssh_channel): integer
---
--- Flush "stdout"
--- @field flush fun(self: ssh_channel)
---
--- Flush "stderr"
--- @field flush_stderr fun(self: ssh_channel)
---
--- Write to the channel
--- @field write fun(self: ssh_channel, str: string)
---
--- Read from "stdout"
--- @field read fun(self: ssh_channel, chunk_size: integer): string
---
--- Read from "stderr"
--- @field read_stderr fun(self: ssh_channel, chunk_size: integer): string
---
--- Set environment variable
--- @field setenv fun(self: ssh_channel, var: string, value: string)
---
--- Send EOF to the channel
--- @field send_eof fun(self: ssh_channel)
---
--- Returns true if reached EOF
--- @field eof fun(self: ssh_channel): boolean
---
--- Waits for EOF on the channel
--- @field wait_eof fun(self: ssh_channel)
---
--- Close the SSH channel
--- @field close fun(self: ssh_channel)
---
--- Request PTY on the channel
--- @field request_pty fun(self: ssh_channel, term: string)

--- @class ssh_channel_read_opts
--- @field chunk_size integer? size of the "chunk" for single read operation. Default is 64
--- @field stream ssh_channel_stream? stream to read from (stdout/stderr). Default is "stdout"

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
