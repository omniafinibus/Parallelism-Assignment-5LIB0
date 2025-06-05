#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct ranges {
  uint32_t start;
  uint32_t mask;
} ranges_t;

ranges_t *list = NULL;

uint32_t list_length = 0;

int find_mask(uint32_t t) {
  uint32_t c = t;
  uint8_t i = 0;
  for (i = 0; i < 32; i++) {
    if ((c & 1) != 0) {
      break;
    }
    c >>= 1;
  }

  return ((uint32_t)(pow(2, i) - 1));
}

void find_args(uint32_t start, uint32_t end) {
  uint32_t start2 = start;
  if (start2 < end) {
    uint32_t lowermask = find_mask(start2);

    if ((start2 + lowermask + 1) > end) {
      lowermask = find_mask(end);

      list = realloc(list, (list_length + 1) * sizeof(ranges_t));
      list[list_length].start = start2;
      list[list_length].mask = ~lowermask;

      list_length++;
    } else {
      list = realloc(list, (list_length + 1) * sizeof(ranges_t));
      list[list_length].start = start2;
      list[list_length].mask = ~lowermask;
      list_length++;
    }
    start2 += lowermask + 1;
    find_args(start2, end);
  }
}
void sort() {
  int changed = 0;
  do {
    changed = 0;
    for (int i = 0; i < list_length - 1; i++) {
      if (list[i].start > list[i + 1].start) {
        ranges_t tmp = list[i + 1];
        list[i + 1] = list[i];
        list[i] = tmp;
        changed = 1;
      }
    }
  } while (changed);

  while (list_length > 0 && list[0].start == 0) {
    list++;
    list_length--;
  }
}

int main(int argc, char **argv) {
  if (argc != 3)
    return 0;
  uint32_t start = strtoul(argv[1], NULL, 0);
  uint32_t length = strtoul(argv[2], NULL, 0);

  uint32_t end = start + length;
  find_args(start, end);

  for (int i = 0; i < list_length; i++) {
    printf("%08X %08X\n", list[i].start, list[i].mask);
  }
  int changed = 0;
  do {
    changed = 0;
    for (int i = 0; i < list_length; i++) {
      for (int j = i + 1; j < list_length; j++) {
        if (list[j].start != 0) {
          if (list[i].mask == list[j].mask) {
            uint32_t x = (~list[i].start & list[j].start);
            if ((x | list[i].start) == list[j].start) {
              list[i].mask &= ~x;
              list[j].start = 0;
              list[j].mask = 0;
              changed = 1;
            }
          }
        }
      }
    }
    sort();
  } while (changed);

  printf("-----------------------\n");
  for (int i = 0; i < list_length; i++) {
    printf("%08X %08X\n", list[i].start, list[i].mask);
  }

  printf("---------- validate\n");
  uint32_t lowermask = find_mask(start);
  uint32_t lowermask2 = find_mask(end);
  if (lowermask2 < lowermask)
    lowermask = lowermask2;
  printf("set size: %u\n", lowermask + 1);
  for (uint64_t s = 0; s < UINT32_MAX; s += (lowermask + 1)) {
    int found = 0;
    for (int i = 0; i < list_length; i++) {
      if ((s & list[i].mask) == list[i].start) {
        found = 1;
      }
    }
    if ((s >= start && s < (start + length))) {
      if (found == 0) {
        printf("%08X is not matched\n", (uint32_t)s);
        return 1;
      }
    } else {
      if (found == 1) {
        printf("%08X is matched\n", (uint32_t)s);
        return 1;
      }
    }
    printf("%08X\r", (uint32_t)s);
  }
}
