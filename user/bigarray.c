#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

#define ARRAY_SIZE (1 << 16) 
#define NUM_CHILDREN 6       

int main(int argc, char* argv[]) {

  printf("started calculation\n");

  int pids[NUM_CHILDREN];
  int statuses[NPROC]; int n = 0;

  int sum = 0;
  int start, end;

  int* array = malloc(ARRAY_SIZE * sizeof(int));
  if (array == 0) {
    printf("Error: malloc failed\n");
    exit(1, "malloc failed");
  }

  for (int i = 0; i < ARRAY_SIZE; i++) {
    array[i] = i;
  }

  int forkn_ret = forkn(NUM_CHILDREN, pids);
  if (forkn_ret < 0) {
    printf("Error: forkn failed\n");
    free(array);
    exit(1, "forkn failed");
  }

  for (int i = 0; i < NUM_CHILDREN; i++) {
    if (forkn_ret == i + 1) {

      // Calculate current child sum
      start = i * (ARRAY_SIZE / NUM_CHILDREN);
      end = (i == NUM_CHILDREN - 1) ? ARRAY_SIZE : start + (ARRAY_SIZE / NUM_CHILDREN);

      int partial_sum = 0;
      for (int j = start; j < end; j++) {
        partial_sum += array[j];
      }

      // Exit and return the sum to parent
      printf("%d\n", partial_sum);
      exit(partial_sum, "");
    }
  }




  // Wait for all child processes to finish
  if (waitall(&n, statuses) < 0) {
    printf("Error: waitall failed\n");
    exit(1, "waitall failed");
  }

  // Verify that the number of children matches
  if (n != NUM_CHILDREN) {
    printf("Error: waitall returned %d children, expected %d\n", n, NUM_CHILDREN);
    exit(1, "waitall mismatch");
  }

  // Calculate the total sum from the exit statuses
  for (int i = 0; i < n; i++) {
    sum += statuses[i];
  }

  // Print the total sum
  printf("Total sum: %d\n", sum);

  // Exit
  free(array);
  exit(0, "calculation complete");
}
