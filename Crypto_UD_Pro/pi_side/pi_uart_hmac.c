/*
 * Raspberry Pi Zero 2 W: challenge-response проверка привязки к PIC32MM
 * по UART (/dev/serial0).
 *
 * Протокол кадра (общий с pic32mm_side/pic32mm_uart_hmac.c):
 *   STX(0xAA) | CMD(1) | LEN(1) | PAYLOAD(LEN) | CRC16_HI | CRC16_LO
 *   CRC16-CCITT (poly 0x1021, init 0xFFFF) считается по CMD+LEN+PAYLOAD.
 *
 *   CMD_NONCE    0x01  PI -> PIC32MM   payload = 16 случайных байт
 *   CMD_RESPONSE 0x02  PIC32MM -> PI   payload = HMAC-SHA256(key, nonce), 32 байта
 *
 * Проверка запускается периодически (heartbeat), а не только при старте.
 * Результат проверки не просто ставит флаг "ok/not ok" — из ответа
 * контроллера выводится рабочий параметр (derive_control_coefficient),
 * без которого основная логика не может посчитать нужные значения.
 *
 * Сборка:
 *   gcc -O2 -Wall -o pi_uart_hmac pi_uart_hmac.c -lcrypto
 *
 * Перед включением UART на Raspberry Pi OS:
 *   raspi-config -> Interface Options -> Serial Port
 *     - login shell over serial: No
 *     - serial port hardware:    Yes
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/random.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>

#define UART_DEV        "/dev/serial0"
#define UART_BAUD       B115200
#define HEARTBEAT_SEC   5
#define RX_RETRIES      3

#define STX             0xAA
#define CMD_NONCE       0x01
#define CMD_RESPONSE    0x02
#define NONCE_LEN       16
#define HMAC_LEN        32

/* Ключ должен совпадать с зашитым в прошивку PIC32MM (SECRET_KEY там же). */
static const uint8_t SECRET_KEY[32] = {
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
    0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
    0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F
};

typedef struct {
    bool    valid;
    uint8_t session_key[HMAC_LEN];
} binding_result_t;

static uint16_t crc16_ccitt(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (int b = 0; b < 8; ++b)
            crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
    }
    return crc;
}

static int uart_open(const char *dev)
{
    int fd = open(dev, O_RDWR | O_NOCTTY);
    if (fd < 0) return -1;

    struct termios tio;
    if (tcgetattr(fd, &tio) != 0) { close(fd); return -1; }

    cfmakeraw(&tio);
    cfsetispeed(&tio, UART_BAUD);
    cfsetospeed(&tio, UART_BAUD);
    tio.c_cc[VMIN]  = 0;
    tio.c_cc[VTIME] = 2; /* 200 мс межбайтовый таймаут чтения */

    if (tcsetattr(fd, TCSANOW, &tio) != 0) { close(fd); return -1; }
    tcflush(fd, TCIOFLUSH);
    return fd;
}

static bool uart_read_byte(int fd, uint8_t *b)
{
    return read(fd, b, 1) == 1;
}

static bool uart_write_all(int fd, const uint8_t *data, size_t len)
{
    size_t off = 0;
    while (off < len) {
        ssize_t n = write(fd, data + off, len - off);
        if (n <= 0) return false;
        off += (size_t)n;
    }
    return true;
}

static bool send_nonce_frame(int fd, const uint8_t *nonce)
{
    uint8_t crc_buf[2 + NONCE_LEN];
    crc_buf[0] = CMD_NONCE;
    crc_buf[1] = NONCE_LEN;
    memcpy(&crc_buf[2], nonce, NONCE_LEN);
    uint16_t crc = crc16_ccitt(crc_buf, sizeof(crc_buf));

    uint8_t frame[1 + 2 + NONCE_LEN + 2];
    size_t i = 0;
    frame[i++] = STX;
    frame[i++] = CMD_NONCE;
    frame[i++] = NONCE_LEN;
    memcpy(&frame[i], nonce, NONCE_LEN); i += NONCE_LEN;
    frame[i++] = (uint8_t)(crc >> 8);
    frame[i++] = (uint8_t)(crc & 0xFF);

    return uart_write_all(fd, frame, i);
}

