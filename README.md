## Nginx QRCode

Ngx_QRCode是基于Lua+ffi+C开发的，生成QRCode的小工具。支持三种模式：①以json形式输出二维码的信息；②下载生成的二维码图片；③直接显示二维码图片。

### 项目结构
```
├── Makefile
├── README.md
├── config
│   ├── config.lua
│   └── nginx.conf
├── content.lua
├── library
│   ├── log.lua
│   ├── qr.lua
│   └── utils.lua
├── logo_style.png
└── main.c
```

* config下面是配置文件，nginx的配置和项目参数的配置。
* `content.lua`是项目的入口文件。
* library文件夹是核心库文件，有loln -s /usr/local/openresty/lualib/libqr.so /usr/lib/g，qr，utils。
* `main.c` 和 `Makefile`是项目中的C相关的。

### 项目依赖

Openresty的安装使用忽略不计！由于包含了C项目，还需要安装依赖有：libqrencode，libpng，libgd。

### 使用
```
git clone git@github.com:dyike/ngx_qrcode.git
cd {project}
make 
mv ./libqr.so /usr/lib/
sudo ldconfig
```
* 将nginx配置修改成自己的目录即可。

### 查看访问
#### json形式输出
```
http://qr.dev/?data=https://www.dyike.com&out_type=json
```

#### 图片显示
```
http://qr.dev/?data=https://www.dyike.com
```

#### 图片下载
```
http://qr.dev/?data=https://www.dyike.com&out_type=download
```