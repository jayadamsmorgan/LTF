local low = require("ltf-ssh")

local session = require("session")

local M = {}

M.low = low

M.new_session = session.new_session

return M
