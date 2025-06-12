<<<<<<< HEAD
#CFLAGS?=-O2 -g -Wall -W $(shell pkg-config --cflags librtlsdr)
#LDLIBS+=$(shell pkg-config --libs librtlsdir) -lpthread -lm -lssl -lcrypto
#OpenSSL 경로 자동 설정 (Mac과 Linux 모두 대응)
OPENSSL_INC := $(shell if [ -d /opt/homebrew/opt/openssl@3/include ]; then echo -I/opt/homebrew/opt/openssl@3/include; else echo -I/usr/include/openssl; fi)
OPENSSL_LIB := $(shell if [ -d /opt/homebrew/opt/openssl@3/lib ]; then echo -L/opt/homebrew/opt/openssl@3/lib; else echo -L/usr/lib; fi)

CFLAGS ?= -O2 -g -Wall -W $(shell pkg-config --cflags librtlsdr) $(OPENSSL_INC) -I/usr/include/json-c
LDLIBS += $(shell pkg-config --libs librtlsdr) $(OPENSSL_LIB) -lssl -lcrypto -lpthread -lm -ljson-c

CC?=gcc
PROGNAME=dump1090

all: dump1090

%.o: %.c
	$(CC) $(CFLAGS) -c $<

dump1090: dump1090.o anet.o
	$(CC) -g -o dump1090 dump1090.o anet.o $(LDFLAGS) $(LDLIBS)

clean:
	rm -f *.o dump1090
=======
LIBUSB_INC_PATH=/usr/local/Cellar/libusb/1.0.9/include/libusb-1.0
LIBUSB_LIB_PATH=/usr/local/Cellar/libusb/1.0.9/lib
LIBRTLSDR_INC_PATH=/usr/local/Cellar/rtlsdr/HEAD/include
LIBRTLSDR_LIB_PATH=/usr/local/Cellar/rtlsdr/HEAD/lib
LIBS=-lusb-1.0 -lrtlsdr -lpthread -lm
CC=gcc
PROGNAME=mode1090

all: mode1090

mode1090.o: mode1090.c
	$(CC) -O2 -g -Wall -W -I$(LIBUSB_INC_PATH) -I$(LIBRTLSDR_INC_PATH) mode1090.c -c -o mode1090.o

mode1090: mode1090.o
	$(CC) -g -L$(LIBUSB_LIB_PATH) -L$(LIBRTLSDR_LIB_PATH) -o mode1090 mode1090.o $(LIBS)

clean:
	rm -f *.o mode1090
>>>>>>> 7ca5a4b (Initial commit of Dump1090, a simple Mode S decoder.)
