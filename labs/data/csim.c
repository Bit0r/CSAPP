#include "cachelab.h"
#include <getopt.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    int s, b;
    size_t S, E;
    char *filename;

    char opt;
    while ((opt = getopt(argc, argv, "s:E:b:t:")) != -1) {
        extern char *optarg;
        switch (opt) {
        case 's':
            s = atoi(optarg);
            S = 1 << s;
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            filename = optarg;
            break;
        default:
            break;
        }
    }

    size_t cache[S][E];
    memset(cache, 0, S * E * 8);
    size_t mask_valid = LONG_MIN, mask_index = S - 1;

    int hit_count, miss_count, eviction_count;
    hit_count = miss_count = eviction_count = 0;

    FILE *fp = fopen(filename, "r");
    char operation;
    size_t address;
    int size;
    while (fscanf(fp, " %c %zx,%d", &operation, &address, &size) == 3) {
        if (operation == 'I') {
            continue;
        }
        printf("%c %zx,%d", operation, address, size);

        bool is_hit = false;
        int index = (address >> b) & mask_index;
        size_t new_block = mask_valid | (address >> (s + b));

        int i;
        for (i = 0; i < E && (cache[index][i] & mask_valid); i++) {
            if (new_block == cache[index][i]) {
                is_hit = true;
                break;
            }
        }

        if (is_hit) {
            hit_count++;
            printf(" hit");
        } else {
            miss_count++;
            printf(" miss");
        }
        if (i == E) {
            eviction_count++;
            i--;
            printf(" eviction");
        }
        if (operation == 'M') {
            hit_count++;
            printf(" hit");
        }

        for (; i > 0; i--) {
            cache[index][i] = cache[index][i - 1];
        }
        cache[index][0] = new_block;

        putchar('\n');
    }
    fclose(fp);

    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
