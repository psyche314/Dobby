#include <dobby.h>
#include <string.h>
int main(void) {
  if (strcmp(DobbyGetVersion(), "1.0.0") != 0) return 1;
  if (DobbyPrepare(0, 0, 0) == 0) return 2;
  if (DobbyCommit(0) == 0 || DobbyDisable(0) == 0 || DobbyEnable(0) == 0) return 3;
  return 0;
}
