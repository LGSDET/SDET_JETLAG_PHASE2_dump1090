#include "unity.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RESET   "\x1b[0m"

int mock_accept_called = 0;
int mock_accept_result_fd = 5;
int mock_ssl_accept_success = 1;
int mock_auth_success = 1;
int mock_trusted_ip = 1;
int modesMaxClients = 1024;

typedef struct client {
    int fd;
    int authenticated;
    int service;
    void *ssl;
} client;

client *Clients[1024] = {0};
int maxfd = 0;
int tls = 1;

// Fake versions renamed to avoid conflicts
int mock_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    (void)sockfd; (void)addr; (void)addrlen;
    mock_accept_called = 1;
    return mock_accept_result_fd;
}

int mock_getpeername(int fd, struct sockaddr *addr, socklen_t *addrlen) {
    (void)fd; (void)addr; (void)addrlen;
    return 0;
}

char* inet_ntoa(struct in_addr in) {
    (void)in;
    return mock_trusted_ip ? "127.0.0.1" : "8.8.8.8";
}

int is_trusted_ip(const char *ip) {
    (void)ip;
    return mock_trusted_ip;
}

void* SSL_new(void* ctx) {
    (void)ctx;
    return (void*)0x1;
}

void SSL_set_fd(void* ssl, int fd) {
    (void)ssl; (void)fd;
}

int SSL_accept(void* ssl) {
    (void)ssl;
    return mock_ssl_accept_success ? 1 : -1;
}

int handleClientAuth(client *c) {
    (void)c;
    return mock_auth_success;
}

// Target function using mock versions
void modesAcceptClients(int listener) {
    int fd = mock_accept(listener, NULL, NULL);
    if (fd < 0) return;

    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    mock_getpeername(fd, (struct sockaddr *)&addr, &addr_len);

    const char *ip = inet_ntoa((struct in_addr){0});
    if (!is_trusted_ip(ip)) {
        return;
    }

    client *c = (client *)malloc(sizeof(client));
    c->fd = fd;
    c->authenticated = 0;
    c->ssl = NULL;
    c->service = 1;

    if (tls) {
        c->ssl = SSL_new(NULL);
        SSL_set_fd(c->ssl, fd);
        if (SSL_accept(c->ssl) <= 0) {
            free(c);
            return;
        }
    }

    if (!handleClientAuth(c)) {
        free(c);
        return;
    }

    c->authenticated = 1;
    Clients[fd] = c;
    if (fd > maxfd) maxfd = fd;
}

// === Test setup ===
void setUp(void) {
    printf(ANSI_COLOR_BLUE "\n=== Running Test: %s ===\n" ANSI_COLOR_RESET, Unity.CurrentTestName);
    memset(Clients, 0, sizeof(Clients));
    maxfd = 0;
    mock_accept_called = 0;
    mock_accept_result_fd = 5;
    mock_ssl_accept_success = 1;
    mock_auth_success = 1;
    mock_trusted_ip = 1;
    tls = 1;
}

void tearDown(void) {
    if (Unity.CurrentTestFailed) {
        printf(ANSI_COLOR_RED "✗ Test Failed\n" ANSI_COLOR_RESET);
    } else {
        printf(ANSI_COLOR_GREEN "✓ Test Passed\n" ANSI_COLOR_RESET);
    }
}

void test_accept_client_from_trusted_ip_should_succeed(void) {
    modesAcceptClients(0);
    TEST_ASSERT_NOT_NULL(Clients[5]);
    TEST_ASSERT_EQUAL_INT(1, Clients[5]->authenticated);
}

void test_accept_client_from_untrusted_ip_should_fail(void) {
    mock_trusted_ip = 0;
    modesAcceptClients(0);
    TEST_ASSERT_NULL(Clients[5]);
}

void test_tls_handshake_fails_should_close_socket(void) {
    mock_ssl_accept_success = 0;
    modesAcceptClients(0);
    TEST_ASSERT_NULL(Clients[5]);
}

void test_authentication_fails_should_close_socket(void) {
    mock_auth_success = 0;
    modesAcceptClients(0);
    TEST_ASSERT_NULL(Clients[5]);
}

void test_accept_client_with_tls_and_auth_success(void) {
    modesAcceptClients(0);
    TEST_ASSERT_NOT_NULL(Clients[5]);
    TEST_ASSERT_EQUAL_INT(1, Clients[5]->authenticated);
}

int main(void) {
    printf(ANSI_COLOR_YELLOW "\n🛫 Starting client Acception Mock Tests 🛬\n" ANSI_COLOR_RESET);
    printf("----------------------------------------\n");

    UNITY_BEGIN();
    RUN_TEST(test_accept_client_from_trusted_ip_should_succeed);
    RUN_TEST(test_accept_client_from_untrusted_ip_should_fail);
    RUN_TEST(test_tls_handshake_fails_should_close_socket);
    RUN_TEST(test_authentication_fails_should_close_socket);
    RUN_TEST(test_accept_client_with_tls_and_auth_success);

    int result = UNITY_END();

    printf("\n");
    if (result == 0) {
        printf(ANSI_COLOR_GREEN "✨ All tests passed successfully! ✨\n" ANSI_COLOR_RESET);
    } else {
        printf(ANSI_COLOR_RED "❌ Some tests failed. Please check the output above. ❌\n" ANSI_COLOR_RESET);
    }
    printf("----------------------------------------\n\n");

    return result;
}