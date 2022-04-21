#include "syscall.h"

int
main()
{
  int result;
  
  result = Create("../test/Hello.txt");

  Halt();
}