#include "ui.h"
#include "gplay.h"

int main(int argc, char **argv) {
  GPlay gplay;
  UI ui(argc, argv, gplay, true);
  int rc = ui.Run();
  return rc;
}
