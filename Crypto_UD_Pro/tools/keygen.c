/*
 * Генератор ключевой пары Ed25519 для привязки PI <-> PIC32MM.
 *
 * Запускать ОДИН РАЗ на доверенном компьютере разработчика (никогда на
 * самом PI или PIC32MM, никогда через сеть). Вывод — готовые C-массивы:
 *  - PRIVATE_KEY вставляется в pic32mm_side/pic32mm_uart_ed25519.c
 *    (и должен существовать только там, под Code Protect);
 *  - PUBLIC_KEY вставляется в pi_side/pi_uart_ed25519.c (безопасно
 *    хранить в открытом виде).
 *
 * Если нужна отдельная ключевая пара на каждое изделие (чтобы вскрытие
 * одного PIC32MM аппаратной атакой не компрометировало всю линейку) —
 * запускайте генератор отдельно на каждую пару PI/PIC32MM и вести
 * сопоставление серийный_номер -> PUBLIC_KEY на стороне PI отдельным
 * защищённым конфигом, а не одной статической константой в коде.
 *
 * Сборка (все .c файлы из ../thirdparty/ed25519):
 *   gcc -O2 -o keygen keygen.c ../thirdparty/ed25519/[a-z]*.c -I../thirdparty/ed25519
 * Запуск:
 *   ./keygen > keys.txt
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include "../thirdparty/ed25519/ed25519.h"

static void print_array(const char *name, const uint8_t *data, size_t len)
{
    printf("static const uint8_t %s[%zu] = {\n    ", name, len);
    for (size_t i = 0; i < len; ++i) {
        printf("0x%02X,", data[i]);
        if ((i + 1) % 8 == 0 && i + 1 != len) printf("\n    ");
    }
    printf("\n};\n\n");
}

int main(void)
{
    unsigned char seed[32], pub[32], priv[64];

    if (ed25519_create_seed(seed) != 0) {
        fprintf(stderr, "не удалось получить случайный seed из ОС\n");
        return 1;
    }
    ed25519_create_keypair(pub, priv, seed);

    printf("/* --- pic32mm_side/pic32mm_uart_ed25519.c: PRIVATE_KEY --- */\n");
    print_array("PRIVATE_KEY", priv, sizeof(priv));

    printf("/* --- pic32mm_side/pic32mm_uart_ed25519.c и\n");
    printf("       pi_side/pi_uart_ed25519.c: PUBLIC_KEY --- */\n");
    print_array("PUBLIC_KEY", pub, sizeof(pub));

    memset(seed, 0, sizeof(seed));
    memset(priv, 0, sizeof(priv));
    return 0;
}
