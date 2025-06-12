// tests/UnitTest_dump1090.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include "unity.h"

// 필요한 함수들만 선언
extern uint32_t modesChecksum(unsigned char *msg, int bits);
extern int modesMessageLenByType(int type);
extern int fixSingleBitErrors(unsigned char *msg, int bits);
extern int verifyPassword(const char *received_password);

#define MODES_LONG_MSG_BITS 112
#define MODES_SHORT_MSG_BITS 56
#define MODES_LONG_MSG_BYTES 14

// ANSI 색상 코드
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// 테스트용 임시 config 파일 경로
#define TEST_CONFIG_PATH "tests/test_config.json"
#define TEST_PASSWORD "testpassword"
#define TEST_SALT "testsalt"

// 해시 계산 함수
char *calculate_hash(const char *password, const char *salt) {
    static char hex_hash[EVP_MAX_MD_SIZE * 2 + 1];
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    char salted_password[512];
    
    snprintf(salted_password, sizeof(salted_password), "%s%s", password, salt);
    
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    const EVP_MD *md = EVP_sha256();
    
    EVP_DigestInit_ex(mdctx, md, NULL);
    EVP_DigestUpdate(mdctx, salted_password, strlen(salted_password));
    EVP_DigestFinal_ex(mdctx, hash, &hash_len);
    EVP_MD_CTX_free(mdctx);
    
    for (unsigned int i = 0; i < hash_len; i++) {
        sprintf(hex_hash + (i * 2), "%02x", hash[i]);
    }
    hex_hash[hash_len * 2] = '\0';
    
    return hex_hash;
}

void setUp(void) {
    printf(ANSI_COLOR_BLUE "\n=== Running Test: %s === \n" ANSI_COLOR_RESET, Unity.CurrentTestName);
    
    // 테스트용 config.json 생성
    if (Unity.CurrentTestName && strstr(Unity.CurrentTestName, "test_verifyPassword")) {
        system("mkdir -p tests");
        FILE *fp = fopen(TEST_CONFIG_PATH, "w");
        if (fp) {
            const char *hash = calculate_hash(TEST_PASSWORD, TEST_SALT);
            fprintf(fp, "{\n"
                       "  \"admin_password_hash\": \"%s\",\n"
                       "  \"salt\": \"%s\"\n"
                       "}\n",
                       hash, TEST_SALT);
            fclose(fp);
        }
    }
}

void tearDown(void) {
    if (Unity.CurrentTestFailed) {
        printf(ANSI_COLOR_RED "✗ Test Failed\n" ANSI_COLOR_RESET);
    } else {
        printf(ANSI_COLOR_GREEN "✓ Test Passed\n" ANSI_COLOR_RESET);
    }
    
    // 테스트용 config 파일 정리
    if (Unity.CurrentTestName && strstr(Unity.CurrentTestName, "test_verifyPassword")) {
        unlink(TEST_CONFIG_PATH);
    }
}

void test_modesChecksum(void) {
    unsigned char msg[MODES_LONG_MSG_BYTES] = {0};
    // 예시: 모든 0인 메시지의 CRC는 0이어야 함
    TEST_ASSERT_EQUAL_UINT32(0, modesChecksum(msg, 112));
}

