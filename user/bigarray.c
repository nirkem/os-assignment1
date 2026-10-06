// Sum 0 to 65,535 across NCHILD children with forkn and waitall.
// Each child sums its own slice and returns the partial sum as its
// exit status. The total, 2,147,450,880, just fits in an int.

#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

#define SIZE   (1 << 16)
#define NCHILD 6

int
main(int argc, char *argv[])
{
  int pids[NCHILD];
  int statuses[NPROC];
  int n, sum, i;

  int *array = malloc(SIZE * sizeof(int));
  if(array == 0){
    printf("bigarray: malloc failed\n");
    exit(1, "malloc failed");
  }
  for(i = 0; i < SIZE; i++)
    array[i] = i;

  int r = forkn(NCHILD, pids);
  if(r < 0){
    printf("bigarray: forkn failed\n");
    exit(1, "forkn failed");
  }

  if(r > 0){
    // Child r sums slice r-1. The last slice takes the remainder.
    int start = (r - 1) * (SIZE / NCHILD);
    int end = r == NCHILD ? SIZE : start + SIZE / NCHILD;
    sum = 0;
    for(i = start; i < end; i++)
      sum += array[i];
    exit(sum, "");
  }

  if(waitall(&n, statuses) < 0){
    printf("bigarray: waitall failed\n");
    exit(1, "waitall failed");
  }
  if(n != NCHILD){
    printf("bigarray: waitall returned %d children, expected %d\n", n, NCHILD);
    exit(1, "wrong child count");
  }

  // The parent prints the partial sums, in the order the children
  // finished, so lines from different children don't interleave.
  sum = 0;
  for(i = 0; i < n; i++){
    printf("partial sum: %d\n", statuses[i]);
    sum += statuses[i];
  }
  printf("Total sum: %d\n", sum);

  free(array);
  exit(0, "calculation complete");
}
