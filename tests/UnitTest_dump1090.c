#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include "unity.h"

// 필요한 함수 선언
extern uint32_t modesChecksum(unsigned char *msg, int bits);
extern int modesMessageLenByType(int type);
extern int fixSingleBitErrors(unsigned char *msg, int bits);
extern int fixTwoBitsErrors(unsigned char *msg, int bits);
extern int verifyPassword(const char *received_password);
extern int is_trusted_ip(const char* ip);

#define MODES_LONG_MSG_BITS 112
#define MODES_SHORT_MSG_BITS 56
#define MODES_LONG_MSG_BYTES 14

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RESET   "\x1b[0m"

void setUp(void) {
    printf(ANSI_COLOR_BLUE "\n=== Running Test: %s ===\n" ANSI_COLOR_RESET, Unity.CurrentTestName);
}

void tearDown(void) {
    if (Unity.CurrentTestFailed) {
        printf(ANSI_COLOR_RED "✗ Test Failed\n" ANSI_COLOR_RESET);
    } else {
        printf(ANSI_COLOR_GREEN "✓ Test Passed\n" ANSI_COLOR_RESET);
    }
}

void test_modesChecksum(void) {
    unsigned char msg[MODES_LONG_MSG_BYTES] = {0,};
    // 예시: 모든 0인 메시지의 CRC는 0이어야 함
    TEST_ASSERT_EQUAL_UINT32(0, modesChecksum(msg, 112));
}

void test_fixSingleBitErrors(void) {
    unsigned char msg[MODES_LONG_MSG_BYTES] = {0};
    // CRC가 맞는 메시지에 bit error를 일부러 넣고, 복구되는지 확인
    msg[0] = 0x8a; // 임의의 값
    uint32_t crc = modesChecksum(msg, 112);
    msg[11] = (crc >> 16) & 0xFF;
    msg[12] = (crc >> 8) & 0xFF;
    msg[13] = crc & 0xFF;
    // 1비트 에러 삽입
    msg[2] ^= 0x01;
    int fixed = fixSingleBitErrors(msg, 112);
    TEST_ASSERT_NOT_EQUAL(-1, fixed);
}

void test_fixTwoBitsErrors(void) {
    unsigned char msg[MODES_LONG_MSG_BYTES] = {0};
    unsigned char original[MODES_LONG_MSG_BYTES];

    // 메시지를 임의의 값으로 세팅 (CRC 포함)
    msg[0] = 0x8a; // arbitrary value
    uint32_t crc = modesChecksum(msg, 112);
    msg[11] = (crc >> 16) & 0xFF;
    msg[12] = (crc >> 8) & 0xFF;
    msg[13] = crc & 0xFF;

    // 원본 메시지 백업
    memcpy(original, msg, MODES_LONG_MSG_BYTES);

    // 서로 다른 두 비트에 오류 삽입
    msg[1] ^= 0x01;  // flip bit in 2nd byte
    msg[2] ^= 0x02;  // flip different bit in 3rd byte

    // 함수 호출
    int result = fixTwoBitsErrors(msg, 112);

    // 결과 확인
    TEST_ASSERT_NOT_EQUAL(-1, result);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(original, msg, MODES_LONG_MSG_BYTES);
}

void test_modesMessageLenByType_long(void) {
    TEST_ASSERT_EQUAL_INT(MODES_LONG_MSG_BITS, modesMessageLenByType(16));   
    TEST_ASSERT_EQUAL_INT(MODES_LONG_MSG_BITS, modesMessageLenByType(17));
    TEST_ASSERT_EQUAL_INT(MODES_LONG_MSG_BITS, modesMessageLenByType(18));
    TEST_ASSERT_EQUAL_INT(MODES_LONG_MSG_BITS, modesMessageLenByType(19));
    TEST_ASSERT_EQUAL_INT(MODES_LONG_MSG_BITS, modesMessageLenByType(20));
    TEST_ASSERT_EQUAL_INT(MODES_LONG_MSG_BITS, modesMessageLenByType(21));
    TEST_ASSERT_EQUAL_INT(MODES_LONG_MSG_BITS, modesMessageLenByType(22));
    TEST_ASSERT_EQUAL_INT(MODES_LONG_MSG_BITS, modesMessageLenByType(23));
    TEST_ASSERT_EQUAL_INT(MODES_LONG_MSG_BITS, modesMessageLenByType(24));
}

