/*
 * PIC32MM: challenge-response привязка к паре PI по UART.
 *
 * Протокол кадра (общий с pi_side/pi_uart_hmac.c):
 *   STX(0xAA) | CMD(1) | LEN(1) | PAYLOAD(LEN) | CRC16_HI | CRC16_LO
 *   CRC16-CCITT (poly 0x1021, init 0xFFFF) считается по CMD+LEN+PAYLOAD.
 *
 *   CMD_NONCE    0x01  PI -> PIC32MM   payload = 16 случайных байт
 *   CMD_RESPONSE 0x02  PIC32MM -> PI   payload = HMAC-SHA256(key, nonce), 32 байта
 *
 * Компилятор: MPLAB XC32. Перед сборкой:
 *  - настроить конфигурационные биты (в т.ч. Code Protect) через
 *    MPLAB Code Configurator под конкретный PIC32MM;
 *  - назначить пины U1RX/U1TX через Peripheral Pin Select (регистры
 *    RPINRx/RPORx свои для каждого корпуса/варианта чипа);
 *  - подставить в SYS_FREQ реальную частоту периферийной шины.
 */

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>

#define SYS_FREQ            24000000UL
#define UART_BAUD           115200UL
#define BYTE_TIMEOUT_LOOPS  200000UL

#define STX             0xAA
#define CMD_NONCE       0x01
#define CMD_RESPONSE    0x02
#define NONCE_LEN       16
#define HMAC_LEN        32
#define MAX_PAYLOAD     64

/* Ключ должен быть уникальным на изделие и присутствовать только в
 * закрытой прошивке (code-protect). Значение ниже — заглушка. */
static const uint8_t SECRET_KEY[32] = {
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
    0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
    0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F
};

/* ---------------- SHA-256 / HMAC-SHA256 (без аппаратного ускорителя) --- */

typedef struct {
    uint8_t  data[64];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t state[8];
} sha256_ctx_t;

static const uint32_t sha256_k[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

#define ROTR(x,n)  (((x) >> (n)) | ((x) << (32-(n))))
#define CH(x,y,z)  (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x)     (ROTR(x,2) ^ ROTR(x,13) ^ ROTR(x,22))
#define EP1(x)     (ROTR(x,6) ^ ROTR(x,11) ^ ROTR(x,25))
#define SIG0(x)    (ROTR(x,7) ^ ROTR(x,18) ^ ((x) >> 3))
#define SIG1(x)    (ROTR(x,17) ^ ROTR(x,19) ^ ((x) >> 10))

static void sha256_transform(sha256_ctx_t *ctx, const uint8_t data[64])
{
    uint32_t m[64];
    uint32_t a, b, c, d, e, f, g, h, t1, t2;
    int i, j;

    for (i = 0, j = 0; i < 16; ++i, j += 4)
        m[i] = ((uint32_t)data[j] << 24) | ((uint32_t)data[j+1] << 16) |
               ((uint32_t)data[j+2] << 8) | (uint32_t)data[j+3];
    for ( ; i < 64; ++i)
        m[i] = SIG1(m[i-2]) + m[i-7] + SIG0(m[i-15]) + m[i-16];

    a = ctx->state[0]; b = ctx->state[1]; c = ctx->state[2]; d = ctx->state[3];
    e = ctx->state[4]; f = ctx->state[5]; g = ctx->state[6]; h = ctx->state[7];

    for (i = 0; i < 64; ++i) {
        t1 = h + EP1(e) + CH(e,f,g) + sha256_k[i] + m[i];
        t2 = EP0(a) + MAJ(a,b,c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    ctx->state[0] += a; ctx->state[1] += b; ctx->state[2] += c; ctx->state[3] += d;
    ctx->state[4] += e; ctx->state[5] += f; ctx->state[6] += g; ctx->state[7] += h;
}

static void sha256_init(sha256_ctx_t *ctx)
{
    ctx->datalen = 0;
    ctx->bitlen  = 0;
    ctx->state[0] = 0x6a09e667; ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372; ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f; ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab; ctx->state[7] = 0x5be0cd19;
}

static void sha256_update(sha256_ctx_t *ctx, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        ctx->data[ctx->datalen++] = data[i];
        if (ctx->datalen == 64) {
            sha256_transform(ctx, ctx->data);
            ctx->bitlen += 512;
            ctx->datalen = 0;
        }
    }
}

static void sha256_final(sha256_ctx_t *ctx, uint8_t hash[32])
{
    uint32_t i = ctx->datalen;

    if (ctx->datalen < 56) {
        ctx->data[i++] = 0x80;
        while (i < 56) ctx->data[i++] = 0;
    } else {
        ctx->data[i++] = 0x80;
        while (i < 64) ctx->data[i++] = 0;
        sha256_transform(ctx, ctx->data);
        memset(ctx->data, 0, 56);
    }

    ctx->bitlen += (uint64_t)ctx->datalen * 8;
    for (i = 0; i < 8; ++i)
        ctx->data[63 - i] = (uint8_t)(ctx->bitlen >> (8 * i));
    sha256_transform(ctx, ctx->data);

    for (i = 0; i < 4; ++i) {
        for (int j = 0; j < 8; ++j)
            hash[j*4 + i] = (uint8_t)(ctx->state[j] >> (24 - i*8));
    }
}

static void hmac_sha256(const uint8_t *key, size_t keylen,
                         const uint8_t *msg, size_t msglen,
                         uint8_t out[32])
{
    uint8_t k_ipad[64], k_opad[64], tk[32], inner[32];
    sha256_ctx_t ctx;

    if (keylen > 64) {
        sha256_init(&ctx);
        sha256_update(&ctx, key, keylen);
        sha256_final(&ctx, tk);
        key = tk;
        keylen = 32;
    }

    memset(k_ipad, 0x36, 64);
    memset(k_opad, 0x5c, 64);
    for (size_t i = 0; i < keylen; ++i) {
        k_ipad[i] ^= key[i];
        k_opad[i] ^= key[i];
    }

    sha256_init(&ctx);
    sha256_update(&ctx, k_ipad, 64);
    sha256_update(&ctx, msg, msglen);
    sha256_final(&ctx, inner);

    sha256_init(&ctx);
    sha256_update(&ctx, k_opad, 64);
    sha256_update(&ctx, inner, 32);
    sha256_final(&ctx, out);
}

/* --------------------------- UART1 (регистровый доступ) --------------- */

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

        uint8_t resp[HMAC_LEN];
        hmac_sha256(SECRET_KEY, sizeof(SECRET_KEY), payload, NONCE_LEN, resp);
        tx_frame(CMD_RESPONSE, resp, sizeof(resp));
    }
}
