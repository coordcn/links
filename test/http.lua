local http = require('http')

local options = {
  host: '127.0.0.1',
  port: 80
}

local server = http.createServer(handle, options)

function handle(req, res)
  server:close()
end

