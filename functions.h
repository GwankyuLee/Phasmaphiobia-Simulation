#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "defs.h"
#include "helpers.h"

void room_init(struct Room* room, const char* name, bool is_exit);
void rooms_connect(struct Room* a, struct Room* b); // Bidirectional connection
void house_init(struct House* house, struct Ghost* ghost, struct CaseFile* casefile);
void ghost_init(struct Ghost* ghost, struct House* house);
void casefile_init(struct CaseFile* casefile);
void hunter_init(struct Hunter* hunter);
#endif // FUNCTIONS_H