//
//  main.c
//  QR
//
//  Created by Tsrign on 16/1/5.


#include <stdio.h>
#include <stdlib.h>
#include <qrencode.h>
#include <png.h>
#include <string.h>
#include <gd.h>
#include <stdarg.h>


#define INCHES_PER_METER (100.0/2.54)
#define max(a,b) ((a) > (b) ? (a) : (b))


//错误对照表
#define QR_ERROR_FGCOLOR_COD   1
#define QR_ERROR_FGCOLOR_MSG   "fgcolor error"
#define QR_ERROR_BGCOLOR_COD   2
#define QR_ERROR_BGCOLOR_MSG   "bgcolor error"
#define QR_ERROR_LEVEL_COD     3
#define QR_ERROR_LEVEL_MSG     "level error"
#define QR_ERROR_STR_COD       4
#define QR_ERROR_STR_MSG       "qr data error"
#define QR_ERROR_MARGIN_COD    5
#define QR_ERROR_MARGIN_MSG    "margin error"
#define QR_ERROR_SIZE_COD      6
#define QR_ERROR_SIZE_MSG      "size less than 100"
#define QR_ERROR_DPI_COD       7
#define QR_ERROR_DPI_MSG       "dpi less than 100"
#define QR_ERROR_LOGO_COD      8
#define QR_ERROR_LOGO_MSG      "logo error"
#define QR_ERROR_MEM_COD      10
#define QR_ERROR_MEM_MSG      "mem error"


#define QR_ERROR_MAX_LENGTH 1024
int errcode = 0;
char errmsg[QR_ERROR_MAX_LENGTH+1]; //保存错误信息
static int s_debug = 0;

const char qr_sig_gif[3] = {'G', 'I', 'F'};
const char qr_sig_jpg[3] = {(char) 0xff, (char) 0xd8, (char) 0xff};
const char qr_sig_png[8] = {(char) 0x89, (char) 0x50, (char) 0x4e, (char) 0x47,
    (char) 0x0d, (char) 0x0a, (char) 0x1a, (char) 0x0a};



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
} QR;


void qr_setdebug(){
    s_debug = 1;
}

static void qr_debug(const char* format, ...){
    if(s_debug){
        va_list ap;
        time_t now;
        static struct tm *currentTime;
        char timestr[128];
        char linebuffer[1024];
        
        now =(time_t)0;
        time(&now);
        currentTime = localtime(&now);
        strftime(timestr, 128, "%X", currentTime);
        
        memset(linebuffer, 0, sizeof(linebuffer));
        va_start(ap, format);
        vsprintf((char *)linebuffer, format, ap);
        va_end(ap);
        
        
        FILE *fp = fopen("/tmp/qrso.debug", "a");
        flockfile(fp);
        fprintf(fp, "[ %s ]: ", timestr);
        fprintf(fp, "%s", linebuffer);
        fprintf(fp, "\n");
        fflush(fp);
        funlockfile(fp);
        fclose(fp);
    }
}


/**
 * 解析十六进制至RGB
 */
static int qr_setcolor(unsigned int color[4], char *value){
    int len = (int)strlen(value);
    int count;
    if(len == 6) {
        count = sscanf(value, "%02x%02x%02x%n", &color[0], &color[1], &color[2], &len);
        if(count < 3 || len != 6) {
            return -1;
        }
        color[3] = 255;
    } else if(len == 8) {
        count = sscanf(value, "%02x%02x%02x%02x%n", &color[0], &color[1], &color[2], &color[3], &len);
        if(count < 4 || len != 8) {
            return -1;
        }
    } else {
        return -1;
    }
    return 0;
}

/*
 * 设置错误，便于返回上层
 */
static void qr_seterror(int code ,char *msg){
    errcode = code;
    snprintf(errmsg, QR_ERROR_MAX_LENGTH, "%s", msg);
}


/*
 *  创建png图像时回调
 */
static void qr_png_write_data(png_structp png_ptr, png_bytep data, png_size_t length){
    
    QRbuf *p = (QRbuf *) png_get_io_ptr(png_ptr);
    
    
    if(p->buf != NULL){
        p->len += length;
        p->buf = realloc(p->buf,  p->len);
    }else{
        p->len = length;
        p->buf = malloc(p->len);
    }
    
    memcpy(p->buf+p->len-length, data, length);
}


