PLATFORM=$(shell uname | tr '[A-Z]' '[a-z]')-$(shell uname -m)
TARGETDIR=.
TARGET=charlotte-client
TARGETS=$(TARGET)
OBJS=charlotte-client.o nmea_parser.o cJSON.o ws.o epoch.o config.o calibration.o nmea_creator.o truewind.o ais.o #mqtt.o
STATICLIBS=/usr/local/lib/libwebsockets.a /usr/local/lib/libuv.a /usr/lib/aarch64-linux-gnu/libcurl.a
LDFLAGS=-lssl -lcrypto -lm #-lmosquitto
VER=1.6
CFLAGS=-g -Wall -ldl -lpthread -DVERSION=\"$(VER)\" #-DCHAR_DEBUG=1 #-DCHAR_DEBUG2=1 #-DCHAR_DEBUG=1 -DMQTT -pthread
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CPPFLAGS) $(STATIC) -o $(TARGET) $(OBJS) $(STATICLIBS) $(CFLAGS) $(LDFLAGS)

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

calibration.o:	calibration.c
			indent -i4 calibration.c
			$(CC) $(CFLAGS) -c calibration.c

nmea_creator.o:	nmea_creator.c
			indent -i4 nmea_creator.c
			$(CC) $(CFLAGS) -c nmea_creator.c

truewind.o:	truewind.c
			indent -i4 truewind.c
			$(CC) $(CFLAGS) -c truewind.c

mqtt.o:		mqtt.c
			indent -i4 mqtt.c
			$(CC) $(CFLAGS) -c mqtt.c

ais.o:		ais.c
			indent -i4 ais.c
			$(CC) $(CFLAGS) -c ais.c

clean:
	-rm -f $(TARGETS) $(OBJS) *.elf *.gdb

PKGFILE=charlotte-client-$(VER).zip
pkg:
	strip charlotte-client charlotte-logger
	cp LICENSE config.template install.sh start.sh stop.sh purge.sh charlotte-client charlotte-logger actisense-serial analyzer package/charlotte-client		
	rm -f package/$(PKGFILE)
	cd package && zip -r $(PKGFILE) charlotte-client/* 
	cp package/$(PKGFILE) /tmp


