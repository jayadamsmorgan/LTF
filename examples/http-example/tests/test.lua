local ltf = require("ltf")
local http = ltf.http

ltf.test({
	name = "test ltf.http",
	body = function()
		local handle = http.new()
		ltf.defer(function()
			handle:cleanup()
		end)

		local src = "Hello from Lua!"
		local pos = 1

		handle
			:setopt(http.OPT_URL, "http://httpbin.org/post")
			:setopt(http.OPT_POST, 1)
			:setopt(http.OPT_INFILESIZE, #src)
			:setopt(http.OPT_READFUNCTION, function(max)
				local chunk = src:sub(pos, pos + max - 1)
				pos = pos + #chunk
				return chunk
			end)
			:setopt(http.OPT_WRITEFUNCTION, function(chunk, n)
				print(chunk)
				return n
			end)

		handle:perform()
	end,
})
