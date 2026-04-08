#include "ui.h"
#include "gplay.h"

int main(int argc, char **argv) {
  GPlay gplay;
  UI ui(argc, argv, gplay, false);
  int rc = ui.Run();
  return rc;
}