void test_modesMessageLenByType_short(void) {
    TEST_ASSERT_EQUAL_INT(MODES_SHORT_MSG_BITS, modesMessageLenByType(0));
    TEST_ASSERT_EQUAL_INT(MODES_SHORT_MSG_BITS, modesMessageLenByType(4));
    TEST_ASSERT_EQUAL_INT(MODES_SHORT_MSG_BITS, modesMessageLenByType(5));
    TEST_ASSERT_EQUAL_INT(MODES_SHORT_MSG_BITS, modesMessageLenByType(11));
}

void test_verifyPassword_correct(void) {
     // 올바른 비밀번호로 테스트
    TEST_ASSERT_EQUAL_INT(1, verifyPassword("admin"));
}

void test_verifyPassword_incorrect(void) {
    // 잘못된 비밀번호로 테스트
    TEST_ASSERT_EQUAL_INT(0, verifyPassword("wrongpassword"));
}

void test_verifyPassword_capital(void) {
    // 잘못된 비밀번호로 테스트
    TEST_ASSERT_EQUAL_INT(0, verifyPassword("ADMIN"));
}
void test_verifyPassword_empty(void) {
    // 빈 비밀번호로 테스트
    TEST_ASSERT_EQUAL_INT(0, verifyPassword(""));
}

void test_verifyPassword_null(void) {
    // NULL 비밀번호로 테스트
    TEST_ASSERT_EQUAL_INT(0, verifyPassword(NULL));
}

void test_verifyPassword_long_password(void) {
    // 매우 긴 비밀번호로 버퍼 오버플로우 시도
    char long_password[1024];
    memset(long_password, 'A', sizeof(long_password)-1);
    long_password[sizeof(long_password)-1] = '\0';
    TEST_ASSERT_EQUAL_INT(0, verifyPassword(long_password));
}

void test_verifyPassword_special_chars(void) {
    // 특수 문자가 포함된 비밀번호
    TEST_ASSERT_EQUAL_INT(0, verifyPassword("ad'min\"`;--"));
}

void test_verifyPassword_binary_data(void) {
    // NULL 바이트가 포함된 비밀번호
    char binary_password[] = "ad\0min";
    TEST_ASSERT_EQUAL_INT(0, verifyPassword(binary_password));
}

void test_is_trusted_ip_should_return_true_for_localhost(void) {
    TEST_ASSERT_EQUAL_INT(1, is_trusted_ip("127.0.0.1"));
}

void test_is_trusted_ip_should_return_true_for_whitelisted_ip(void) {
    TEST_ASSERT_EQUAL_INT(1, is_trusted_ip("192.168.0.100"));
}

void test_is_trusted_ip_should_return_false_for_non_whitelisted_ip(void) {
    TEST_ASSERT_EQUAL_INT(0, is_trusted_ip("10.0.0.1"));
    TEST_ASSERT_EQUAL_INT(0, is_trusted_ip("8.8.8.8"));
}



int main(void) {
    printf(ANSI_COLOR_YELLOW "\n🛫 Starting dump1090 Unit Tests 🛬\n" ANSI_COLOR_RESET);
    printf("----------------------------------------\n");

    UNITY_BEGIN();
    RUN_TEST(test_modesChecksum);
    RUN_TEST(test_fixSingleBitErrors);
    RUN_TEST(test_fixTwoBitsErrors);
    RUN_TEST(test_modesMessageLenByType_long);
    RUN_TEST(test_modesMessageLenByType_short);
    RUN_TEST(test_verifyPassword_correct);
    RUN_TEST(test_verifyPassword_incorrect);
    RUN_TEST(test_verifyPassword_capital);
    RUN_TEST(test_verifyPassword_empty);
    RUN_TEST(test_verifyPassword_null);
    RUN_TEST(test_verifyPassword_long_password);
    RUN_TEST(test_verifyPassword_special_chars);
    RUN_TEST(test_verifyPassword_binary_data);
    RUN_TEST(test_is_trusted_ip_should_return_true_for_localhost);
    RUN_TEST(test_is_trusted_ip_should_return_true_for_whitelisted_ip);
    RUN_TEST(test_is_trusted_ip_should_return_false_for_non_whitelisted_ip);
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