static void qr_png_flush_data(png_structp png_ptr){
    printf("exec user flush\n");
}



/**
 * 初始化一个QR对象
 * @param str 二维码存储的内容
 * @param size 二维码的尺寸
 * @param margin 二维码边框(包含在二维码尺寸中)
 * @param fgcolor 二维码前景色 000000
 * @param bgcolor 二维码背景色 ffffff
 * @param level 二维码生成级别 H M Q H
 * @param dpi 默认72
 * @param logo logo指向的内存 可以为NULL
 * @param logolen  logo指向内存的大小
 * @param logobg logobg指向的内存, 可以为NULL
 * @param logobglen logobglen logobg指向的内存
 * @param logomargin logo的边框大小
 */

QR* qr_init(char *str, unsigned int size, unsigned int margin, char *fgcolor, char *bgcolor, char level, unsigned int dpi,unsigned int logosize, void *logo, unsigned int logolen, void *logobg, unsigned int logobglen, unsigned int logomargin){

    QR *qr = (QR *)malloc(sizeof(QR));

    
    qr_debug("qr init start");
    
    switch (level) {
        case 'l':
        case 'L':
            qr->level = QR_ECLEVEL_L;
            break;
        case 'm':
        case 'M':
            qr->level = QR_ECLEVEL_M;
            break;
        case 'q':
        case 'Q':
            qr->level = QR_ECLEVEL_Q;
            break;
        case 'h':
        case 'H':
            qr->level = QR_ECLEVEL_H;
            break;
        default:
            qr_seterror(QR_ERROR_LEVEL_COD, QR_ERROR_LEVEL_MSG);
    }
    
    if (*fgcolor == '#') {
        if (qr_setcolor(qr->fg_color, fgcolor+1) != 0) {
            qr_seterror(QR_ERROR_FGCOLOR_COD, QR_ERROR_FGCOLOR_MSG);
            return NULL;
        }
    } else {
        if (qr_setcolor(qr->fg_color, fgcolor) != 0) {
            qr_seterror(QR_ERROR_FGCOLOR_COD, QR_ERROR_FGCOLOR_MSG);
            return NULL;
        }
    }
    
    if (*bgcolor == '#') {
        if(qr_setcolor(qr->bg_color, bgcolor+1) != 0) {
            qr_seterror(QR_ERROR_BGCOLOR_COD, QR_ERROR_BGCOLOR_MSG);
            return NULL;
        }
    } else {
        if (qr_setcolor(qr->bg_color, bgcolor) != 0) {
            qr_seterror(QR_ERROR_BGCOLOR_COD, QR_ERROR_BGCOLOR_MSG);
            return NULL;
        }
    }
    
    
    qr->margin = max(margin, 0);
    qr->size = max(size, 100);
    qr->dpi = max(dpi, 72);
    
    /* logo */
    qr->logosize = max(logosize, 0);
    
    if (logo != NULL) {
        qr->logo = (void *)malloc(logolen);
        
        if (qr->logo == NULL) {
            qr_seterror(QR_ERROR_MEM_COD, QR_ERROR_MEM_MSG);
            return  NULL;
        }

        memcpy(qr->logo, logo, logolen);
        
        qr->logolen = logolen;
    } else {
        qr->logo = NULL;
        qr->logolen = 0;
    }
    
    if (logobg != NULL) {
        qr->logobg = (void *)malloc(logobglen);
        
        if (qr->logobg == NULL) {
            qr_seterror(QR_ERROR_MEM_COD, QR_ERROR_MEM_MSG);
            return  NULL;
        }
        
        memcpy(qr->logobg, logobg, logobglen);

        qr->logobglen = logobglen;
    } else {
        qr->logobg = NULL;
        qr->logobglen = 0;
    }
    
    qr->logomargin = max(logomargin, 0);
    
    qr->qrcode = QRcode_encodeData(strlen(str), str, 0, qr->level);
    
    if (qr->qrcode == NULL) {
        qr_seterror(QR_ERROR_STR_COD, QR_ERROR_STR_MSG);
        return NULL;
    }
    
    qr->qrbuf.len = 0;
    qr->qrbuf.buf = NULL;
    qr_debug("qr init ok");
    return qr;
}

