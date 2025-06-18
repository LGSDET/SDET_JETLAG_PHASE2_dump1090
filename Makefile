# OpenSSL 경로 자동 설정 (Mac과 Linux 모두 대응)
OPENSSL_INC := $(shell if [ -d /opt/homebrew/opt/openssl@3/include ]; then echo -I/opt/homebrew/opt/openssl@3/include; else echo -I/usr/include/openssl; fi)
OPENSSL_LIB := $(shell if [ -d /opt/homebrew/opt/openssl@3/lib ]; then echo -L/opt/homebrew/opt/openssl@3/lib; else echo -L/usr/lib; fi)
RTLSDR_INC := -I$(shell brew --prefix librtlsdr)/include
JSONC_INC := -I$(shell brew --prefix json-c)/include/json-c

CFLAGS ?= -O2 -g -Wall -Wno-unused-result $(shell pkg-config --cflags librtlsdr) $(OPENSSL_INC) -I/opt/homebrew/include
CFLAGS += $(RTLSDR_INC)
CFLAGS += $(JSONC_INC)

CFLAGS += --coverage
LDFLAGS += --coverage

LDLIBS += $(shell pkg-config --libs librtlsdr) $(OPENSSL_LIB) -L/opt/homebrew/lib -lssl -lcrypto -lpthread -lm -ljson-c -lrtlsdr

CC ?= gcc
PROGNAME = dump1090

all: dump1090

%.o: %.c
	$(CC) $(CFLAGS) -c $<

dump1090: dump1090.o anet.o
	$(CC) -g -o dump1090 dump1090.o anet.o $(LDFLAGS) $(LDLIBS)

# ✅ test: 모든 테스트 (기본 + mock + accept 통합 실행)
test: test_dump1090 test_tls_mock test_accept_clients
	@echo ""
	@echo "🧪 Running Unit Tests..."
	./test_tls_mock
	./test_accept_clients
	./test_dump1090

# 기본 dump1090 유닛 테스트
test_dump1090: Unity/unity.o tests/UnitTest_dump1090.o dump1090.o anet.o
	$(CC) -g -o test_dump1090 Unity/unity.o tests/UnitTest_dump1090.o dump1090.o anet.o $(LDFLAGS) $(LDLIBS) -lgcov

# Mock 테스트 (init_tls)
test_tls_mock: tests/test_tls_mock.o Unity/unity.o
	$(CC) -g -o test_tls_mock tests/test_tls_mock.o Unity/unity.o $(LDFLAGS) $(LDLIBS) -lgcov

# accept 클라이언트 처리 테스트
test_accept_clients: tests/test_accept_clients.o Unity/unity.o
	$(CC) -g -o test_accept_clients tests/test_accept_clients.o Unity/unity.o $(LDFLAGS) $(LDLIBS) -lgcov

Unity/unity.o: Unity/unity.c
	$(CC) $(CFLAGS) -c Unity/unity.c -o Unity/unity.o

tests/UnitTest_dump1090.o: tests/UnitTest_dump1090.c
	$(CC) $(CFLAGS) -I. -IUnity $(OPENSSL_INC) -c tests/UnitTest_dump1090.c -o tests/UnitTest_dump1090.o

tests/test_tls_mock.o: tests/test_tls_mock.c
	$(CC) $(CFLAGS) -IUnity -c tests/test_tls_mock.c -o tests/test_tls_mock.o

tests/test_accept_clients.o: tests/test_accept_clients.c
	$(CC) $(CFLAGS) -IUnity -c tests/test_accept_clients.c -o tests/test_accept_clients.o

dump1090.o: dump1090.c
	$(CC) $(CFLAGS) -DUNIT_TEST -c dump1090.c

anet.o: anet.c
	$(CC) $(CFLAGS) -c anet.c -o anet.o

clean:
	rm -f *.o dump1090 test_dump1090 test_tls_mock test_accept_clients Unity/*.o tests/*.o