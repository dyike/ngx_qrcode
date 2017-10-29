local cjson = require "cjson"


function retjson(code, message, data)
    local result = {
        code = code,
        message = message,
        data = data
    }

    ngx.header.Content_type = 'text/json'
    ngx.say(cjson.encode(result))
    ngx.exit(200);
end

function retinfo(message)
    ngx.header.Content_type = 'text/html'
    ngx.say(message)
    ngx.exit(200);
end



function getClientIp()
    IP = ngx.req.get_headers()['X-Real-IP']
    if IP == nil then
       IP = ngx.var.remote_addr
    end
    if IP == nil then 
       IP = "Unknown"
    end
    return IP
end
