-- Curent lib version working only in blocking  mode

local ltf = require("ltf-main")
local ts = require("ltf-ssh")

local M = {}

-- For low level function usage in tests
M.low = ts

local DEFAULT_CHUNK = 4096

local DEFAULT_TIMEOUT = 10000

-- Arguments: ip, port, usr, pswd,
M.create_connection = function(ip, port, usr, pswd, timeout)
	local res, err = ts.lib_init()
	if res == nil then
		error(err)
	end

	local session, err = ts.session_init()

	if session == nil then
		error(err)
	end

	local socket, err = ts.socket_init(ip, port)
	if socket == nil then
		error(err)
	end

	res, err = session:handshake(socket)
	if res == nil then
		error(err)
	end

	res, err = session:userauth_password(usr, pswd)
	if res == nil then
		error(err)
	end

	-- Set timeout for all operation within function
	if timeout ~= nil then
		session:set_timeout(timeout)
		session:set_read_timeout(timeout)
	end

	-- create connection struct
	local connection = {
		ip = ip,
		port = port,
		usr = usr,
		pswd = pswd,
		session = session,
		socket = socket,
	}

	return connection
end

-- Helper to pass env variables for channel
local function set_env(channel, env)
	for _, pair in ipairs(env) do
		local ok, err = channel:set_env(pair.var, pair.val)
		if not ok then
			error(("setenv failed for %s=%s: %s"):format(pair.var, pair.val, tostring(err)))
		end
	end
end

-- Arguments: ip, port, usr, pswd,
M.close_connection = function(connection)
	local res, err = connection.session:disconnect("User disconnect")
	if res == nil then
		ltf.log_error(err)
	end

	res, err = connection.session:free()
	if res == nil then
		ltf.log_error(err)
	end

	res, err = ts.socket_free(connection.socket)
	if res == nil then
		ltf.log_error(err)
	end
	ts.lib_exit()
end

local function waitsocket_for(connection)
	return ts.low.waitsocket(connection.socket, connection.session)
end

local function read_full_stdout(channel, connection, chunk_size, timeout, until_str)
	-- Timeout for whole operation
	if timeout == nil then
		print("Set default timeout")
		timeout = DEFAULT_TIMEOUT
	end

	--  Timeout for atomic ssh read operation
	connection.session:set_timeout(100)
	connection.session:set_read_timeout(100)

	local now_ms = function()
		return ltf:millis()
	end

	local last_time = now_ms()
	local remaining = timeout
	local full_buff
	local parts = {}
	local idx = 1
	while true do
		local chunk, err = channel:read(chunk_size)
		local now = now_ms()
		local elapsed = now - last_time
		last_time = now
		remaining = remaining - elapsed

		if chunk and #chunk > 0 then
			-- parts[idx] = chunk
			idx = idx + 1
			table.insert(parts, chunk)
			full_buff = table.concat(parts)
		end

		-- In case of exec_cmd
		if channel:eof() then
			break
		-- In case of read_until
		elseif until_str and full_buff:find(until_str, 1, true) then
			break
		-- in case of read and read_until
		elseif remaining > 0 then
			if err and err:find("EAGAIN") then
				local ok, werr = waitsocket_for(connection)
				if not ok then
					return nil, "waitsocket failed: " .. tostring(werr)
				end
			elseif err and not err:find("TIMEOUT") then
				return nil, tostring(err or "unknown read error")
			end
		else
			break
		end
	end

	connection.session:set_timeout(DEFAULT_TIMEOUT)
	connection.session:set_read_timeout(DEFAULT_TIMEOUT)

	return full_buff
end

-- read_full_stderr(channel, connection, chunk_size)
local function read_full_stderr(channel, connection, chunk_size)
	chunk_size = chunk_size or DEFAULT_CHUNK
	local parts = {}
	local idx = 1

	while true do
		local chunk, err = channel:read_stderr(chunk_size)
		if chunk then
			if #chunk > 0 then
				parts[idx] = chunk
				idx = idx + 1
			else
				break
			end
		else
			if err and err:find("EAGAIN") then
				local ok, werr = waitsocket_for(connection)
				if not ok then
					return nil, "waitsocket failed: " .. tostring(werr)
				end
			elseif err:find("TIMEOUT") then
				break
			else
				return nil, tostring(err or "unknown read_stderr error")
			end
		end
	end

	return table.concat(parts)
