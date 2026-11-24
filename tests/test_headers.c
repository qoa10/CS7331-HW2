#include <stdio.h>
#include <stdlib.h>

int scale(int x) { return x * 8; }

int main(void) {
  int *a = malloc(10 * sizeof(int));
  for (int i = 0; i < 10; i++)
    a[i] = scale(i) / 2;
  printf("%d\n", a[9]);
  free(a);
  return 0;
}
