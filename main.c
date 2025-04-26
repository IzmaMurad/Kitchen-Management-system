#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "chef.h"

int main() {
    pthread_t reporter_thread;
    pthread_t chefs[NUM_CHEFS];
    pthread_t kitchen_manager;
    Chef chef_data[NUM_CHEFS];

    srand(time(NULL));
    initialize_resources(&kitchen);

    if (pthread_create(&reporter_thread, NULL, status_reporter, NULL) != 0) {
        perror("Failed to create reporter thread");
        return 1;
    }

    if (pthread_create(&kitchen_manager, NULL, kitchen_manager_work, NULL) != 0) {
        perror("Failed to create kitchen manager thread");
        return 1;
    }

    for (int i = 0; i < NUM_CHEFS; i++) {
        chef_data[i].id = i;
        initialize_chef_needs(&chef_data[i]);

        if (pthread_create(&chefs[i], NULL, chef_work, &chef_data[i]) != 0) {
            perror("Failed to create chef thread");
            return 1;
        }
    }

    
    for (int i = 0; i < NUM_CHEFS; i++) {
        pthread_join(chefs[i], NULL);
    }
    pthread_join(kitchen_manager, NULL);
    pthread_join(reporter_thread, NULL); 

    cleanup_resources(&kitchen);

    return 0;
}

