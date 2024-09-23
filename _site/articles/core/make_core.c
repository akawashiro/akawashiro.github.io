#include <stdint.h>
#include <stdio.h>

uint64_t a = 0xaaaabbbbccccdddd;

int main() {
  // Print all lines in /proc/self/maps
  FILE *fp = fopen("/proc/self/maps", "r");
  char buf[0x1000];
  while (fgets(buf, sizeof(buf), fp)) {
    printf("%s", buf);
  }
  fclose(fp);

  for (size_t i = 0; i < 0x100000; i++) {
    *(&a + i) = 0xdeadbeefdeadbeef;
  }
}
