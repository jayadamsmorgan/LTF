-- Curent lib version working only in blocking  mode

local ltf = require("ltf-main")
local ts = require("ltf-ssh")

local M = {}

-- For low level function usage in tests
M.low = ts

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

local CHUNK_DEFAULT = 4096

local function waitsocket_for(connection)
	return ts.low.waitsocket(connection.socket, connection.session)
end

local function read_full_stdout(channel, connection, chunk_size)
	chunk_size = CHUNK_DEFAULT
	local parts = {}
	local idx = 1

	while true do
		local chunk, err = channel:read(chunk_size)
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
			else
				return nil, tostring(err or "unknown read error")
			end
		end
	end

	return table.concat(parts)
end

-- read_full_stderr(channel, connection, chunk_size)
-- аналогично для stderr
local function read_full_stderr(channel, connection, chunk_size)
	chunk_size = chunk_size or CHUNK_DEFAULT
	local parts = {}
	local idx = 1

	while true do
		local chunk, err = channel:read_stderr(chunk_size)
		if chunk then
			if #chunk > 0 then
				parts[idx] = chunk
				idx = idx + 1
			else
				-- EOF stderr
				break
			end
		else
			if err and err:find("EAGAIN") then
				local ok, werr = waitsocket_for(connection)
				if not ok then
					return nil, "waitsocket failed: " .. tostring(werr)
				end
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

	-- Pass env
	-- set_env(channel, env)

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
M.open_shell = function(connection, cmd) end

-- Arguments:
M.shell_write = function(connection, cmd) end

-- Arguments:
M.shell_read_until = function(connection, cmd) end

-- Arguments:
M.close_shell = function(connection, cmd) end

-- Arguments:
M.send_file_scp = function(connection, cmd) end

-- Arguments:
M.recv_file_scp = function(connection, cmd) end

return M
