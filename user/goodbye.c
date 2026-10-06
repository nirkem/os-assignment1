// Exit with a message. The shell prints it once the program ends.

#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  exit(0, "Goodbye World xv6");
}
