local low = require("ltf-ssh")
local util = require("ltf.util")

local M = {}

--- @class sftp_channel_low
--- @field open fun(self: sftp_channel_low, remote_file: string, flags: integer, mode: integer, open_type: integer)
--- @field write fun(self: sftp_channel_low, chunk: string, chunk_size: integer)
--- @field read fun(self: sftp_channel_low, chunk_size: integer)
--- @field file_info fun(self: sftp_channel_low, remote_file: string): file_info?
--- @field resolve_symlink fun(self: sftp_channel_low, path: string): string? resolved_path
--- @field close fun()
--- @field shutdown fun()

--- @alias sftp_channel_send_flag
--- | '"create"' fail if remote host has a file with the same name
--- | '"overwrite"' overwrite the file on the remote host if it exists

--- @class sftp_file_transfer_opts
--- @field local_file string
--- @field remote_file string
--- @field resolve_symlinks boolean?
--- @field mode sftp_channel_send_flag?
--- @field file_permissions integer?
--- @field chunk_size integer?

--- @class sftp_file_exists_result
--- @field exists boolean
--- @field is_directory boolean

--- @class sftp_channel
---
--- @field low sftp_channel_low
--- @field ssh_session ssh_session
---
--- @field send fun(self: sftp_channel, opts: sftp_file_transfer_opts)
--- @field receive fun(self: sftp_channel, opts: sftp_file_transfer_opts)
--- @field file_info fun(self: sftp_channel, path: string): file_info?
--- @field close fun(self: sftp_channel)

--- @param channel sftp_channel
--- @param opts sftp_file_transfer_opts
local function send(channel, opts)
	opts.chunk_size = opts.chunk_size or 1024
	opts.resolve_symlinks = opts.resolve_symlinks or true
	opts.mode = opts.mode or "create"
	opts.file_permissions = opts.file_permissions or 420 -- think of a better way

	local file_info = util.file_info(opts.local_file)
	if not file_info then
		error("File " .. opts.local_file .. " does not exist")
	end

	local path = opts.local_file
	if file_info.type == "symlink" and opts.resolve_symlinks then
		local resolved = util.resolve_symlink(path)
		if not resolved then
			-- Dangling symlink, sending as is
		else
			path = resolved
			file_info = util.file_info(path)
			if not file_info then
				error("Unknown error, file " .. path .. " does not exist")
			end
		end
	end

	if file_info.type == "directory" then
		-- TODO
		error("Transferring directories is unsupported at the moment")
	end

	local size = file_info.size
	if size == 0 then
		error("File " .. opts.local_file .. " is of size 0")
	end

	local f, ferr = io.open(path, "rb")
	if not f then
		error("fopen: " .. path .. "failed with code: " .. tostring(ferr))
	end

	local flags = 0
	if opts.mode == "create" then
		flags = 10
		local remote_file_info = channel:file_info(opts.remote_file)
		if remote_file_info ~= nil then
			error(
				"File "
					.. opts.remote_file
					.. " already exists on the remote host. Use 'mode = \"overwrite\"' if you want to overwrite it."
			)
		end
	elseif opts.mode == "overwrite" then
		flags = 26
	else
		error("Unknown mode " .. opts.mode)
	end

	local open_type = 0 -- 0 for file, 1 - directory
	channel.low:open(opts.remote_file, flags, opts.file_permissions, open_type)

	-- write loop
	local total_sent = 0
	local buf_len = opts.chunk_size
	while true do
		local chunk = f:read(buf_len)
		if not chunk then
			-- EOF
			break
		end

		local offset = 1
		local remaining = #chunk
		while remaining > 0 do
			local piece = chunk:sub(offset, offset + remaining - 1)
			local written = channel.low:write(piece, #piece)

			-- advance by number of bytes written
			offset = offset + written
			remaining = remaining - written
			total_sent = total_sent + written
		end
	end

	channel.low:close()
	f:close()
end

--- @param session sftp_channel
--- @param opts sftp_file_transfer_opts
local function receive(session, opts)
	opts.chunk_size = opts.chunk_size or 1024
	local out, oerr = io.open(opts.local_file, "wb")
	if not out then
		return nil, ("fopen failed: %s (%s)"):format(tostring(opts.local_file), tostring(oerr))
	end

	session.low:open(opts.remote_file, 1, 0, 0)

	local total_received = 0
	while true do
		local chunk = session.low:read(opts.chunk_size)

		if #chunk == 0 then
			break
		end

		-- write to local file
		out:write(chunk)
		total_received = total_received + #chunk
	end

	session.low:close()
	out:close()
end

--- @param ssh_session ssh_session
---
--- @nodiscard
--- @return sftp_channel
M.new_sftp_channel = function(ssh_session)
	local low_channel = low.sftp_init(ssh_session)
	local channel = {
		low = low_channel,
		ssh_session = ssh_session,
	}
	local mt = {}
	mt.__index = mt

	function mt:send(opts)
		return send(self, opts)
	end
	function mt:receive(opts)
		return receive(self, opts)
	end
	function mt:file_info(path)
		return self.low:file_info(path)
	end
	function mt:close()
		return self.low:shutdown()
	end

	setmetatable(channel, mt)

	return channel
end

return M
