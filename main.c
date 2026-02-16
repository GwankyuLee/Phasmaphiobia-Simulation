#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <time.h>
#include "defs.h"
#include "helpers.h"
#include "functions.h"

#define NUM_SIMULATIONS 5

struct Temp{
    char huntName[MAX_HUNTER_NAME];
    int huntID;
};

int main() {
    srand(time(NULL));
    //user input
    //store input in temporary array
    struct Temp temp[MAX_ROOM_OCCUPANCY];
    int numInputHunters = 0;

    printf("--- GHOST HUNT SIMULATION ---\n");

    while (numInputHunters < MAX_ROOM_OCCUPANCY) {
        printf("Please enter your hunter's name (type 'done' to stop) (63 character limit): ");
        scanf("%63s", temp[numInputHunters].huntName);
        while (getchar() != '\n');
        if (strcmp(temp[numInputHunters].huntName, "done") == 0) break;

        printf("Please enter your hunter's ID: ");
        scanf("%d", &temp[numInputHunters].huntID);
        while (getchar() != '\n');
        numInputHunters++;
    }

    //pipes and forks for simulations
    int pipes[NUM_SIMULATIONS][2];
    pid_t pids[NUM_SIMULATIONS];

    for (int i = 0; i < NUM_SIMULATIONS; i++) {
        if (pipe(pipes[i]) == -1) { perror("pipe"); exit(1); }

        pids[i] = fork();
        if (pids[i] < 0) { perror("fork"); exit(1); }

        if (pids[i] == 0) {
            // child process
            close(pipes[i][0]);  // close read end
            srand(time(NULL) ^ getpid());

            struct House house;
            struct Ghost ghost;
            struct CaseFile casefile;
            house_init(&house, &ghost, &casefile);

            // Use user input hunters in child
            house.hunters = malloc(sizeof(struct Hunter*) * numInputHunters);
            house.numHunters = numInputHunters;

            for (int i = 0; i < numInputHunters; i++) {
                // Each child gets a copy of user hunter
                hunter_init(&house.hunters[i], temp[i].huntName, temp[i].huntID, &house);
            }
            printf("\n- pid:%d Finished initialization -\n", pids[i]);

            printf("\n-creating threads-\n");
            // Init threads
            pthread_t ghostThread;
            pthread_t* hunterThreads = malloc(sizeof(pthread_t) * numInputHunters);

            struct timespec start, end;
            clock_gettime(CLOCK_MONOTONIC, &start);

            pthread_create(&ghostThread, NULL, ghost_thread, &ghost);

            for (int h = 0; h < numInputHunters; h++) {
                struct ThreadArgs* args = malloc(sizeof(struct ThreadArgs));
                args->house = &house;
                args->hunter = house.hunters[h];
                pthread_create(&hunterThreads[h], NULL, hunter_thread, args);
            }

            pthread_join(ghostThread, NULL);
            for (int h = 0; h < numInputHunters; h++){
                pthread_join(hunterThreads[h], NULL);
            }
            
            // Freeing semaphores
            for (int i = 0; i < house.room_count; i++) {
                sem_destroy(&house.rooms[i].mutex);
            }
            // Destroy casefile semaphore
            sem_destroy(&casefile.mutex);

            clock_gettime(CLOCK_MONOTONIC, &end);

            double duration = (end.tv_sec - start.tv_sec) * 1000.0 +
                              (end.tv_nsec - start.tv_nsec) / 1000000.0;

            struct Result result;
            // Go through loop of hunters and if at least 1 hunter's exit reason was 0 the hunter win.
            result.ghost_won = true;
            for (int i = 0; i<house.numHunters; i++){
                if (house.hunters[i]->reason == 0){
                    result.ghost_won = false;
                    break;
                }
            }
            result.actual_ghost = ghost.type;
            result.final_evidence = casefile.collected;
            result.duration_ms = duration;

            write(pipes[i][1], &result, sizeof(result));
            close(pipes[i][1]);
            

            // Free hunters
            for (int i = 0; i < house.numHunters; i++) {
                free(house.hunters[i]);
            }

            free(house.hunters);
            free(hunterThreads);
            exit(0);  // child done
        }

        // Parent process
        close(pipes[i][1]); // close write end
    }

    // parent process collects resutls
    struct Result results[NUM_SIMULATIONS];
    double totalTime = 0.0;
    int ghostWins = 0, hunterWins = 0;

    for (int i = 0; i < NUM_SIMULATIONS; i++) {
        read(pipes[i][0], &results[i], sizeof(results[i]));
        close(pipes[i][0]);
        waitpid(pids[i], NULL, 0);

        totalTime += results[i].duration_ms;
        if (results[i].ghost_won) ghostWins++;
        else hunterWins++;
    }

    // Print outcomes of each simulation
    for (int i = 0; i < NUM_SIMULATIONS; i++) {
        printf("Simulation %d:\n", i+1);
        printf("  Ghost Type: %s\n", ghost_to_string(results[i].actual_ghost));
        printf("  Evidence: %s\n", ghost_to_string(results[i].final_evidence));
        printf("  Winner: %s\n", results[i].ghost_won ? "Ghost" : "Hunters");
        printf("  Duration: %.5f ms\n\n", results[i].duration_ms);
    }

    printf("====== SUMMARY ======\n");
    printf("Total Ghost Wins: %d\n", ghostWins);
    printf("Total Hunter Wins: %d\n", hunterWins);
    printf("Average Duration: %.5f ms\n", totalTime / NUM_SIMULATIONS);

    
    return 0;
}
