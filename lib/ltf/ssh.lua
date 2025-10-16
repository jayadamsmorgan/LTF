-- Curent lib version working only in blocking  mode

local ltf = require("ltf-main")
local ts = require("ltf-ssh")

local M = {}

-- For low level function usage in tests
M.low = ts

-- Arguments: ip, port, usr, pswd,
M.create_connection = function(ip, port, usr, pswd)
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
end

-- Arguments: connection with stdout and stderr output
M.execute_cmd = function(connection, cmd, stdout_b, stderr_b) end

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