void test_modesMessageLenByType(void) {
    TEST_ASSERT_EQUAL_INT(MODES_LONG_MSG_BITS, modesMessageLenByType(17));
    TEST_ASSERT_EQUAL_INT(MODES_SHORT_MSG_BITS, modesMessageLenByType(11));
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

void test_verifyPassword_correct(void) {
    // 올바른 비밀번호로 테스트
    TEST_ASSERT_EQUAL_INT(1, verifyPassword(TEST_PASSWORD));
}

void test_verifyPassword_incorrect(void) {
    // 잘못된 비밀번호로 테스트
    TEST_ASSERT_EQUAL_INT(0, verifyPassword("wrongpassword"));
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
    TEST_ASSERT_EQUAL_INT(0, verifyPassword("pass'word\"`;--"));
}

void test_verifyPassword_binary_data(void) {
    // NULL 바이트가 포함된 비밀번호
    char binary_password[] = "pass\0word";
    TEST_ASSERT_EQUAL_INT(0, verifyPassword(binary_password));
}

void test_verifyPassword_missing_config(void) {
    // config 파일이 없는 경우
    unlink(TEST_CONFIG_PATH);
    TEST_ASSERT_EQUAL_INT(0, verifyPassword(TEST_PASSWORD));
}

void test_verifyPassword_invalid_json(void) {
    // 잘못된 형식의 JSON
    FILE *fp = fopen(TEST_CONFIG_PATH, "w");
    if (fp) {
        fprintf(fp, "{ invalid json }");
        fclose(fp);
    }
    TEST_ASSERT_EQUAL_INT(0, verifyPassword(TEST_PASSWORD));
}

void test_verifyPassword_missing_hash(void) {
    // hash 필드가 없는 JSON
    FILE *fp = fopen(TEST_CONFIG_PATH, "w");
    if (fp) {
        fprintf(fp, "{ \"salt\": \"%s\" }", TEST_SALT);
        fclose(fp);
    }
    TEST_ASSERT_EQUAL_INT(0, verifyPassword(TEST_PASSWORD));
}

void test_verifyPassword_missing_salt(void) {
    // salt 필드가 없는 JSON
    FILE *fp = fopen(TEST_CONFIG_PATH, "w");
    if (fp) {
        fprintf(fp, "{ \"admin_password_hash\": \"%s\" }", 
               calculate_hash(TEST_PASSWORD, TEST_SALT));
        fclose(fp);
    }
    TEST_ASSERT_EQUAL_INT(0, verifyPassword(TEST_PASSWORD));
}

void test_verifyPassword_empty_hash(void) {
    // 빈 해시값
    FILE *fp = fopen(TEST_CONFIG_PATH, "w");
    if (fp) {
        fprintf(fp, "{ \"admin_password_hash\": \"\", \"salt\": \"%s\" }", 
               TEST_SALT);
        fclose(fp);
    }
    TEST_ASSERT_EQUAL_INT(0, verifyPassword(TEST_PASSWORD));
}

void test_verifyPassword_empty_salt(void) {
    // 빈 salt값
    FILE *fp = fopen(TEST_CONFIG_PATH, "w");
    if (fp) {
        const char *hash = calculate_hash(TEST_PASSWORD, "");
        fprintf(fp, "{ \"admin_password_hash\": \"%s\", \"salt\": \"\" }", hash);
        fclose(fp);
    }
    TEST_ASSERT_EQUAL_INT(0, verifyPassword(TEST_PASSWORD));
}

void test_verifyPassword_non_hex_hash(void) {
    // 16진수가 아닌 해시값
    FILE *fp = fopen(TEST_CONFIG_PATH, "w");
    if (fp) {
        fprintf(fp, "{ \"admin_password_hash\": \"not-a-hex-hash\", \"salt\": \"%s\" }", 
               TEST_SALT);
        fclose(fp);
    }
    TEST_ASSERT_EQUAL_INT(0, verifyPassword(TEST_PASSWORD));
}

int main(void) {
    printf(ANSI_COLOR_YELLOW "\n🛫 Starting dump1090 Unit Tests 🛬\n" ANSI_COLOR_RESET);
    printf("----------------------------------------\n");
    
    UNITY_BEGIN();
    RUN_TEST(test_modesChecksum);
    RUN_TEST(test_modesMessageLenByType);
    RUN_TEST(test_fixSingleBitErrors);
    RUN_TEST(test_verifyPassword_correct);
    RUN_TEST(test_verifyPassword_incorrect);
    RUN_TEST(test_verifyPassword_empty);
    RUN_TEST(test_verifyPassword_null);
    RUN_TEST(test_verifyPassword_long_password);
    RUN_TEST(test_verifyPassword_special_chars);
    RUN_TEST(test_verifyPassword_binary_data);
    RUN_TEST(test_verifyPassword_missing_config);
    RUN_TEST(test_verifyPassword_invalid_json);
    RUN_TEST(test_verifyPassword_missing_hash);
    RUN_TEST(test_verifyPassword_missing_salt);
    RUN_TEST(test_verifyPassword_empty_hash);
    RUN_TEST(test_verifyPassword_empty_salt);
    RUN_TEST(test_verifyPassword_non_hex_hash);
    
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