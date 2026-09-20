/*
 * PIC32MM: challenge-response привязка к своему PI по UART, асимметричная
 * схема (Ed25519) — как рекомендовано в исходном md-документе.
 *
 * В этой прошивке — ТОЛЬКО приватный ключ Ed25519 (защищён Code Protect).
 * У PI (открытый код) — только публичный ключ, см. pi_side/pi_uart_ed25519.c.
 * Даже полное вскрытие PI-бинаря не даёт извлечь то, чем можно прошить
 * клон-контроллер.
 *
 * Криптография — не собственная реализация, а вендоренная библиотека
 * orlp/ed25519 (zlib license, чистый ANSI C, см. ../thirdparty/ed25519).
 *
 * Протокол кадра (общий с pi_side/pi_uart_ed25519.c):
 *   STX(0xAA) | CMD(1) | LEN(1) | PAYLOAD(LEN) | CRC16_HI | CRC16_LO
 *   CRC16-CCITT (poly 0x1021, init 0xFFFF) считается по CMD+LEN+PAYLOAD.
 *
 *   CMD_NONCE    0x01  PI -> PIC32MM   payload = 16 случайных байт
 *   CMD_RESPONSE 0x02  PIC32MM -> PI   payload = DEVICE_ID(4) || подпись(64), 68 байт
 *                                      подпись считается над DEVICE_ID || nonce,
 *                                      а не только над nonce — иначе ID можно было
 *                                      бы подменить независимо от подписи.
 *
 * DEVICE_ID — уникальный на экземпляр код, прошиваемый в память программ на
 * производстве вместе с PRIVATE_KEY/PUBLIC_KEY (см. tools/keygen.c). PI
 * запрашивает его тем же обменом, что и подпись, и использует для pairing
 * lock (см. pi_side/pi_uart_ed25519.c) — привязки конкретного PI именно к
 * этому физическому экземпляру PIC32MM, а не к любому "genuine" чипу из
 * той же партии.
 *
 * Ключевая пара генерируется ОДИН РАЗ на доверенном компьютере разработчика
 * инструментом ../tools/keygen.c (никогда не на самом PIC32MM/PI) —
 * см. инструкцию в keygen.c.
 *
 * Компилятор: MPLAB XC32. Перед сборкой:
 *  - добавить в проект все .c/.h из ../thirdparty/ed25519 (стандартный C,
 *    без ассемблерных вставок под конкретную архитектуру);
 *  - настроить конфигурационные биты (в т.ч. Code Protect) через
 *    MPLAB Code Configurator под конкретный PIC32MM;
 *  - назначить пины U1RX/U1TX через Peripheral Pin Select (регистры
 *    RPINRx/RPORx свои для каждого корпуса/варианта чипа);
 *  - подставить в SYS_FREQ реальную частоту периферийной шины;
 *  - заменить PRIVATE_KEY/PUBLIC_KEY на значения из keygen.c.
 */

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include "../thirdparty/ed25519/ed25519.h"

#define SYS_FREQ            24000000UL
#define UART_BAUD           115200UL
#define BYTE_TIMEOUT_LOOPS  200000UL

#define STX             0xAA
#define CMD_NONCE       0x01
#define CMD_RESPONSE    0x02
#define NONCE_LEN       16
#define ID_LEN          4
#define SIG_LEN         64
#define RESP_LEN        (ID_LEN + SIG_LEN)   /* 68 */
#define MAX_PAYLOAD     RESP_LEN

/*
 * TODO: заменить на реальный уникальный ID этого экземпляра, прошиваемый
 * на производстве (серийный номер и т.п.). Значение ниже — заглушка.
 */
static const uint8_t DEVICE_ID[ID_LEN] = { 0x00, 0x00, 0x00, 0x01 };

/*
 * TODO: заменить на реальную ключевую пару, сгенерированную keygen.c.
 * PRIVATE_KEY (64 байта) должен присутствовать ТОЛЬКО в этой прошивке,
 * под Code Protect. Значения ниже — заглушка, для реального изделия
 * непригодны.
 */
