require "library.utils"
local log = require "library.log"
local qr = require "library.qr"
local CONF = require "config.config"

local get = ngx.req.get_uri_args()
local data = get.data or ""
local out_type = get.out_type or CONF.qr_output_type
local size = get.size or CONF.qr_size
local margin = get.margin or CONF.qr_margin
local fgcolor = get.foregroung or CONF.qr_fgcolor
local bgcolor = get.background or CONF.qr_bgcolor
local level = get.level or CONF.qr_level
local dpi = get.dpi or CONF.qr_dpi
local logosize = get.logosize or CONF.qr_size * CONF.logo_scale
local logomargin = get.logomargin or CONF.logo_margin
local logo = nil 
local logpbg = "logo_style.png"

-- 生成qr图像
local qrstr, qrerrcode, qrerrmsg = qr.generate(data, size, margin, fgcolor, bgcolor, level, dpi, logosize, logo, logobg, logomargin)

-- 生成失败
if qrstr == nil then 
    retjson(-1, qrerrmsg, {})
else 
    if out_type == "json" then   -- json形式输出信息
        retjson(0, "success", {url = data, imagedata = ngx.encode_base64(qrstr)})  
    elseif out_type == "download" then   -- 图片下载
        ngx.header.Cache_Control = "public"
        ngx.header.Expires = "0"
        ngx.header.Pragma = "public"
        ngx.header.Content_Type = "image/png"
        ngx.header.Content_Disposition = "attachment;filename="..ngx.escape_uri("QRCode")..".png"
        ngx.say(qrstr)
        ngx.exit(200)
    else  -- 图片输出
        ngx.header.Content_Type = "image/png"
        ngx.say(qrstr)
        ngx.exit(200)
    end     
end 