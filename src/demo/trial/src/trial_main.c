#include "egg/egg.h"

void egg_client_quit() {
  egg_log("%s",__func__);
}

int egg_client_init() {
  egg_log("%s helooooooo?",__func__);
  return 0;
}

void egg_client_update(double elapsed) {
  egg_log("%s elapsed=%f",__func__,elapsed);
}

void egg_client_render() {
  egg_log("%s",__func__);
}
