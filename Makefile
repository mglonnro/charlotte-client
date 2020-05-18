PLATFORM=$(shell uname | tr '[A-Z]' '[a-z]')-$(shell uname -m)
TARGETDIR=.
TARGET=charlotte-client
TARGETS=$(TARGET)
OBJS=charlotte-client.o nmea_parser.o cJSON.o wsclient.o
LDFLAGS=-lcurl -lwsclient
CFLAGS=-g
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LDLIBS$(LDLIBS-$(@)))

charlotte-client.o:	charlotte-client.c
			$(CC) $(CFLAGS) -c charlotte-client.c

nmea_parser.o:	nmea_parser.c
			$(CC) $(CFLAGS) -c nmea_parser.c

cJSON.o:	cJSON.c
			$(CC) $(CFLAGS) -c cJSON.c

wsclient.o:	wsclient.c
			$(CC) $(CFLAGS) -c wsclient.c

clean:
	-rm -f $(TARGETS) $(OBJS) *.elf *.gdb

VER=0.0.2
PKGFILE=charlotte-client-$(VER).tar.gz
LIBS=/usr/local/lib/libwsclient.so.0.0.0 /usr/local/lib/libwsclient.la /usr/local/lib/libcurl.so.4.6.0 /usr/local/lib/libcurl.la
pkg:
	mkdir -p package/charlotte-client/lib
	cp config.template install.sh start.sh stop.sh charlotte-client actisense-serial analyzer package/charlotte-client		
	cp $(LIBS) package/charlotte-client/lib
	rm -f package/$(PKGFILE)
	cd package && tar zcvf $(PKGFILE) charlotte-client/*


