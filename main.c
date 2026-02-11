#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/wait.h>
#include "defs.h"
#include "helpers.h"
#include "functions.h"

int main() {
    struct House house;
    struct Ghost ghost;
    struct CaseFile casefile;
    house_init(&house, &ghost, &casefile);

    char huntName[MAX_HUNTER_NAME];
    int huntID;
    int numHunters = 0;
    house.hunters = malloc(MAX_ROOM_OCCUPANCY * sizeof(struct Hunter*));
    if (house.hunters == NULL) {
        fprintf(stderr, "House hunter array Memorry allocation failed, Exiting the program. \n");
        exit(EXIT_FAILURE);
    }

    while(numHunters<MAX_ROOM_OCCUPANCY) {
        printf("Please enter your hunter's name (type 'done' to stop) (63character limit): ");
        scanf("%63s", huntName);
        while (getchar() != '\n');
        if (strcmp(huntName, "done") == 0) {
            break;
        }

        printf("\nPlease enter your hunter's ID: ");
        scanf("%d", &huntID);
        while (getchar() != '\n');
        hunter_init(&(house.hunters[numHunters]), huntName, huntID, &house);
        numHunters++;
    }
    //realloc unused memory
    house.hunters = realloc(house.hunters, numHunters*sizeof(struct Hunter*));
    if (house.hunters == NULL && numHunters >0) {
        fprintf(stderr, "realloc failed for hunters array");
        exit(EXIT_FAILURE);
    }

    printf("\n-finished initilization-\n");
    /*
    1. Initialize a House structure.
    2. Populate the House with rooms using the provided helper function.
    3. Initialize all of the ghost data and hunters.
done
    4. Create threads for the ghost and each hunter.
    5. Wait for all threads to complete.
    6. Print final results to the console:
         - Type of ghost encountered.
         - The reason that each hunter exited
         - The evidence collected by each hunter and which ghost is represented by that evidence.
    7. Clean up all dynamically allocated resources and call sem_destroy() on all semaphores.
    */
    /*
    log hunter init
    log move
    log evidence
    log swap
    log exit
    log return to van
    similar for ghost
    */
    return 0;
}
