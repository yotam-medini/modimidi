#include "ui.h"
#include "gplay.h"

int main(int argc, char **argv) {
  constexpr auto is_android = true;
  GPlay gplay(is_android);
  UI ui(argc, argv, gplay, is_android);
  int rc = ui.Run();
  return rc;
}