static const uint8_t PRIVATE_KEY[64] = {
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
    0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
    0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
    0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
    0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
    0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F
};

static const uint8_t PUBLIC_KEY[32] = {
    0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,
    0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,
    0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,
    0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F
};

/* ---------------------------- UART1 (регистровый доступ) --------------- */

static void uart1_init(void)
{
    U1MODE = 0;
    U1STA  = 0;
    U1MODEbits.BRGH = 1; /* высокоскоростной режим, делитель x4 */
    U1BRG  = (uint16_t)((SYS_FREQ / (4UL * UART_BAUD)) - 1UL);
    U1STAbits.UTXEN = 1;
    U1STAbits.URXEN = 1;
    U1MODEbits.ON = 1;
}

static void uart1_putc(uint8_t c)
{
    while (U1STAbits.UTXBF);
    U1TXREG = c;
}

static bool uart1_getc_timeout(uint8_t *c, uint32_t timeout_loops)
{
    while (!U1STAbits.URXDA) {
        if (--timeout_loops == 0) return false;
    }
    *c = (uint8_t)U1RXREG;
    return true;
}

/* ------------------------------- Кадры --------------------------------- */

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

static bool rx_frame(uint8_t *cmd, uint8_t *payload, uint8_t *len)
{
    uint8_t b;

    if (!uart1_getc_timeout(&b, BYTE_TIMEOUT_LOOPS) || b != STX) return false;
    if (!uart1_getc_timeout(cmd, BYTE_TIMEOUT_LOOPS)) return false;
    if (!uart1_getc_timeout(len, BYTE_TIMEOUT_LOOPS)) return false;
    if (*len > MAX_PAYLOAD) return false;

    for (uint8_t i = 0; i < *len; ++i)
        if (!uart1_getc_timeout(&payload[i], BYTE_TIMEOUT_LOOPS)) return false;

    uint8_t crc_hi, crc_lo;
    if (!uart1_getc_timeout(&crc_hi, BYTE_TIMEOUT_LOOPS)) return false;
    if (!uart1_getc_timeout(&crc_lo, BYTE_TIMEOUT_LOOPS)) return false;

    uint8_t crc_buf[2 + MAX_PAYLOAD];
    crc_buf[0] = *cmd;
    crc_buf[1] = *len;
    memcpy(&crc_buf[2], payload, *len);
    uint16_t crc = crc16_ccitt(crc_buf, (size_t)(2 + *len));

    return crc_hi == (uint8_t)(crc >> 8) && crc_lo == (uint8_t)(crc & 0xFF);
}

static void tx_frame(uint8_t cmd, const uint8_t *payload, uint8_t len)
{
    uint8_t crc_buf[2 + MAX_PAYLOAD];
    crc_buf[0] = cmd;
    crc_buf[1] = len;
    memcpy(&crc_buf[2], payload, len);
    uint16_t crc = crc16_ccitt(crc_buf, (size_t)(2 + len));

    uart1_putc(STX);
    uart1_putc(cmd);
    uart1_putc(len);
    for (uint8_t i = 0; i < len; ++i) uart1_putc(payload[i]);
    uart1_putc((uint8_t)(crc >> 8));
    uart1_putc((uint8_t)(crc & 0xFF));
}

/* -------------------------------- main ---------------------------------- */

int main(void)
{
    uart1_init();

    for (;;) {
        uint8_t cmd, len, payload[MAX_PAYLOAD];

        if (!rx_frame(&cmd, payload, &len)) continue;
        if (cmd != CMD_NONCE || len != NONCE_LEN) continue;

        uint8_t msg[ID_LEN + NONCE_LEN];
        memcpy(msg, DEVICE_ID, ID_LEN);
        memcpy(msg + ID_LEN, payload, NONCE_LEN);

        uint8_t resp[RESP_LEN];
        memcpy(resp, DEVICE_ID, ID_LEN);
        ed25519_sign(resp + ID_LEN, msg, sizeof(msg), PUBLIC_KEY, PRIVATE_KEY);

        tx_frame(CMD_RESPONSE, resp, sizeof(resp));
    }
}
