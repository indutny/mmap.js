# include <windows.h>

int getpagesize (void) {
  SYSTEM_INFO si;
  GetSystemInfo (&si);
  return si.dwPageSize;
}
