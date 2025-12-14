local M = {}

--- @class ssh_exec_channel
---
--- @field low ssh_channel
---
--- @field exec fun(self: ssh_exec_channel, opts: ssh_channel_exec_opts): ssh_channel_exec_result
--- @field close fun(self: ssh_exec_channel)

--- @class ssh_channel_exec_opts
--- @field cmd string
--- @field env table<string, string>?
--- @field read_chunk_size integer?

--- @class ssh_channel_exec_result
--- @field stdout string
--- @field stderr string
--- @field exit_code integer

--- @param chan ssh_channel
--- @param stream ssh_channel_stream
--- @param chunk_size integer
local function read_helper(chan, stream, chunk_size)
	local buftable = {}
	while true do
		local chunk
		if stream == "stdout" then
			chunk = chan:read(chunk_size)
		else
			chunk = chan:read_stderr(chunk_size)
		end

		if #chunk == 0 then
			break
		end

		table.insert(buftable, chunk)
	end

	return table.concat(buftable)
end

--- @param session ssh_session
---
--- @return ssh_exec_channel
M.new_exec_channel = function(session)
	local chan = require("ltf.ssh.channel").open_channel(session)

	local exec_channel = {
		low = chan,
	}

	local mt = {}
	mt.__index = mt

	--- @param exec_opts ssh_channel_exec_opts
	function mt:exec(exec_opts)
		if exec_opts.env then
			for key, value in pairs(exec_opts.env) do
				self.low:setenv(key, value)
			end
		end

		self.low:exec(exec_opts.cmd)

		self.low:wait_eof()

		local result = {
			stdout = read_helper(self.low, "stdout", exec_opts.read_chunk_size or 64),
			stderr = read_helper(self.low, "stderr", exec_opts.read_chunk_size or 64),
			exit_code = self.low:get_exit_status(),
		}

		return result
	end

	function mt:close()
		return self.low:close()
	end

	setmetatable(exec_channel, mt)

	return exec_channel
end

return M
