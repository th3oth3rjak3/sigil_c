#include "common.h"
#include "chunk.h"
#include <stdio.h>

int main(void)
{
  Random thing = {.value = 1};
  printf("Hello, value = %zu!\n", thing.value);
  return 0;
}