end
-- Execute command via  previously opened ssh connection with <create_connection>
-- Pass true to stdout_b and/or stderr_b to recive stdout and/or stderr output
M.execute_cmd = function(connection, cmd, stdout_b, stderr_b, timeout, env)
	-- Set timeout for all operation within function
	if timeout ~= nil then
		connection.session:set_timeout(timeout)
		connection.session:set_read_timeout(timeout)
	end

	-- Initiate  channel within existing connection
	local channel, err = ts.channel_init(connection.session)
	if not channel then
		error("open_channel failed: " .. tostring(err))
	end

	-- Some environment variables may be set,
	-- It's up to the server which ones it'll allow though
	if env ~= nil then
		set_env(channel, env)
	end

	-- Execute command
	local ok, e = channel:exec(cmd)
	if not ok then
		if e and e:find("EAGAIN") then
			ts.waitsocket(connection.socket, connection.session)
			ok, e = channel:exec(cmd)
		end
		if not ok then
			error("exec failed: " .. tostring(e))
		end
	end

	-- Recive command's stdout
	local stdout_text, serr = read_full_stdout(channel, connection)
	if not stdout_text then
		error("stdout read error: " .. tostring(serr))
	end

	-- Recive command's stderr
	local stderr_text, serr2 = read_full_stderr(channel, connection)
	if not stderr_text then
		error("stderr read error: " .. tostring(serr2))
	end

	-- Close channel and free chanel's resources
	local okc, cerr = channel:close()
	if okc == nil and cerr:find("EAGAIN") then
		ts.waitsocket(connection.socket, connection.session)
		okc, cerr = channel:close()
	end

	channel:free()

	-- Return stdout and/or stderr if stdout_b and/or stderr_b is true

	if stdout_b and stderr_b then
		return stdout_text, stderr_text
	elseif stdout_b then
		return stdout_text
	elseif stderr_b then
		return stderr_text
	else
		return true
	end
end

-- Arguments:
M.open_shell = function(connection, terminal, timeout, env)
	-- Set timeout for all operation within function
	if timeout ~= nil then
		connection.session:set_timeout(timeout)
		connection.session:set_read_timeout(timeout)
	end

	-- Initiate  channel within existing connection
	local channel, err = ts.channel_init(connection.session)
	if not channel then
		error("open_channel failed: " .. tostring(err))
	end

	-- Some environment variables may be set,
	-- It's up to the server which ones it'll allow though
	if env ~= nil then
		set_env(channel, env)
	end

	-- Request PTY
	local ok, rerr = channel:request_pty(terminal)
	if rerr then
		error("stdout read error: " .. tostring(err))
	end
	-- Open SHELL
	local ok1, err2 = channel:shell()
	if err2 then
		error("stdout read error: " .. tostring(err))
	end
	return channel
end

