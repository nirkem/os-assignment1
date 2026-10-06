// Tests for memsize, exit messages, forkn and waitall.
// Each test runs in its own child. A failing test exits with
// status 1 and says why in its exit message.

#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

#define BADADDR ((void *)0xffffffffffL)  // not mapped in any process

#define CHECK(cond, msg) do { if(!(cond)) exit(1, msg); } while(0)

static int
streq(char *a, char *b)
{
  return strcmp(a, b) == 0;
}

void
memsize_grows(void)
{
  int before = memsize();
  CHECK(sbrk(3 * 4096) != (char *)-1, "sbrk failed");
  CHECK(memsize() == before + 3 * 4096, "memsize did not grow by 12 KB");
  sbrk(-3 * 4096);
  CHECK(memsize() == before, "memsize did not shrink back");
  exit(0, "");
}

void
exit_message(void)
{
  int pid, status;
  char msg[MAXEXITMSG];

  if((pid = fork()) == 0)
    exit(7, "hello parent");
  CHECK(wait(&status, msg) == pid, "wait returned the wrong pid");
  CHECK(status == 7, "wrong exit status");
  CHECK(streq(msg, "hello parent"), "wrong exit message");
  exit(0, "");
}

void
exit_message_truncated(void)
{
  char msg[MAXEXITMSG];

  if(fork() == 0)
    exit(0, "0123456789012345678901234567890123456789");
  CHECK(wait(0, msg) > 0, "wait failed");
  CHECK(strlen(msg) == MAXEXITMSG - 1, "long message not cut to fit");
  CHECK(streq(msg, "0123456789012345678901234567890"), "wrong truncated message");
  exit(0, "");
}

void
exit_message_null(void)
{
  char msg[MAXEXITMSG];

  if(fork() == 0)
    exit(0, 0);
  CHECK(wait(0, msg) > 0, "wait failed");
  CHECK(msg[0] == 0, "null message is not empty");

  if(fork() == 0)
    exit(0, BADADDR);
  CHECK(wait(0, msg) > 0, "wait failed");
  CHECK(msg[0] == 0, "bad message pointer is not empty");
  exit(0, "");
}

void
wait_bad_message_pointer(void)
{
  int pid;

  if((pid = fork()) == 0)
    exit(0, "x");
  CHECK(wait(0, BADADDR) == -1, "wait accepted a bad pointer");
  // The child was not freed, so a second wait still finds it.
  CHECK(wait(0, 0) == pid, "child lost after failed wait");
  exit(0, "");
}

void
forkn_rejects_bad_n(void)
{
  int pids[NFORKN + 1], n;

  CHECK(forkn(0, pids) == -1, "forkn(0) succeeded");
  CHECK(forkn(-1, pids) == -1, "forkn(-1) succeeded");
  CHECK(forkn(NFORKN + 1, pids) == -1, "forkn(17) succeeded");
  CHECK(waitall(&n, 0) == 0 && n == 0, "failed forkn left children");
  exit(0, "");
}

void
forkn_bad_pids_pointer(void)
{
  int n;

  CHECK(forkn(4, BADADDR) == -1, "forkn accepted a bad pointer");
  CHECK(waitall(&n, 0) == 0 && n == 0, "failed forkn left children");
  exit(0, "");
}

void
forkn_children(void)
{
  int pids[NFORKN], statuses[NPROC], seen[NFORKN + 1];
  int n, i, r;

  r = forkn(NFORKN, pids);
  if(r > 0)
    exit(r, "");
  CHECK(r == 0, "forkn failed");

  for(i = 0; i < NFORKN; i++)
    for(int j = 0; j < i; j++)
      CHECK(pids[i] != pids[j], "duplicate pids");

  CHECK(waitall(&n, statuses) == 0, "waitall failed");
  CHECK(n == NFORKN, "waitall returned the wrong count");

  // Every child exited with its own index, so the statuses
  // are 1 to NFORKN in some order.
  memset(seen, 0, sizeof(seen));
  for(i = 0; i < n; i++){
    CHECK(statuses[i] >= 1 && statuses[i] <= NFORKN, "bad child index");
    CHECK(!seen[statuses[i]], "two children got the same index");
    seen[statuses[i]] = 1;
  }
  exit(0, "");
}