/**
 * 根据QR对象计算出图像
 * @param QR对象指针
 * @return 返回图像的内存大小
 */
int qr_create(QR *qr){
    png_structp png_ptr;
    png_infop info_ptr;
    png_colorp palette;
    png_byte alpha_values[2];
    unsigned char *row, *p, *q;
    int x, y, xx, yy, bit;
    int realwidth;
    int realsize;
    int realmargin;
    QRcode *qrcode;
    
    qr_debug("qr create start");
    
   
    qrcode = qr->qrcode;
    
    realsize = qr->size / qrcode->width + 1;
    realmargin = 0;

    realwidth = (qrcode->width + realmargin * 2) * realsize;
    row = (unsigned char *)malloc((realwidth + 7) / 8);
    if (row == NULL) {
        return -1;
    }
    
    
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    
    if (png_ptr == NULL) {
        return -1;
    }
    
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        return -1;
    }
    
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return -1;
    }
    
    palette = (png_colorp) malloc(sizeof(png_color) * 2);
    if (palette == NULL) {
        return -1;
    }
    palette[0].red   = qr->fg_color[0];
    palette[0].green = qr->fg_color[1];
    palette[0].blue  = qr->fg_color[2];
    palette[1].red   = qr->bg_color[0];
    palette[1].green = qr->bg_color[1];
    palette[1].blue  = qr->bg_color[2];
    alpha_values[0] = qr->fg_color[3];
    alpha_values[1] = qr->bg_color[3];
    png_set_PLTE(png_ptr, info_ptr, palette, 2);
    png_set_tRNS(png_ptr, info_ptr, alpha_values, 2, NULL);
    
    //png_init_io(png_ptr, fp);
    //png_set_write_status_fn(png_ptr, write_row_callback);
    
    png_set_write_fn(png_ptr, &(qr->qrbuf), qr_png_write_data, qr_png_flush_data);
    
    png_set_IHDR(png_ptr, info_ptr,
                 realwidth, realwidth,
                 1,
                 PNG_COLOR_TYPE_PALETTE,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    png_set_pHYs(png_ptr, info_ptr,
                 qr->dpi * INCHES_PER_METER,
                 qr->dpi * INCHES_PER_METER,
                 PNG_RESOLUTION_METER);
    png_write_info(png_ptr, info_ptr);
    
    /* top margin */
    memset(row, 0xff, (realwidth + 7) / 8);
    for (y=0; y<realmargin * realsize; y++) {
        png_write_row(png_ptr, row);
    }
    
    /* data */
    p = qrcode->data;
    for (y=0; y<qrcode->width; y++) {
        bit = 7;
        memset(row, 0xff, (realwidth + 7) / 8);
        q = row;
        q += realmargin * realsize / 8;
        bit = 7 - (realmargin * realsize % 8);
        for (x=0; x<qrcode->width; x++) {
            for (xx=0; xx<realsize; xx++) {
                *q ^= (*p & 1) << bit;
                bit--;
                if (bit < 0) {
                    q++;
                    bit = 7;
                }
            }
            p++;
        }
        for (yy=0; yy<realsize; yy++) {
            png_write_row(png_ptr, row);
        }
    }
    /* bottom margin */
    memset(row, 0xff, (realwidth + 7) / 8);
    for (y=0; y<realmargin * realsize; y++) {
        png_write_row(png_ptr, row);
    }
    
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    
    free(row);
    free(palette);
    
    qr_debug("qr create png ok");
    //调整图片大小，打上logo
  
    
    gdImagePtr im = gdImageCreateTrueColor(qr->size, qr->size);
    
    if (im == NULL) {
        qr_seterror(QR_ERROR_MEM_COD, QR_ERROR_MEM_MSG);
        return -1;
    }
    
    gdImagePtr qrim = gdImageCreateFromPngPtr(qr->qrbuf.len, qr->qrbuf.buf);
    
    if (qrim == NULL) {
        qr_seterror(QR_ERROR_MEM_COD, QR_ERROR_MEM_MSG);
        return -1;
    }
    
    
   
    int fgcolor = gdImageColorAllocate(im, qr->fg_color[0], qr->fg_color[1], qr->fg_color[2]);
    int bgcolor = gdImageColorAllocate(im, qr->bg_color[0], qr->bg_color[1], qr->bg_color[2]);
    
  
    
    
    
    //背景默认为二维码的背景
    gdImageFill(im, 0, 0, bgcolor);
    
    gdImageCopyResized(im, qrim, qr->margin, qr->margin, 0, 0, qr->size-qr->margin*2,
                       qr->size-qr->margin*2, gdImageSX(qrim), gdImageSY(qrim));
    
    
    
    if (qr->logosize > 0) {
        
        if (qr->logobg != NULL && qr->logobglen > 0){ //打上logobg
            gdImagePtr logobgim;
            char *p = qr->logobg;
            if (*p == '#' && strlen(p+1) == 6){ //传递的是颜色
                logobgim = gdImageCreateTrueColor(qr->logosize, qr->logosize);
                unsigned int tmpcolor[4];
                qr_setcolor(tmpcolor, p+1);
                int logobgcolor = gdImageColorAllocate(logobgim, tmpcolor[0], tmpcolor[1], tmpcolor[2]);
                gdImageFill(logobgim, 0, 0, logobgcolor);
            } else { //传递的是图像
                char filetype[8];
                memcpy(filetype, qr->logobg, 8);
               
                if (memcmp(filetype, qr_sig_gif, 3) == 0) {
                    logobgim = gdImageCreateFromGifPtr(qr->logobglen, qr->logobg);
                } else if (memcmp(filetype, qr_sig_jpg, 3) == 0) {
                    logobgim = gdImageCreateFromJpegPtr(qr->logobglen, qr->logobg);
                } else if (memcmp(filetype, qr_sig_png, 8) == 0) {
                    logobgim = gdImageCreateFromPngPtr(qr->logobglen, qr->logobg);
                } else {
                    qr_seterror(QR_ERROR_LOGO_COD, QR_ERROR_LOGO_MSG);
                    return -1;
                }

            }
            
            if (logobgim == NULL) {
                qr_seterror(QR_ERROR_LOGO_COD, QR_ERROR_LOGO_MSG);
                return -3;
            }
            
            //打上logobg
            
            gdImageCopyResized(im, logobgim, (qr->size - qr->logosize)/2, (qr->size - qr->logosize)/2, 0, 0, qr->logosize, qr->logosize, gdImageSX(logobgim), gdImageSY(logobgim));
            
            gdImageDestroy(logobgim);
        }
        
        
        
        if (qr->logo != NULL && qr->logolen > 0) { //打上logo
            gdImagePtr logoim;
            char *p = qr->logo;
            if (*p == '#' && strlen(p+1) == 6) { //颜色
                logoim = gdImageCreateTrueColor(qr->logosize - qr->logomargin, qr->logosize - qr->logomargin);
                unsigned int tmpcolor[4];
                qr_setcolor(tmpcolor, p+1);
                int logobgcolor = gdImageColorAllocate(logoim, tmpcolor[0], tmpcolor[1], tmpcolor[2]);
                gdImageFill(logoim, 0, 0, logobgcolor);
            } else { //图像
                char filetype[8];
                memcpy(filetype, qr->logo, 8);

                if (memcmp(filetype, qr_sig_gif, 3) == 0) {
                    logoim = gdImageCreateFromGifPtr(qr->logolen, qr->logo);
                } else if (memcmp(filetype, qr_sig_jpg, 3) == 0) {
                    logoim = gdImageCreateFromJpegPtr(qr->logolen, qr->logo);
                } else if (memcmp(filetype, qr_sig_png, 8) == 0) {
                    logoim = gdImageCreateFromPngPtr(qr->logolen, qr->logo);
                } else {
                    qr_seterror(QR_ERROR_LOGO_COD, QR_ERROR_LOGO_MSG);
                    return -1;
                }

            }
            
            if (logoim == NULL) {
                qr_seterror(QR_ERROR_LOGO_COD, QR_ERROR_LOGO_MSG);
                return -1;
            }
            
            gdImageSetInterpolationMethod(logoim, GD_POWER);
            
            //打上logo
            gdImageCopyResampled(im, logoim, (qr->size - (qr->logosize - qr->logomargin))/2, (qr->size - (qr->logosize - qr->logomargin))/2, 0, 0, qr->logosize-qr->logomargin, qr->logosize-qr->logomargin, gdImageSX(logoim ), gdImageSY(logoim));
            
            gdImageDestroy(logoim);
            
        }
    }
    


    
    qr_debug("qr create gd ok");
    
    /*
    FILE *fp = fopen("/Users/Tsrign/Projects/C/QR/QR/gd.png", "w");
    gdImagePng(im, fp);
    fclose(fp);
    */
    
    //输出png图像
    free(qr->qrbuf.buf);
    
    
    qr->qrbuf.buf = gdImagePngPtr(im, &(qr->qrbuf.len));
    
    
    gdImageDestroy(im);
    gdImageDestroy(qrim);
    
    
    return qr->qrbuf.len;

}

