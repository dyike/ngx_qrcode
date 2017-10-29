local ffi = require "ffi"

local _M = {}

local mt = {__index=M}

ffi.cdef[[
void *malloc(int size);
void free(void *p);

int errcode;
char errmsg[1025];

typedef enum {
    QR_ECLEVEL_L = 0,
    QR_ECLEVEL_M,
    QR_ECLEVEL_Q,
    QR_ECLEVEL_H      
} QRecLevel;


typedef enum {
    QR_MODE_NUL = -1,
    QR_MODE_NUM = 0,
    QR_MODE_AN,
    QR_MODE_8,
    QR_MODE_KANJI,
    QR_MODE_STRUCTURE,
    QR_MODE_ECI,
    QR_MODE_FNC1FIRST,
    QR_MODE_FNC1SECOND,
} QRencodeMode;

typedef struct {
	int version;         ///< version of the symbol
	int width;           ///< width of the symbol
	unsigned char *data; ///< symbol data
} QRcode;

typedef struct{
    void *buf;
    int len;
}QRbuf;

typedef struct {
    unsigned int size; //设置二维码尺寸
    unsigned int margin; //设置二维码的边框
    unsigned int dpi; //dpi
    unsigned int fg_color[4]; //设置前景色
    unsigned int bg_color[4]; //设置背景色
    unsigned int logosize; //logo 尺寸
    unsigned int logomargin; //logo margin
    void *logo; //logo指向的内存
    unsigned int logolen; //logo占用内存大小
    void *logobg; //logobg指向的内存
    unsigned int logobglen; //logobg指向的内存大小
    QRecLevel level; //编码级别
    QRencodeMode hint; //编码模式
    QRbuf qrbuf;
    QRcode *qrcode;
}QR;

QR* qr_init(char *str, unsigned int size, unsigned int margin, char *fgcolor, char *bgcolor, char level, unsigned int dpi,unsigned int logosize, void *logo, unsigned int logolen, void *logobg, unsigned int logobglen, unsigned int logomargin);
int qr_create(QR *qr);
void qr_save(QR *qr, void *buf);
void qr_cleanup(QR *qr);
void qr_setdebug();
]]

local qr = ffi.load("qr")

--
--@Description: Create QR CODE
--@param: data 二维码内容
--@param: size 二维码尺寸
--@param: margin 二维码边框(包含在二维码尺寸中) 
--@param: fgcolor 前景色 默认：000000
--@param: bgcolor 背景色 默认: ffffff
--@param: level  二维码级别 L, M, Q, H
--@param: dpi 默认为72
--@param: logo 二维码logo地址
--@param: logoscale 二维码尺寸
--QR* qr_init(char *str, unsigned int size, unsigned int margin, char *fgcolor, char *bgcolor, char level, unsigned int dpi,unsigned int logosize, void *logo, unsigned int logolen, void *logobg, unsigned int logobglen, unsigned int logomargin);
function _M.generate(data, size, margin, fgcolor, bgcolor, level, dpi, logosize, logo, logobg, logomargin)
    local cdata     = ffi.new("char[?]", #data, data)
    local csize     = tonumber(size)
    local cmargin   = tonumber(margin)
    local cfgcolor  = ffi.new("char[?]", #fgcolor, fgcolor)
    local cbgcolor  = ffi.new("char[?]", #bgcolor, bgcolor)
    local clevel    = ffi.new("char", string.byte(level))
    local cdpi      = tonumber(dpi)
    local clogosize = tonumber(logosize)
   
   
    local clogo, clogolen, clogobg, clogobglen

    if logo == nil then
        clogo = nil
        clogolen = 0
    else
        clogo = ffi.new("char[?]", #logo, logo)
        clogolen = #logo
    end

    if logobg == nil then
        clogobg = nil
        clogobglen = 0
    else
        clogobg = ffi.new("char[?]", #logobg, logobg)
        clogobglen = #logobg
    end


    local clogomargin = tonumber(logomargin)


    local qrp = qr.qr_init(cdata, csize, cmargin, cfgcolor, cbgcolor, clevel, cdpi, clogosize, clogo, clogolen, clogobg, clogobglen, clogomargin)

    if qrp == nil then
        return nil , qr.errcode, ffi.string(qr.errmsg)
    end

    local len = qr.qr_create(qrp)

    if len <= 0 then
            return nil, qr.errcode, ffi.string(qr.errmsg)
    end

    local qrbuf = ffi.C.malloc(len)
    qr.qr_save(qrp, qrbuf)
    local qrstr = ffi.string(qrbuf, len)

    ffi.C.free(qrbuf)
    qr.qr_cleanup(qrp)

    return qrstr, 0, "success"
end

--@Description: Scan Code
--@param img 图片
function _M.scan(img)

end


--@Descripion: Debug Mode
--@param: void
function _M.debug()
    qr.qr_setdebug()
end

return _M
