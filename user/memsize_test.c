#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[]) {
    int memory_size = memsize();
    printf("Running process is using: %d bytes of memory\n", memory_size);

    int alloc_size = 20 * 1024;
    char* allocated_memory = malloc(alloc_size);
    if (allocated_memory == 0) {
        printf("Memory allocation failed\n");
        exit(1, "");
    }

    int new_memory_size = memsize();
    printf("Running process is using: %d bytes after the allocation\n", new_memory_size);

    free(allocated_memory);

    int after_free_mem_size = memsize();
    printf("Running process is using: %d bytes after the release\n", after_free_mem_size);

    exit(0, "");
}
