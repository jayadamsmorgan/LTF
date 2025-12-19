local low = require("ltf-ssh")

local session = require("ltf.ssh.session")

local M = {}

M.low = low

M.new_session = session.new_session

return M
