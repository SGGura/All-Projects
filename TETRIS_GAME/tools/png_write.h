#ifndef PNG_WRITE_H
#define PNG_WRITE_H

#include <stdint.h>

/* Writes an RGB565 buffer as a 24-bit PNG (uncompressed deflate blocks, so no
   zlib dependency). Returns 0 on success. Test tooling only. */
int png_write_rgb565(const char *path, const uint16_t *px, int w, int h);

#endif /* PNG_WRITE_H */