void
forkn_repeated(void)
{
  int pids[NFORKN], n;

  // Leaked proc slots would make this fail after a few rounds.
  for(int round = 0; round < 20; round++){
    int r = forkn(NFORKN, pids);
    if(r > 0)
      exit(0, "");
    CHECK(r == 0, "forkn failed on a later round");
    CHECK(waitall(&n, 0) == 0 && n == NFORKN, "waitall wrong count");
  }
  exit(0, "");
}

void
forkn_all_or_nothing(void)
{
  int fds[2], pids[NFORKN], n, r;
  char c;

  // Fill the process table with children that block on a pipe.
  CHECK(pipe(fds) == 0, "pipe failed");
  for(;;){
    int pid = fork();
    if(pid < 0)
      break;
    if(pid == 0){
      close(fds[1]);
      read(fds[0], &c, 1);
      exit(0, "");
    }
  }

  // Let 10 of them go, freeing exactly 10 slots.
  for(int i = 0; i < 10; i++)
    write(fds[1], "x", 1);
  for(int i = 0; i < 10; i++)
    CHECK(wait(0, 0) > 0, "wait failed");

  // 16 can't fit, so none may be created...
  CHECK(forkn(NFORKN, pids) == -1, "forkn(16) succeeded with 10 free");
  // ...and the partial ones must be freed, so 10 still fit.
  r = forkn(10, pids);
  if(r > 0)
    exit(0, "");
  CHECK(r == 0, "failed forkn leaked process slots");

  close(fds[1]);
  CHECK(waitall(&n, 0) == 0, "waitall failed");
  exit(0, "");
}

void
waitall_no_children(void)
{
  int n = -1;

  CHECK(waitall(&n, 0) == 0, "waitall failed");
  CHECK(n == 0, "count not 0");
  exit(0, "");
}

void
waitall_after_fork(void)
{
  int statuses[NPROC], n, sum = 0;

  // waitall also collects children made by plain fork.
  for(int i = 1; i <= 5; i++)
    if(fork() == 0)
      exit(i * 10, "");
  CHECK(waitall(&n, statuses) == 0, "waitall failed");
  CHECK(n == 5, "wrong count");
  for(int i = 0; i < n; i++)
    sum += statuses[i];
  CHECK(sum == 150, "wrong statuses");
  CHECK(wait(0, 0) == -1, "children left after waitall");
  exit(0, "");
}

struct test {
  void (*f)(void);
  char *name;
} tests[] = {
  {memsize_grows, "memsize_grows"},
  {exit_message, "exit_message"},
  {exit_message_truncated, "exit_message_truncated"},
  {exit_message_null, "exit_message_null"},
  {wait_bad_message_pointer, "wait_bad_message_pointer"},
  {forkn_rejects_bad_n, "forkn_rejects_bad_n"},
  {forkn_bad_pids_pointer, "forkn_bad_pids_pointer"},
  {forkn_children, "forkn_children"},
  {forkn_repeated, "forkn_repeated"},
  {forkn_all_or_nothing, "forkn_all_or_nothing"},
  {waitall_no_children, "waitall_no_children"},
  {waitall_after_fork, "waitall_after_fork"},
  {0, 0},
};

int
main(int argc, char *argv[])
{
  int failed = 0;

  // Stop on the name: the first function can sit at address 0.
  for(struct test *t = tests; t->name; t++){
    int status;
    char msg[MAXEXITMSG];

    printf("%s: ", t->name);
    int pid = fork();
    if(pid < 0){
      printf("fork failed\n");
      exit(1, "");
    }
    if(pid == 0)
      t->f();
    wait(&status, msg);
    if(status == 0){
      printf("OK\n");
    } else {
      printf("FAILED: %s\n", msg);
      failed++;
    }
  }

  if(failed){
    printf("%d syscalltests FAILED\n", failed);
    exit(1, "");
  }
  printf("ALL SYSCALLTESTS PASSED\n");
  exit(0, "");
}
