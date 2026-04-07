#include "ui.h"

int main(int argc, char **argv) {
  UI ui(argc, argv, true);
  int rc = ui.Run();
  return rc;
}
