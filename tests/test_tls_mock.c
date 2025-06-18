#include "unity.h"
#include <string.h>


#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// Mocked OpenSSL TLS functions
typedef struct {} SSL_CTX;

int mock_SSL_CTX_new_called = 0;
int mock_cert_file_result = 1;
int mock_key_file_result = 1;
int mock_key_check_result = 1;

SSL_CTX *mock_tls_ctx = (SSL_CTX*)0x1; // dummy pointer
SSL_CTX *tls_ctx = NULL; // to be used by init_tls

SSL_CTX* SSL_CTX_new(const void* method) {
    mock_SSL_CTX_new_called = 1;
    return mock_tls_ctx;
}

int SSL_CTX_use_certificate_file(SSL_CTX *ctx, const char *file, int type) {
    return mock_cert_file_result;
}

int SSL_CTX_use_PrivateKey_file(SSL_CTX *ctx, const char *file, int type) {
    return mock_key_file_result;
}

int SSL_CTX_check_private_key(SSL_CTX *ctx) {
    return mock_key_check_result;
}

// Target function mock (assumes SSL_CTX_new result goes to tls_ctx)
void init_tls(const char *cert, const char *key) {
    mock_SSL_CTX_new_called = 0;
    tls_ctx = SSL_CTX_new(NULL);
    if (!tls_ctx) return;

    if (!SSL_CTX_use_certificate_file(tls_ctx, cert, 1)) {
        tls_ctx = NULL;
        return;
    }

    if (!SSL_CTX_use_PrivateKey_file(tls_ctx, key, 1)) {
        tls_ctx = NULL;
        return;
    }

    if (!SSL_CTX_check_private_key(tls_ctx)) {
        tls_ctx = NULL;
        return;
    }
}

// Unity test setup
void setUp(void) {
    printf(ANSI_COLOR_BLUE "\n=== Running Test: %s ===\n" ANSI_COLOR_RESET, Unity.CurrentTestName);
    mock_SSL_CTX_new_called = 0;
    tls_ctx = NULL;
    mock_cert_file_result = 1;
    mock_key_file_result = 1;
    mock_key_check_result = 1;
}

void tearDown(void) {
    if (Unity.CurrentTestFailed) {
        printf(ANSI_COLOR_RED "✗ Test Failed\n" ANSI_COLOR_RESET);
    } else {
        printf(ANSI_COLOR_GREEN "✓ Test Passed\n" ANSI_COLOR_RESET);
    }
}

void test_init_tls_should_call_SSL_CTX_new(void) {
    init_tls("cert.pem", "key.pem");
    TEST_ASSERT_TRUE(mock_SSL_CTX_new_called);
}

void test_init_tls_should_fail_if_cert_invalid(void) {
    mock_cert_file_result = 0;
    init_tls("bad_cert.pem", "key.pem");
    TEST_ASSERT_NULL(tls_ctx);
}

void test_init_tls_should_fail_if_key_invalid(void) {
    mock_key_file_result = 0;
    init_tls("cert.pem", "bad_key.pem");
    TEST_ASSERT_NULL(tls_ctx);
}

void test_init_tls_should_fail_if_key_check_fails(void) {
    mock_key_check_result = 0;
    init_tls("cert.pem", "key.pem");
    TEST_ASSERT_NULL(tls_ctx);
}

void test_init_tls_should_succeed_if_all_valid(void) {
    init_tls("cert.pem", "key.pem");
    TEST_ASSERT_EQUAL_PTR(mock_tls_ctx, tls_ctx);
}

// Unity 메인 실행 함수
int main(void) {
    printf(ANSI_COLOR_YELLOW "\n🛫 Starting TLS Mock Tests 🛬\n" ANSI_COLOR_RESET);
    printf("----------------------------------------\n");
    UNITY_BEGIN();
    RUN_TEST(test_init_tls_should_call_SSL_CTX_new);
    RUN_TEST(test_init_tls_should_fail_if_cert_invalid);
    RUN_TEST(test_init_tls_should_fail_if_key_invalid);
    RUN_TEST(test_init_tls_should_fail_if_key_check_fails);
    RUN_TEST(test_init_tls_should_succeed_if_all_valid);
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