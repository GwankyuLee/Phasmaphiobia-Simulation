#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "defs.h"
#include "helpers.h"
#include "functions.h"

//room functions
void room_init(struct Room* room, const char* name, bool is_exit, struct House* house) {
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
    //for BFS
    room->id = house->room_count;
    house->room_count++;
}

void room_connect(struct Room* a, struct Room* b) {
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
    house->ghost = ghost;
    house->casefile = casefile;
    house->numHunters = 0;
}

//ghost functinos

void ghost_init(struct Ghost* ghost, struct House* house) {
    ghost->id = DEFAULT_GHOST_ID;
    //ghost is randomly assigned one of the types of ghosts
    const enum GhostType* list;
    int numGhosts = get_all_ghost_types(&list);
    int randN = rand() % (numGhosts);
    ghost->type = list[randN];
    //ghost begins in a random room of the house
    randN = rand() % (house->room_count);
    ghost->room = &(house->rooms[randN]);
    //update room
    house->rooms[randN].ghost = ghost; 
    
    ghost->boredom = 0;
    ghost->exit = false;
    //log ghost init
    log_ghost_init(ghost->id, ghost->room->name, ghost->type);
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

void hunter_init(struct Hunter** hunter, const char* name, int id, struct House* house) {
    //allocate memory for new hunter
    *hunter = malloc(sizeof(struct Hunter));
    if (*hunter == NULL) {
        fprintf(stderr, "Hunter Memorry allocation failed, Exiting the program. \n");
        exit(EXIT_FAILURE);
    }

    strcpy((*hunter)->name, name);
    (*hunter)->id = id;
    (*hunter)->room = house->starting_room;
    // update room
    house->starting_room->hunters[house->starting_room->numHunters] = *hunter;
    house->starting_room->numHunters++;

    (*hunter)->casefile = (house->casefile);
    //random device
    const enum EvidenceType* list;
    int numType = get_all_evidence_types(&list);
    int randN = rand() % (numType);
    (*hunter)->device = list[randN];

    (*hunter)->path.head = NULL;
    (*hunter)->fear = 0;
    (*hunter)->boredom = 0;
    (*hunter)->reason = -1;
    (*hunter)->exit = false;
    (*hunter)->returnVan = false;
    log_hunter_init(id, (*hunter)->room->name, name, (*hunter)->device);
}

void lock_two_rooms(struct Room* r1, struct Room* r2) {
    // We use the ID as the "Level"
    if (r1->id < r2->id) {
        // r1 is "lower level", so lock it first
        sem_wait(&r1->mutex);
        sem_wait(&r2->mutex);
    } else {
        // r2 is "lower level", so lock it first
        sem_wait(&r2->mutex);
        sem_wait(&r1->mutex);
    }
}

void unlock_two_rooms(struct Room* r1, struct Room* r2) {
    sem_post(&r1->mutex);
    sem_post(&r2->mutex);
}


void ghost_action(struct Ghost* ghost) {
    sem_wait(&ghost->room->mutex);
    if (ghost->room->numHunters>0) {
        ghost->boredom = 0;

        int rand = rand_int_threadsafe(0,2);
        if (rand == 0){
            //hauting
            uint8_t bits[3];
            int count = 0;
            for (int i = 0; i<7; i++) {
                uint8_t bit = 1<<i;
                if (ghost->type & bit) {
                    bits[count++] = bit;
                    if (count == 3) break;
                }
            }
            
            int randType = rand_int_threadsafe(0,3);
            enum EvidenceType current = bits[randType];
            ghost->room->evidence = ghost->room->evidence | current;
            log_ghost_evidence(ghost->id, ghost->boredom, ghost->room->name, current);
            sem_post(&ghost->room->mutex);
        } else {
            //moving
            struct Room *origin = ghost->room;
            int randRoom = rand_int_threadsafe(0, ghost->room->connections);
            struct Room* next = origin->rooms[randRoom];
            sem_post(&ghost->room->mutex);
            lock_two_rooms(origin, next);
            origin->ghost = NULL;
            ghost->room = next;
            next->ghost = ghost;
            log_ghost_move(ghost->id, ghost->boredom, origin->name, ghost->room->name);
            unlock_two_rooms(origin, next);
            
        }

    } else {
        ghost->boredom++;
        int rand = rand_int_threadsafe(0,3);
        if (rand == 0){
            //hauting
            uint8_t bits[3];
            int count = 0;
            for (int i = 0; i<7; i++) {
                uint8_t bit = 1<<i;
                if (ghost->type & bit) {
                    bits[count++] = bit;
                    if (count == 3) break;
                }
            }
            int randType = rand_int_threadsafe(0,3);
            enum EvidenceType current = bits[randType];
            ghost->room->evidence = ghost->room->evidence | current;
            log_ghost_evidence(ghost->id, ghost->boredom, ghost->room->name, current);
            sem_post(&ghost->room->mutex);
        } else if(rand == 1) {
            //moving
            
            struct Room *origin = ghost->room;
            int randRoom = rand_int_threadsafe(0, ghost->room->connections);
            struct Room* next = origin->rooms[randRoom];
            sem_post(&ghost->room->mutex);
            lock_two_rooms(origin, next);

            origin->ghost = NULL;
            ghost->room = next;
            next->ghost = ghost;
            log_ghost_move(ghost->id, ghost->boredom, origin->name, ghost->room->name);
            unlock_two_rooms(origin, next);

        } else {
            //idle
            log_ghost_idle(ghost->id, ghost->boredom, ghost->room->name);
            sem_post(&ghost->room->mutex);
        }

    }

    if (ghost->boredom>ENTITY_BOREDOM_MAX) {
        ghost->exit = true;
        log_ghost_exit(ghost->id, ghost->boredom, ghost->room->name);
        return;
        //release the thread
    }
}





void enqueue(struct RoomQueue* q, struct Room* room) {
    struct RoomNode* temp = malloc(sizeof(struct RoomNode));
    if (temp == NULL) return;
    temp->room = room;
    temp->next = NULL;
    if (q->rear == NULL) {
        q->front = q->rear = temp;
        return;
    }
    q->rear->next = temp;
    q->rear = temp;
}


struct Room* dequeue(struct RoomQueue* q) {
    if (q->front == NULL) return NULL;
    struct RoomNode* temp = q->front;
    struct Room* room = temp->room;
    q->front = q->front->next;
    if (q->front == NULL) q->rear = NULL;
    free(temp);
    return room;
}

bool is_queue_empty(struct RoomQueue* q) {
    return q->front == NULL;
}


//BFS,
//takes in start and des, push it into the hunter path stack
void returnToVan(struct Hunter* hunter, struct Room* start, struct Room* des) {
    // 1. Initialize BFS structures
    bool visited[MAX_ROOMS]; 
    struct Room* parent[MAX_ROOMS];

    for(int i = 0; i < MAX_ROOMS; i++) {
        visited[i] = false;
        parent[i] = NULL;
    }

    struct RoomQueue q = {NULL, NULL};

    // Setup start node
    enqueue(&q, start);
    visited[start->id] = true;
    parent[start->id] = NULL;

    bool found = false;

    // 2. Perform BFS Loop
    while (!is_queue_empty(&q)) {
        struct Room* current = dequeue(&q);

        if (current == des) {
            found = true;
            break;
        }

        for (int i = 0; i < current->connections; i++) {
            struct Room* neighbor = current->rooms[i];
            
            // Check if we've already visited this neighbor using its ID
            if (!visited[neighbor->id]) {
                visited[neighbor->id] = true;
                parent[neighbor->id] = current; // Record where we came from
                enqueue(&q, neighbor);
            }
        }
    }

    // 3. Reconstruct Path (Backtracking)
    // We backtrack from Destination -> Start.
    // Pushing onto a Stack reverses the order, so the top of the stack 
    // will be the immediate next room the hunter should enter.
    if (found) {
        struct Room* curr = des;
        // Trace back from Destination -> Start using the parent array
        while (curr != NULL && curr != start) {
            stack_push(&hunter->path, curr);
            curr = parent[curr->id];
        }
    }

    // 4. Cleanup
    // Clear any remaining items in queue if we exited early
    while (!is_queue_empty(&q)) {
        dequeue(&q);
    }

}


void stack_push(struct RoomStack* stack, struct Room* room) {
    struct RoomNode* node = malloc(sizeof(struct RoomNode));
    if (node == NULL) return; // Safety check
    node->room = room;
    node->next = stack->head;
    stack->head = node;
}

struct Room* stack_pop(struct RoomStack* stack) {
    if (stack->head == NULL) {
        return NULL;  // Stack underflow (empty stack)
    }

    struct RoomNode* temp = stack->head;
    struct Room* poppedRoom = temp->room;

    stack->head = temp->next;  // Move head down
    free(temp);                // Free removed node

    return poppedRoom;
}




void hunter_action(struct Hunter* hunter, struct House* house) {
    //if ghost is in the room set boredom to 0 and increase fear by 1
    //otherwise increase boredom by 1
    sem_wait(&hunter->room->mutex);
    if (hunter->room->ghost == NULL) {
        hunter->boredom++;
    } else {
        hunter->boredom = 0;
        hunter->fear++;
    }
    sem_post(&hunter->room->mutex);


    if (hunter->room == house->starting_room){
        if (hunter->returnVan){
            log_return_to_van(hunter->id, hunter->boredom, hunter->fear, hunter->room->name, hunter->device, false);
        }


        hunter->returnVan = false;
        sem_wait(&hunter->casefile->mutex);
        if (evidence_is_valid_ghost(hunter->casefile->collected)) {
            
            hunter->casefile->solved = true;
            sem_post(&hunter->casefile->mutex);
            hunter->reason = 0;
            hunter->exit = true;
            //free hunter and remove from the room

            log_exit(hunter->id, hunter->boredom, hunter->fear, hunter->room->name, hunter->device, hunter->reason);
            return;
             
        } else {
            sem_post(&hunter->casefile->mutex);
            // swap device by random excluding the one already holding
            const enum EvidenceType* evidence_list;
            int num_evidence = get_all_evidence_types(&evidence_list);
            enum EvidenceType current_device = hunter->device;
            
            // Keep picking until we get a different one
            do {
                int choice = rand_int_threadsafe(0, num_evidence);
                hunter->device = evidence_list[choice];
            } while (hunter->device == current_device);

            log_swap(hunter->id, hunter->boredom, hunter->fear, current_device, hunter->device);
            
        }
    }
    sem_wait(&hunter->casefile->mutex);
    if (!hunter->casefile->solved){
        //exit the simulation, game over.
        if (hunter->boredom > ENTITY_BOREDOM_MAX){
            hunter->reason = 1;
            hunter->exit = true;
            log_exit(hunter->id, hunter->boredom, hunter->fear, hunter->room->name, hunter->device, hunter->reason);
            sem_post(&hunter->casefile->mutex);
            return;
        }
    }
    sem_post(&hunter->casefile->mutex);

    if (hunter->fear > HUNTER_FEAR_MAX){
        hunter->reason = 2;
        hunter->exit = true;
        log_exit(hunter->id, hunter->boredom, hunter->fear, hunter->room->name, hunter->device, hunter->reason);
        return;
    }

    //gather evidence
    //we return to van only when we found evidence or to change device
    //when we find we ignore everything and move to van.
    if (!hunter->returnVan){
        sem_wait(&hunter->room->mutex);
        if ((hunter->device & hunter->room->evidence) == hunter->device){
            log_evidence(hunter->id, hunter->boredom, hunter->fear, hunter->room->name, hunter->device);
            
            hunter->room->evidence &= ~hunter->device;
            sem_post(&hunter->room->mutex);

            sem_wait(&hunter->casefile->mutex);
            hunter->casefile->collected = hunter->casefile->collected | hunter->device;
            if (evidence_is_valid_ghost(hunter->casefile->collected)){
                hunter->casefile->solved = true;
            }
            sem_post(&hunter->casefile->mutex);

            if (hunter->room != house->starting_room){
                returnToVan(hunter, hunter->room, house->starting_room);
                hunter->returnVan = true;
                log_return_to_van(hunter->id, hunter->boredom, hunter->fear, hunter->room->name, hunter->device, true);
            }
        } else {
            sem_post(&hunter->room->mutex);
            //5 % chance to set returnVan to true
            //choose 1-100 and if modulo 20 is equal to 0, set returnVan to true
            if (rand_int_threadsafe(1, 101)%20 == 0) {
                returnToVan(hunter, hunter->room, house->starting_room);
                hunter->returnVan = true;
                log_return_to_van(hunter->id, hunter->boredom, hunter->fear, hunter->room->name, hunter->device, true);
            }
            
        }
        
    }

    //movement
    struct Room* next;
    struct Room* origin = hunter->room;
    if (hunter->returnVan) {
        next = stack_pop(&hunter->path);
        //should never happen but error check
        if (next == NULL) return;
        //from room, remove hunter array and decrement hunters number
    } else {
        //no need for lock since rooms[] is never changed
        int rand = rand_int_threadsafe(0, hunter->room->connections);
        next = origin->rooms[rand];
    }
    lock_two_rooms(origin, next);
    if (next->numHunters >= MAX_ROOM_OCCUPANCY) {
        if (hunter->returnVan) {
            stack_push(&hunter->path, next);
        }
        unlock_two_rooms(origin, next);
        //then do not move
    }else if (origin->numHunters <1) {
        unlock_two_rooms(origin, next);
        //should never happen
    } else if (origin->numHunters < 2) {
        //no need to actually remove the space
        origin->numHunters--;
        hunter->room = next;
        next->hunters[next->numHunters] = hunter;
        next->numHunters++;
        unlock_two_rooms(origin, next);
    } else {
        for (int i =0; i<origin->numHunters; i++){
            if (origin->hunters[i]->id == hunter->id) {
                //how to remove this from the array without error
                for (int j = i; j < origin->numHunters - 1; j++) {
                    origin->hunters[j] = origin->hunters[j + 1];
                }
                break;
            }
        }
        origin->numHunters--;
        hunter->room = next;
        next->hunters[next->numHunters] = hunter;
        next->numHunters++;
        unlock_two_rooms(origin, next);
    }

}


void hunter_exit(struct Hunter* hunter){
    sem_wait(&hunter->room->mutex);
    struct Room* current = hunter->room;
    for (int i = 0; i < current->numHunters; i++) {
        if (current->hunters[i] == hunter) {

            // Shift left
            for (int j = i; j < current->numHunters - 1; j++) {
                current->hunters[j] = current->hunters[j + 1];
            }

            current->hunters[current->numHunters - 1] = NULL;
            current->numHunters--;
            break;
        }
    }
    sem_post(&hunter->room->mutex);

}

void ghost_exit(struct Ghost* ghost){
    
    sem_wait(&ghost->room->mutex);
    struct Room* room = ghost->room;
    if (room != NULL) {
        room->ghost = NULL;
    }
    sem_post(&ghost->room->mutex);
    ghost->room = NULL;
}




void* hunter_thread(void* arg){
    struct ThreadArgs* args = (struct ThreadArgs*)arg;;
    struct Hunter* hunter = args->hunter;
    struct House* house = args->house;
    while (!hunter->exit) {
        hunter_action(hunter, house);
        usleep(100);
    }
    hunter_exit(hunter);
    printf("hunter exit reason: %s",exit_reason_to_string(hunter->reason));
    free(args);
    while (hunter->path.head != NULL) {
        stack_pop(&hunter->path); // This function frees the node
    }
    

    return NULL;
}

void* ghost_thread(void* arg){
    struct Ghost* ghost = (struct Ghost*)arg;

    while (!ghost->exit) {
        ghost_action(ghost);
        usleep(100);
    }
    ghost_exit(ghost);

    return NULL;
}