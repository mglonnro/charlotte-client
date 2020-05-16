PLATFORM=$(shell uname | tr '[A-Z]' '[a-z]')-$(shell uname -m)
TARGETDIR=.
TARGET=charlotte-client
TARGETS=$(TARGET)
OBJS=charlotte-client.o nmea_parser.o cJSON.o wsclient.o
LDFLAGS=-lcurl -lwsclient
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