-- Write message in shell context, shell must be opened before
M.shell_write = function(connection, channel, cmd)
	local ok, err = channel:write(cmd, #cmd)
	if ok == nil and err:find("EAGAIN") then
		ts.waitsocket(connection.socket, connection.session)
		ok, err = channel:write(cmd, #cmd)
	end
	-- channel:send_eof(channel)
	return ok, err
end

-- Read in shell context, shell must be opened before
M.shell_read = function(connection, channel, timeout, chunk_size)
	chunk_size = chunk_size or DEFAULT_CHUNK
	local stdout_text, err = read_full_stdout(channel, connection, chunk_size, timeout)
	if err then
		error("stdout read error: " .. tostring(err))
	end
	return stdout_text, err
end

-- Read Until some string will appear in std_out in shell context, shell must be opened before
M.shell_read_until = function(connection, channel, timeout, prompt, chunk_size)
	chunk_size = chunk_size or DEFAULT_CHUNK
	local stdout_text, err = read_full_stdout(channel, connection, chunk_size, timeout, prompt)
	if err then
		error("stdout read error: " .. tostring(err))
	end
	return stdout_text, err
end

-- Arguments:
M.close_shell = function(connection, channel)
	-- Close channel and free chanel's resources
	local okc, cerr = channel:close()
	if okc == nil and cerr:find("EAGAIN") then
		ts.waitsocket(connection.socket, connection.session)
		okc, cerr = channel:close()
	end

	channel:free()
end

-- Send file to remote host
-- M.send_file_scp = function(connection, path_to_remotefile, path_to_localfile, mode, chunk_size)
-- 	-- Some settings
-- 	chunk_size = chunk_size or DEFAULT_CHUNK
--
-- 	-- Parse localfile info get path string return
-- 	local f, ferr = io.open(path_to_localfile, "rb")
-- 	if not f then
-- 		error("fopen: " .. path_to_localfile .. "failed with code: " .. tostring(ferr))
-- 	end
-- 	-- determine file size
-- 	local ok, size_or_err = pcall(function()
-- 		return f:seek("end")
-- 	end)
-- 	if not ok or not size_or_err then
-- 		f:close()
-- 		error("fseek: " .. path_to_localfile .. "failed with code: " .. tostring(size_or_err))
-- 	end
-- 	local filesize = size_or_err
-- 	print(filesize)
-- 	f:seek("set", 0)
--
-- 	-- try to open scp send channel (handle EAGAIN)
-- 	local ch, serr = ts.scp_send64(connection.session, path_to_remotefile, mode, filesize, 0, 0)
-- 	if not ch and serr and serr:find("EAGAIN") then
-- 		local okw, werr = ts.waitsocket(connection.socket, connection.session)
-- 		if not okw then
-- 			f:close()
-- 			error("scp_send64 failed with code: " .. tostring(serr))
-- 		end
-- 		ch, serr = ts.scp_send64(connection.session, path_to_remotefile, mode, filesize, 0, 0)
-- 	end
-- 	if not ch then
-- 		f:close()
-- 		error("scp_send64 failed with code: " .. tostring(serr))
-- 	end
--
-- 	local total_sent = 0
-- 	local buf
-- 	while true do
-- 		buf = f:read(chunk_size)
-- 		if not buf then
-- 			if f:seek() then
-- 				-- nil from read but not EOF? treat as error
-- 				-- (in Lua read returns nil on EOF or error; can't easily separate, so check EOF)
-- 			end
-- 		end
-- 		if not buf or #buf == 0 then
-- 			break
-- 		end
--
-- 		local offset = 1
-- 		local remaining = #buf
-- 		while remaining > 0 do
-- 			local piece = buf:sub(offset, offset + remaining - 1)
-- 			local written, werr = ch:write(piece, #piece)
-- 			if written == nil then
-- 				if werr and werr:find("EAGAIN") then
-- 					local okw, werr2 = ts.waitsocket(connection.socket, connection.session)
-- 					if not okw then
-- 						-- cleanup
-- 						pcall(function()
-- 							ch:send_eof()
-- 						end)
-- 						pcall(function()
-- 							ch:wait_eof()
-- 						end)
-- 						pcall(function()
-- 							ch:wait_closed()
-- 						end)
-- 						pcall(function()
-- 							ch:free()
-- 						end)
-- 						f:close()
-- 						error("scp_send64 failed with code: " .. tostring(werr2))
-- 					end
-- 					-- retry write
-- 				else
-- 					-- unrecoverable write error
-- 					pcall(function()
-- 						ch:send_eof()
-- 					end)
-- 					pcall(function()
-- 						ch:wait_eof()
-- 					end)
-- 					pcall(function()
-- 						ch:wait_closed()
-- 					end)
-- 					pcall(function()
-- 						ch:free()
-- 					end)
-- 					f:close()
-- 					error("scp_send64 failed with code: " .. tostring(werr2))
-- 				end
-- 			else
-- 				-- written is number of bytes written
-- 				offset = offset + written
-- 				remaining = remaining - written
-- 				total_sent = total_sent + written
-- 			end
-- 		end
-- 	end
-- 	-- channel cleannng procedure
-- 	pcall(function()
-- 		ch:send_eof()
-- 	end)
-- 	pcall(function()
-- 		ch:wait_eof()
-- 	end)
-- 	pcall(function()
-- 		ch:wait_closed()
-- 	end)
-- 	-- fallback: try close() (some bindings implement close())
-- 	pcall(function()
-- 		ch:close()
-- 	end)
-- 	pcall(function()
-- 		ch:free()
-- 	end)
--
-- 	f:close()
--
-- 	return total_sent, nil
-- end

-- Recive file to remote host
-- M.recv_file_scp = function(connection, cmd)
-- local local_path, remote_path, opts_or_err = _parse_cmd(cmd, false)
--     if not local_path then
--         return nil, opts_or_err
--     end
--     local opts = opts_or_err or {}
--     local chunk_size = opts.chunk_size or DEFAULT_CHUNK
--     local timeout_ms = opts.timeout_ms or DEFAULT_TIMEOUT
--
--     -- open scp channel (handle EAGAIN)
--     local ch, fileinfo, serr = ts.scp_recv2(connection.session, remote_path)
--     if not ch and serr and serr:find("EAGAIN") then
--         local okw, werr = ts.waitsocket(connection.socket, connection.session)
--         if not okw then
--             return nil, ("waitsocket failed: %s"):format(tostring(werr))
--         end
--         ch, fileinfo, serr = ts.scp_recv2(connection.session, remote_path)
--     end
--     if not ch then
--         return nil, ("scp_recv2 failed: %s"):format(tostring(serr))
--     end
--
--     -- open local file for write
--     local out, oerr = io.open(local_path, "wb")
--     if not out then
--         pcall(function() ch:free() end)
--         return nil, ("fopen(%s) failed: %s"):format(tostring(local_path), tostring(oerr))
--     end
--
--     local total = 0
--     local filesize = tonumber(fileinfo.size) or 0
--
--     while total < filesize do
--         local chunk, rerr = ch:read(chunk_size)
--         if chunk and #chunk > 0 then
--             local wrote, werr = out:write(chunk)
--             if not wrote and werr then
--                 out:close()
--                 pcall(function() ch:free() end)
--                 return nil, ("local write failed: %s"):format(tostring(werr))
--             end
--             total = total + #chunk
--         else
--             if rerr and rerr:find("EAGAIN") then
--                 local okw, werr = ts.waitsocket(connection.socket, connection.session)
--                 if not okw then
--                     out:close()
--                     pcall(function() ch:free() end)
--                     return nil, ("waitsocket failed: %s"):format(tostring(werr))
--                 end
--                 -- retry read
--             elseif rerr and rerr:find("TIMEOUT") then
--                 -- treat timeout as failure for recv_file_scp
--                 out:close()
--                 pcall(function() ch:free() end)
--                 return nil, ("read timeout")
--             else
--                 -- EOF or other error -> break/exit
--                 break
--             end
--         end
--     end
--
--     -- cleanup channel
--     if ch.wait_closed then pcall(function() ch:wait_closed() end) end
--     pcall(function() ch:free() end)
--     out:close()
--
--     return total, nil
-- end

M.send_file_sftp = function(connection, path_to_remotefile, path_to_localfile, mode, chunk_size)
	-- Some settings
	chunk_size = chunk_size or DEFAULT_CHUNK
	-- Parse localfile info get path string return
	local f, ferr = io.open(path_to_localfile, "rb")
	if not f then
		error("fopen: " .. path_to_localfile .. "failed with code: " .. tostring(ferr))
	end
	-- determine file size
	local ok, size_or_err = pcall(function()
		return f:seek("end")
	end)
	if not ok or not size_or_err then
		f:close()
		error("fseek: " .. path_to_localfile .. "failed with code: " .. tostring(size_or_err))
	end
	local filesize = size_or_err
	print(filesize)
	f:seek("set", 0)

	local sftp_session, err = ts.sftp_init(connection.session)
	if not sftp_session then
		f:close()
		error("sftp_init failed with code: " .. tostring(size_or_err))
	end
	local open_ok, open_err = sftp_session:open(path_to_remotefile, 26, 420)
	if not open_ok then
		pcall(function()
			sftp_session:shutdown()
		end)
		f:close()
		error("sftp_open failed with code: " .. tostring(open_err))
	end

	-- write loop
	local total_sent = 0
	local buf_len = chunk_size
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
			local written, werr = sftp_session:write(piece, #piece)

			if written == nil then
				-- error or EAGAIN
				if werr and tostring(werr):find("EAGAIN") then
					-- wait then retry
					local w_ok, w_err = ts.waitsocket(connection.socket, connection.session)
					if not w_ok then
						-- cleanup
						pcall(function()
							sftp_session:close()
						end)
						pcall(function()
							sftp_session:shutdown()
						end)
						f:close()
						return nil, ("waitsocket failed during write: %s"):format(tostring(w_err))
					end
					-- retry write (do not advance offset)
				else
					-- unrecoverable write error
					pcall(function()
						sftp_session:close()
					end)
					pcall(function()
						sftp_session:shutdown()
					end)
					f:close()
					return nil, ("sftp write failed: %s"):format(tostring(werr or "unknown"))
				end
			else
				-- advance by number of bytes written
				offset = offset + written
				remaining = remaining - written
				total_sent = total_sent + written
			end
		end
	end

	-- close sftp handle and shutdown session
	local ok_c, cerr = pcall(function()
		return sftp_session:close()
	end)
	if not ok_c then
		-- best-effort: still try shutdown
		pcall(function()
			sftp_session:shutdown()
		end)
		f:close()
		return nil, ("sftp_close error: %s"):format(tostring(cerr))
	end
	local ok_sh, sh_err = pcall(function()
		return sftp_session:shutdown()
	end)
	if not ok_sh then
		f:close()
		return nil, ("sftp_shutdown error: %s"):format(tostring(sh_err))
	end

	f:close()
end
M.recv_file_sftp = function(connection, path_to_remotefile, path_to_localfile, chunk_size)
	chunk_size = chunk_size or DEFAULT_CHUNK

	-- open local file for writing
	local out, oerr = io.open(path_to_localfile, "wb")
	if not out then
		return nil, ("fopen failed: %s (%s)"):format(tostring(path_to_localfile), tostring(oerr))
	end

	-- init sftp session
	local sftp_session, serr = ts.sftp_init(connection.session)
	if not sftp_session then
		out:close()
		return nil, ("sftp_init failed: %s"):format(tostring(serr))
	end

	-- open remote file for read (flag = 1)
	local ok_open, open_err = sftp_session:open(path_to_remotefile, 1, 0)
	if not ok_open then
		pcall(function()
			sftp_session:shutdown()
		end)
		out:close()
		return nil, ("sftp_open failed: %s"):format(tostring(open_err))
	end

	-- read loop
	local total_received = 0
	while true do
		local chunk, rerr = sftp_session:read(chunk_size)
		if chunk and #chunk > 0 then
			-- write to local file
			local ok, werr = out:write(chunk)
			if not ok then
				-- writing error
				pcall(function()
					sftp_session:close()
				end)
				pcall(function()
					sftp_session:shutdown()
				end)
				out:close()
				return nil, ("local write failed: %s"):format(tostring(werr))
			end
			total_received = total_received + #chunk
		else
			-- chunk is nil or empty string; handle cases
			if chunk == "" then
				-- EOF
				break
			end
			if chunk == nil and rerr then
				if tostring(rerr):find("EAGAIN") then
					-- wait and retry
					local w_ok, w_err = ts.waitsocket(connection.socket, connection.session)
					if not w_ok then
						pcall(function()
							sftp_session:close()
						end)
						pcall(function()
							sftp_session:shutdown()
						end)
						out:close()
						return nil, ("waitsocket failed during read: %s"):format(tostring(w_err))
					end
					-- retry read
				else
					-- unrecoverable remote read error
					pcall(function()
						sftp_session:close()
					end)
					pcall(function()
						sftp_session:shutdown()
					end)
					out:close()
					return nil, ("sftp read failed: %s"):format(tostring(rerr))
				end
			else
				-- chunk == nil and no rerr (treat as EOF)
				break
			end
		end
	end

	-- close and shutdown
	local ok_c, cerr = pcall(function()
		return sftp_session:close()
	end)
	if not ok_c then
		pcall(function()
			sftp_session:shutdown()
		end)
		out:close()
		return nil, ("sftp_close error: %s"):format(tostring(cerr))
	end
	local ok_sh, sh_err = pcall(function()
		return sftp_session:shutdown()
	end)
	if not ok_sh then
		out:close()
		return nil, ("sftp_shutdown error: %s"):format(tostring(sh_err))
	end

	out:close()
	return total_received, nil
end

M.file_should_exist = function(connection, path)
	-- init sftp session
	local sftp_session, serr = ts.sftp_init(connection.session)
	if not sftp_session then
		return nil, ("sftp_init failed: %s"):format(tostring(serr))
	end
	-- check with stat_ex
	local res, err = sftp_session:stat_ex(path)
	if res then
		sftp_session:shutdown()
		return true
	else
		local error_code = sftp_session:last_error()
		if error_code == 2 then
			sftp_session:shutdown()
			return false
		else
			sftp_session:shutdown()
			return nil
		end
	end
end
M.directory_should_exist = function(connection, path)
	local ret = M.file_should_exist(connection, path)
	return ret
end
M.put_directory = function() end
M.get_directory = function() end

return M
