ifeq ($(shell uname -s), Darwin)
	TARGET=libqr.dylib
	INSTALLDIR=/usr/local/lib/
else
	TARGET=libqr.so
	INSTALLDIR=/usr/local/lib64/
endif

all:
	gcc --shared -o $(TARGET) main.c -lpng -lqrencode -lgd -fPIC

install:
	cp $(TARGET) $(INSTALLDIR)

clean:
	rm -rf $(TARGET)

restart:
	/usr/local/openresty/nginx/sbin/nginx -s reload

