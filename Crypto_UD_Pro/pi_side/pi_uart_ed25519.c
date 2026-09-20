/*
 * Raspberry Pi Zero 2 W: проверка привязки к PIC32MM по UART
 * (/dev/serial0), асимметричная схема Ed25519.
 *
 * У PI (код открыт/незащищён) — только ПУБЛИЧНЫЙ ключ. Даже полное
 * вскрытие этого бинаря не даёт извлечь ничего, чем можно прошить
 * клон-контроллер — приватный ключ есть только в PIC32MM (Code Protect).
 *
 * Криптография — вендоренная библиотека orlp/ed25519 (zlib license,
 * чистый ANSI C, см. ../thirdparty/ed25519), та же самая, что и в
 * прошивке PIC32MM, поэтому подпись/проверка побайтово совместимы.
 *
 * Протокол кадра (общий с pic32mm_side/pic32mm_uart_ed25519.c):
 *   STX(0xAA) | CMD(1) | LEN(1) | PAYLOAD(LEN) | CRC16_HI | CRC16_LO
 *   CRC16-CCITT (poly 0x1021, init 0xFFFF) считается по CMD+LEN+PAYLOAD.
 *
 *   CMD_NONCE    0x01  PI -> PIC32MM   payload = 16 случайных байт
 *   CMD_RESPONSE 0x02  PIC32MM -> PI   payload = Ed25519-подпись nonce, 64 байта
 *
 * Проверка запускается периодически (heartbeat), а не только при старте.
 * Результат проверки не просто ставит флаг "ok/not ok" — из подписанного
 * ответа выводится рабочий параметр (derive_control_coefficient), без
 * которого основная логика не может посчитать нужные значения.
 *
 * Сборка (все .c файлы из ../thirdparty/ed25519):
 *   gcc -O2 -Wall -I../thirdparty/ed25519 -o pi_uart_ed25519 \
 *       pi_uart_ed25519.c ../thirdparty/ed25519/[a-z]*.c
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
#include "../thirdparty/ed25519/ed25519.h"

#define UART_DEV        "/dev/serial0"
#define UART_BAUD       B115200
#define HEARTBEAT_SEC   5
#define RX_RETRIES      3

#define STX             0xAA
#define CMD_NONCE       0x01
#define CMD_RESPONSE    0x02
#define NONCE_LEN       16
#define SIG_LEN         64

/*
 * TODO: заменить на публичный ключ из keygen.c (соответствует PRIVATE_KEY
 * в прошивке PIC32MM). Значение ниже — заглушка.
 */
static const uint8_t PUBLIC_KEY[32] = {
    0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,
    0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,
    0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,
    0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F
};

typedef struct {
    bool    valid;
    uint8_t signature[SIG_LEN];
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

static bool recv_response_frame(int fd, uint8_t sig[SIG_LEN])
{
    for (int attempt = 0; attempt < RX_RETRIES; ++attempt) {
        uint8_t b;
        if (!uart_read_byte(fd, &b) || b != STX) continue;

        uint8_t cmd, len;
        if (!uart_read_byte(fd, &cmd)) continue;
        if (!uart_read_byte(fd, &len)) continue;
        if (cmd != CMD_RESPONSE || len != SIG_LEN) continue;

        uint8_t payload[SIG_LEN];
        bool ok = true;
        for (int j = 0; j < SIG_LEN; ++j) {
            if (!uart_read_byte(fd, &payload[j])) { ok = false; break; }
        }
        if (!ok) continue;

        uint8_t crc_hi, crc_lo;
        if (!uart_read_byte(fd, &crc_hi) || !uart_read_byte(fd, &crc_lo)) continue;

        uint8_t crc_buf[2 + SIG_LEN];
        crc_buf[0] = cmd;
        crc_buf[1] = len;
        memcpy(&crc_buf[2], payload, SIG_LEN);
        uint16_t crc = crc16_ccitt(crc_buf, sizeof(crc_buf));
        if (crc_hi != (uint8_t)(crc >> 8) || crc_lo != (uint8_t)(crc & 0xFF)) continue;

        memcpy(sig, payload, SIG_LEN);
        return true;
    }
    return false;
}

static binding_result_t check_binding(int fd)
{
    binding_result_t result = { .valid = false };

    uint8_t nonce[NONCE_LEN];
    if (getrandom(nonce, sizeof(nonce), 0) != (ssize_t)sizeof(nonce)) return result;

    if (!send_nonce_frame(fd, nonce)) return result;

    uint8_t sig[SIG_LEN];
    if (!recv_response_frame(fd, sig)) return result;

    if (!ed25519_verify(sig, nonce, NONCE_LEN, PUBLIC_KEY)) return result;

    memcpy(result.signature, sig, SIG_LEN);
    result.valid = true;
    return result;
}

/* Пример: реальный рабочий параметр, выводимый из подписи контроллера.
 * В прикладном коде на его месте должен быть параметр, без которого
 * основная логика физически не может выполниться (а не просто флаг). */
static double derive_control_coefficient(const uint8_t signature[SIG_LEN])
{
    uint32_t v = ((uint32_t)signature[0] << 24) | ((uint32_t)signature[1] << 16) |
                 ((uint32_t)signature[2] << 8)  |  (uint32_t)signature[3];
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
            double k = derive_control_coefficient(r.signature);
            printf("привязка подтверждена, коэффициент=%f\n", k);
            /* k используется дальше как обязательный параметр реальной
             * рабочей логики приложения. */
        }

        sleep(HEARTBEAT_SEC);
    }
}
