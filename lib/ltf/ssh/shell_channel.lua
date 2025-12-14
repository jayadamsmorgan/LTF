--- @class ssh_shell_channel
---
--- @field low ssh_channel
---
--- @field write fun(self: ssh_shell_channel, str: string)
--- @field read fun(self: ssh_shell_channel, opts: ssh_channel_read_opts): string
--- @field read_until fun(self: ssh_shell_channel, opts: ssh_read_until_opts): boolean, string
--- @field close fun(self: ssh_shell_channel)

local channel = require("ltf.ssh.channel")
local ltf = require("ltf-main")

local M = {}

--- @type ssh_channel_read_opts
M.default_ssh_read_opts = {
	stream = "stdout",
	chunk_size = 64,
}

--- @type ssh_read_until_opts
M.default_ssh_read_until_opts = {
	pattern = "\n",
	timeout = 200, -- total timeout, read_opts.timeout = timeout for each individual reads
	read_opts = M.default_ssh_read_opts,
}

--- @param chan ssh_channel
--- @param opts ssh_channel_read_opts
local function read(chan, opts)
	local def = channel.default_ssh_read_opts
	opts = opts or def
	opts.stream = opts.stream or def.stream
	opts.chunk_size = opts.chunk_size or def.chunk_size

	if opts.stream == "stdout" then
		return chan:read(opts.chunk_size)
	else
		return chan:read_stderr(opts.chunk_size)
	end
end

--- @class ssh_read_until_opts
--- @field pattern string
--- @field timeout integer?
--- @field read_opts ssh_channel_read_opts

--- @param chan ssh_channel
--- @param opts ssh_read_until_opts
---
--- @return boolean, string
local function read_until(chan, opts)
	local now_ms = function()
		return ltf:millis()
	end

	local def = channel.default_ssh_read_until_opts
	opts = opts or def
	opts.timeout = opts.timeout or def.timeout
	opts.read_opts = opts.read_opts or def.read_opts
	opts.read_opts.stream = opts.read_opts.stream or def.read_opts.stream
	opts.read_opts.chunk_size = opts.read_opts.chunk_size or def.read_opts.chunk_size

	local buftable = {}
	local deadline = now_ms() + opts.timeout

	while true do
		if deadline then
			local remaining = math.max(0, math.ceil(deadline - now_ms()))
			if remaining == 0 then
				error("timeout")
			end
		end

		local chunk
		if opts.read_opts.stream == "stdout" then
			chunk = chan:read(opts.read_opts.chunk_size)
		else
			chunk = chan:read_stderr(opts.read_opts.chunk_size)
		end

		if chunk and #chunk > 0 then
			table.insert(buftable, chunk)

			local full_buff = table.concat(buftable)
			if full_buff:find(opts.pattern, 1, true) then
				return true, full_buff
			end
		end

		if not deadline and (not chunk or #chunk == 0) then
			break
		end
	end

	return false, ""
end

--- @class ssh_shell_channel_opts
--- @field terminal string?

--- @type ssh_shell_channel_opts
local default_ssh_shell_channel_opts = {
	terminal = "xterm",
}

--- @param session ssh_session
--- @param opts ssh_shell_channel_opts?
---
--- @return ssh_shell_channel
M.new_shell_channel = function(session, opts)
	local low = channel.open_channel(session)

	local def = default_ssh_shell_channel_opts
	opts = opts or def
	opts.terminal = opts.terminal or def.terminal

	low:request_pty(opts.terminal)
	low:shell()

	local mt = {}
	mt.__index = mt

	local shell_channel = {
		low = low,
	}

	function mt:write(str)
		return self.low:write(str)
	end
	function mt:read(read_opts)
		return read(self.low, read_opts)
	end
	function mt:read_until(read_until_opts)
		return read_until(self.low, read_until_opts)
	end
	function mt:close()
		return self.low:close()
	end

	setmetatable(shell_channel, mt)

	return shell_channel
end

return M
