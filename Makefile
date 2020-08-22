PLATFORM=$(shell uname | tr '[A-Z]' '[a-z]')-$(shell uname -m)
TARGETDIR=.
TARGET=charlotte-client
TARGETS=$(TARGET)
OBJS=charlotte-client.o nmea_parser.o cJSON.o ws.o
LDFLAGS=-lcurl -lwebsockets
VER=0.0.10
CFLAGS=-g -Wall -DVERSION=\"$(VER)\"
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LDLIBS$(LDLIBS-$(@)))

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

clean:
	-rm -f $(TARGETS) $(OBJS) *.elf *.gdb

PKGFILE=charlotte-client-$(VER).tar.gz
LIBS=/usr/local/lib/libwsclient.so.0.0.0 /usr/local/lib/libwsclient.la /usr/local/lib/libcurl.so.4.6.0 /usr/local/lib/libcurl.la
pkg:
	mkdir -p package/charlotte-client/lib
	cp config.template install.sh start.sh stop.sh charlotte-client charlotte-logger actisense-serial analyzer package/charlotte-client		
	cp $(LIBS) package/charlotte-client/lib
	rm -f package/$(PKGFILE)
	cd package && tar zcvf $(PKGFILE) charlotte-client/* 
	cp package/$(PKGFILE) /tmp


