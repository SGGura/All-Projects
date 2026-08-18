#include "png_write.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t crc_table[256];
static int crc_ready;

static void crc_init(void)
{
    uint32_t n, k, c;

    for (n = 0; n < 256; n++) {
        c = n;
        for (k = 0; k < 8; k++) {
            c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        }
        crc_table[n] = c;
    }
    crc_ready = 1;
}

static uint32_t crc32_buf(const uint8_t *p, size_t n, uint32_t crc)
{
    if (!crc_ready) {
        crc_init();
    }
    while (n--) {
        crc = crc_table[(crc ^ *p++) & 0xFFu] ^ (crc >> 8);
    }
    return crc;
}

static uint32_t adler32_buf(const uint8_t *p, size_t n)
{
    uint32_t a = 1, b = 0;

    while (n--) {
        a = (a + *p++) % 65521u;
        b = (b + a) % 65521u;
    }
    return (b << 16) | a;
}

static void put_be32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static int write_chunk(FILE *f, const char *type, const uint8_t *data, uint32_t len)
{
    uint8_t hdr[8];
    uint8_t crcbuf[4];
    uint32_t crc;

    put_be32(hdr, len);
    memcpy(hdr + 4, type, 4);
    if (fwrite(hdr, 1, 8, f) != 8) {
        return -1;
    }
    if (len && fwrite(data, 1, len, f) != len) {
        return -1;
    }

    crc = crc32_buf((const uint8_t *)type, 4, 0xFFFFFFFFu);
    crc = crc32_buf(data, len, crc) ^ 0xFFFFFFFFu;
    put_be32(crcbuf, crc);

    return (fwrite(crcbuf, 1, 4, f) == 4) ? 0 : -1;
}

int png_write_rgb565(const char *path, const uint16_t *px, int w, int h)
{
    static const uint8_t sig[8] = {137, 'P', 'N', 'G', 13, 10, 26, 10};
    FILE *f;
    uint8_t ihdr[13];
    uint8_t *raw, *z;
    size_t raw_len, z_len, pos, off;
    int x, y, rc = -1;

    if (w <= 0 || h <= 0) {
        return -1;
    }

    raw_len = (size_t)h * (1u + 3u * (size_t)w);
    raw = (uint8_t *)malloc(raw_len);
    if (!raw) {
        return -1;
    }

    pos = 0;
    for (y = 0; y < h; y++) {
        raw[pos++] = 0; /* filter: none */
        for (x = 0; x < w; x++) {
            uint16_t c = px[(size_t)y * w + x];
            uint8_t r5 = (uint8_t)((c >> 11) & 0x1F);
            uint8_t g6 = (uint8_t)((c >> 5) & 0x3F);
            uint8_t b5 = (uint8_t)(c & 0x1F);

            raw[pos++] = (uint8_t)((r5 << 3) | (r5 >> 2));
            raw[pos++] = (uint8_t)((g6 << 2) | (g6 >> 4));
            raw[pos++] = (uint8_t)((b5 << 3) | (b5 >> 2));
        }
    }

    /* zlib stream made of stored (uncompressed) deflate blocks */
    z_len = 2 + 4 + raw_len + 5 * ((raw_len + 65534) / 65535);
    z = (uint8_t *)malloc(z_len);
    if (!z) {
        free(raw);
        return -1;
    }

    pos = 0;
    z[pos++] = 0x78;
    z[pos++] = 0x01;
    off = 0;
    while (off < raw_len) {
        size_t n = raw_len - off;
        int final;

        if (n > 65535) {
            n = 65535;
        }
        final = (off + n >= raw_len) ? 1 : 0;

        z[pos++] = (uint8_t)final;
        z[pos++] = (uint8_t)(n & 0xFF);
        z[pos++] = (uint8_t)(n >> 8);
        z[pos++] = (uint8_t)(~n & 0xFF);
        z[pos++] = (uint8_t)((~n >> 8) & 0xFF);
        memcpy(z + pos, raw + off, n);
        pos += n;
        off += n;
    }
    put_be32(z + pos, adler32_buf(raw, raw_len));
    pos += 4;

    f = fopen(path, "wb");
    if (!f) {
        goto done;
    }
    if (fwrite(sig, 1, 8, f) != 8) {
        goto close_done;
    }

    put_be32(ihdr, (uint32_t)w);
    put_be32(ihdr + 4, (uint32_t)h);
    ihdr[8] = 8;   /* bit depth  */
    ihdr[9] = 2;   /* truecolour */
    ihdr[10] = 0;
    ihdr[11] = 0;
    ihdr[12] = 0;

    if (write_chunk(f, "IHDR", ihdr, sizeof(ihdr)) != 0) {
        goto close_done;
    }
    if (write_chunk(f, "IDAT", z, (uint32_t)pos) != 0) {
        goto close_done;
    }
    if (write_chunk(f, "IEND", NULL, 0) != 0) {
        goto close_done;
    }
    rc = 0;

close_done:
    fclose(f);
done:
    free(z);
    free(raw);
    return rc;
}
