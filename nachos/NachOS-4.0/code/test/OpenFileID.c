#include "syscall.h"

int
main()
{
  int result;
  int type;

  cout << "Nhap vao type: ";
  cin >> type;
  result = OpenFileID->Open('Hello.txt', int type);
  
  Halt();
}