#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "defs.h"
#include "helpers.h"
#include "functions.h"

//room functions
void room_init(struct Room* room, const char* name, bool is_exit) {
    strcpy(room->name, name);
    room->name[MAX_ROOM_NAME - 1] = '\0';
    room->exit = is_exit;
    room->ghost = NULL;
    room->connections = 0;
    room->numHunters = 0;
    room->evidence = 0;
    for (int i = 0; i<MAX_CONNECTIONS; i++) {
        room->rooms[i] = NULL;
    }
    for (int i =0; i<MAX_ROOM_OCCUPANCY; i++) {
        room->hunters[i] = NULL;
    }
    if (sem_init(&room->mutex, 0, 1) != 0) {
        perror("Semaphore initilaization failed for room\n");
    }
}

void rooms_connect(struct Room* a, struct Room* b) {
    if (a->connections < MAX_CONNECTIONS && b->connections < MAX_CONNECTIONS) {

        a->rooms[a->connections] = b;
        a->connections++;

        b->rooms[b->connections] = a;
        b->connections++;
    } else {
        printf("Error: Could not connect %s and %s (Max connections reached)\n", a->name, b->name);
    }
}

//house functions

void house_init(struct House* house, struct Ghost* ghost, struct CaseFile* casefile) {
    house_populate_rooms(house);
    ghost_init(ghost, house);
    casefile_init(casefile);
    house->hunters = NULL;
}

//ghost functinos

void ghost_init(struct Ghost* ghost, struct House* house) {
    srand(time(NULL));
    ghost->id = DEFAULT_GHOST_ID;
    //ghost is randomly assigned one of the types of ghosts
    const enum GhostType* list;
    int numGhosts = get_all_ghost_types(&list);
    int randN = rand() % (numGhosts);
    ghost->type = list[randN];
    //ghost begins in a random room of the house
    randN = rand() % (house->room_count);
    ghost->room = &(house->rooms[randN]);
    
    ghost->boredom = 0;
    ghost->exit = false;
}

//casefile functions

void casefile_init(struct CaseFile* casefile) {
    casefile->collected = 0;
    casefile->solved = false;
    if (sem_init(&casefile->mutex, 0, 1) != 0) {
        perror("Semaphore initilaization failed for casefile\n");
    }
}

//hunter functions

void hunter_init(struct Hunter* hunter) {

}