/**
 * 将QR图像复制到上层缓冲区
 * @param QR对象指针
 * @param 由上层分配和销毁的缓冲区
 * @return void
 */

void qr_save(QR *qr, void *buf){
    qr_debug("qr.save start");
    if (qr->qrbuf.buf != NULL) {
        memcpy(buf, qr->qrbuf.buf, qr->qrbuf.len);
    }
    qr_debug("qr.save ok");
}

/**
 * 对象销毁，释放内存
 */
void qr_cleanup(QR *qr){
    qr_debug("qr.cleanup start");
    //释放qrcode对象
    QRcode_free(qr->qrcode);
    //释放二维码图片内容
    gdFree(qr->qrbuf.buf);
    //释放logo
    if (qr->logo != NULL) {
        free(qr->logo);
    }
    //释放logobg
    if (qr->logobg != NULL) {
        free(qr->logobg);
    }
    //释放二维码本身
    free(qr);
    qr_debug("qr.cleanup ok");
}


int main(int argc, char *argv[]){
    char *str = "xixiasdajksdjkasjdjsadjsak中饭安德安德阿斯顿金卡空间的健康安德安德撒jhaha";
    
    int size1 = 0;
    int size2 = 0;
    
    
    FILE *logofp = fopen("/Users/Tsrign/weibo-logo.png", "r");
    fseek( logofp , 0 , SEEK_END);
    size1 = ftell(logofp);
    fseek( logofp , 0 , SEEK_SET);
    void *logo = malloc(size1);
    fread(logo, size1, 1, logofp);
    fclose(logofp);
    
    FILE *logobgfp = fopen("/Users/Tsrign/border_style.png", "r");
    fseek( logobgfp , 0 , SEEK_END);
    size2 = ftell(logobgfp);
    fseek( logobgfp , 0 , SEEK_SET);
    void *logobg = malloc(size2);
    fread(logobg, size2, 1, logobgfp);
    fclose(logobgfp);
    
    //测试缩放
    
    gdImagePtr srcim;
    gdImagePtr destim;
    srcim = gdImageCreateFromPngPtr(size1, logo);
    destim = gdImageCreateTrueColor(36, 36);
    
    gdImageSetInterpolationMethod(srcim, GD_POWER);
    //gdImageCopyResampled(destim, srcim, 0, 0, 0, 0, 36, 36, gdImageSX(srcim), gdImageSY(srcim));
    destim = gdImageScale(srcim, 36, 36);
    FILE *tfp = fopen("/Users/Tsrign/testlogo.png", "w");
    gdImagePng(destim, tfp);
    fclose(tfp);
    
   
    while(1){
        //qr_init(char *str, unsigned int size, unsigned int margin, char *fgcolor, char *bgcolor, char level, unsigned int dpi,unsigned int logosize, void *logo, unsigned int logolen, void *logobg, unsigned int logobglen, unsigned int logomargin)
       
        
    QR *qr = qr_init(str, 180, 10, "#000000", "#ffffff", 'M', 72, 36, logo, size1, logobg, size2, 16);
    int len = qr_create(qr);

        
        if (len > 0) {
            void *buf = malloc(len);
            qr_save(qr, buf);
            
            FILE *fp = fopen("/Users/Tsrign/qr.png", "w");
            fwrite(buf, len,  1, fp);
            fclose(fp);
            free(buf);
            qr_cleanup(qr);
        } else {
            printf("%d", len);
        }
        break; 
    }    
    return 0;
}

