#ifndef UI_H
#define UI_H
#include "defs.h"

void ui_init();
void ui_shutdown();
void ui_draw(struct House* house);
void ui_push_log(const char* msg);



#endif