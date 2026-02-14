#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "defs.h"
#include "helpers.h"

void room_init(struct Room* room, const char* name, bool is_exit, struct House* house);
void room_connect(struct Room* a, struct Room* b); // Bidirectional connection
void house_init(struct House* house, struct Ghost* ghost, struct CaseFile* casefile);
void ghost_init(struct Ghost* ghost, struct House* house);
void casefile_init(struct CaseFile* casefile);
void hunter_init(struct Hunter** hunter, const char* name, int id, struct House* house);
void ghost_action(struct Ghost* ghost);
void hunter_action(struct Hunter* hunter, struct House* house);

void returnToVan(struct Hunter* hunter, struct Room* start, struct Room* des);

void stack_push(struct RoomStack* stack, struct Room* room);
struct Room* stack_pop(struct RoomStack* stack);

void enqueue(struct RoomQueue* q, struct Room* room);
struct Room* dequeue(struct RoomQueue* q);
bool is_queue_empty(struct RoomQueue* q);

void hunter_exit(struct Hunter* hunter);
void ghost_exit(struct Ghost* ghost);

void lock_two_rooms(struct Room* r1, struct Room* r2);
void unlock_two_rooms(struct Room* r1, struct Room* r2);

void* hunter_thread(void* arg);
void* ghost_thread(void* arg);

#endif //FUNCTIONS_H