static bool recv_response_frame(int fd, uint8_t resp[HMAC_LEN])
{
    for (int attempt = 0; attempt < RX_RETRIES; ++attempt) {
        uint8_t b;
        if (!uart_read_byte(fd, &b) || b != STX) continue;

        uint8_t cmd, len;
        if (!uart_read_byte(fd, &cmd)) continue;
        if (!uart_read_byte(fd, &len)) continue;
        if (cmd != CMD_RESPONSE || len != HMAC_LEN) continue;

        uint8_t payload[HMAC_LEN];
        bool ok = true;
        for (int j = 0; j < HMAC_LEN; ++j) {
            if (!uart_read_byte(fd, &payload[j])) { ok = false; break; }
        }
        if (!ok) continue;

        uint8_t crc_hi, crc_lo;
        if (!uart_read_byte(fd, &crc_hi) || !uart_read_byte(fd, &crc_lo)) continue;

        uint8_t crc_buf[2 + HMAC_LEN];
        crc_buf[0] = cmd;
        crc_buf[1] = len;
        memcpy(&crc_buf[2], payload, HMAC_LEN);
        uint16_t crc = crc16_ccitt(crc_buf, sizeof(crc_buf));
        if (crc_hi != (uint8_t)(crc >> 8) || crc_lo != (uint8_t)(crc & 0xFF)) continue;

        memcpy(resp, payload, HMAC_LEN);
        return true;
    }
    return false;
}

static bool const_time_eq(const uint8_t *a, const uint8_t *b, size_t len)
{
    uint8_t diff = 0;
    for (size_t i = 0; i < len; ++i) diff |= (uint8_t)(a[i] ^ b[i]);
    return diff == 0;
}

static binding_result_t check_binding(int fd)
{
    binding_result_t result = { .valid = false };

    uint8_t nonce[NONCE_LEN];
    if (getrandom(nonce, sizeof(nonce), 0) != (ssize_t)sizeof(nonce)) return result;

    if (!send_nonce_frame(fd, nonce)) return result;

    uint8_t resp[HMAC_LEN];
    if (!recv_response_frame(fd, resp)) return result;

    uint8_t expected[HMAC_LEN];
    unsigned int outlen = HMAC_LEN;
    if (!HMAC(EVP_sha256(), SECRET_KEY, sizeof(SECRET_KEY),
              nonce, NONCE_LEN, expected, &outlen)) {
        return result;
    }

    if (!const_time_eq(resp, expected, HMAC_LEN)) return result;

    memcpy(result.session_key, resp, HMAC_LEN);
    result.valid = true;
    return result;
}

/* Пример: реальный рабочий параметр, выводимый из ответа контроллера.
 * В прикладном коде на его месте должен быть параметр, без которого
 * основная логика физически не может выполниться (а не просто флаг). */
static double derive_control_coefficient(const uint8_t session_key[HMAC_LEN])
{
    uint32_t v = ((uint32_t)session_key[0] << 24) | ((uint32_t)session_key[1] << 16) |
                 ((uint32_t)session_key[2] << 8)  |  (uint32_t)session_key[3];
    return 1.0 + (double)(v % 1000) / 100000.0;
}

int main(void)
{
    int fd = uart_open(UART_DEV);
    if (fd < 0) {
        fprintf(stderr, "не удалось открыть %s\n", UART_DEV);
        return 1;
    }

    for (;;) {
        binding_result_t r = check_binding(fd);

        if (!r.valid) {
            fprintf(stderr, "проверка привязки к PIC32MM не пройдена\n");
            /* Здесь должна реально отключаться рабочая логика (обнуление
             * коэффициентов/ключей, без которых основной алгоритм не
             * работает), а не просто пропуск блока по условию. */
        } else {
            double k = derive_control_coefficient(r.session_key);
            printf("привязка подтверждена, коэффициент=%f\n", k);
            /* k используется дальше как обязательный параметр реальной
             * рабочей логики приложения. */
        }

        sleep(HEARTBEAT_SEC);
    }
}
