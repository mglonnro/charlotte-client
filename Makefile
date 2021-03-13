PLATFORM=$(shell uname | tr '[A-Z]' '[a-z]')-$(shell uname -m)
TARGETDIR=.
TARGET=charlotte-client
TARGETS=$(TARGET)
OBJS=charlotte-client.o nmea_parser.o cJSON.o ws.o epoch.o config.o
STATICLIBS=/usr/local/lib/libwebsockets.a /usr/local/lib/libcurl.a /usr/local/lib/libuv.a
LDFLAGS=-lssl -lcrypto
VER=0.0.12
CFLAGS=-g -Wall -ldl -lpthread -DVERSION=\"$(VER)\" #-DCHAR_DEBUG=1 #-DCHAR_DEBUG2=1 #-DCHAR_DEBUG=1
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(STATIC) $(LDFLAGS) -o $(TARGET) $(OBJS) $(STATICLIBS)

charlotte-client.o:	charlotte-client.c
			indent -i4 charlotte-client.c
			$(CC) $(CFLAGS) -c charlotte-client.c

nmea_parser.o:	nmea_parser.c
			indent -i4 nmea_parser.c
			$(CC) $(CFLAGS) -c nmea_parser.c

cJSON.o:	cJSON.c
			$(CC) $(CFLAGS) -c cJSON.c

ws.o:	ws.c
			indent -i4 ws.c
			$(CC) $(CFLAGS) -c ws.c

epoch.o:	epoch.c
			indent -i4 epoch.c
			$(CC) $(CFLAGS) -c epoch.c

config.o:	config.c
			indent -i4 config.c
			$(CC) $(CFLAGS) -c config.c

clean:
	-rm -f $(TARGETS) $(OBJS) *.elf *.gdb

PKGFILE=charlotte-client-$(VER).zip
pkg:
	strip charlotte-client charlotte-logger
	cp config.template install.sh start.sh stop.sh charlotte-client charlotte-logger actisense-serial analyzer package/charlotte-client		
	rm -f package/$(PKGFILE)
	cd package && zip -r $(PKGFILE) charlotte-client/* 
	cp package/$(PKGFILE) /tmp


