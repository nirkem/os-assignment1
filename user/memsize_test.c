// Show the process's memory size before and after a 20 KB malloc,
// and after free. free returns memory to malloc's free list, not
// to the kernel, so the size doesn't drop again.

#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  printf("Running process is using: %d bytes of memory\n", memsize());

  char *mem = malloc(20 * 1024);
  if(mem == 0){
    printf("memsize_test: malloc failed\n");
    exit(1, "malloc failed");
  }
  printf("Running process is using: %d bytes after the allocation\n", memsize());

  free(mem);
  printf("Running process is using: %d bytes after the release\n", memsize());

  exit(0, "");
}
