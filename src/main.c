// main.c
#include "common.h"
#include <stdio.h>

int main(void) {
  Random thing = {.value = 1}; // designated initializer in C
  printf("Hello, value = %zu!\n", thing.value);
  return 0;
}
