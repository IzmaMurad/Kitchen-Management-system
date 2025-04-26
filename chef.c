#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include "chef.h"

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_CYAN    "\033[36m"

const char* resource_names[] = {
    "Stove",
    "Knife",
    "Chopping Board",
    "Vegetables",
    "Cooking Pot",
     "Spices"
};

KitchenManager kitchen;

void initialize_chef_needs(Chef *chef) {
   
    chef->skill_level = rand() % 3;
    chef->success_count = 0;
    chef->failure_count = 0;

    switch (chef->id) {
        case 0: chef->needs[0] = STOVE; chef->needs[1] = KNIFE; break;
        case 1: chef->needs[0] = KNIFE; chef->needs[1] = CHOPPING_BOARD; break;
        case 2: chef->needs[0] = STOVE; chef->needs[1] = CHOPPING_BOARD; break;
        case 3: chef->needs[0] = KNIFE; chef->needs[1] = STOVE; break;
        case 4: chef->needs[0] = CHOPPING_BOARD; chef->needs[1] = KNIFE; break;
        case 5: chef->needs[0] = VEGETABLES; chef->needs[1] = KNIFE; break;
        case 6: chef->needs[0] = VEGETABLES; chef->needs[1] = CHOPPING_BOARD; break;
        case 7: chef->needs[0] = VEGETABLES; chef->needs[1] = COOKING_POT; break;
        case 8: chef->needs[0] = COOKING_POT; chef->needs[1] = STOVE; break;
        case 9: chef->needs[0] = CHOPPING_BOARD; chef->needs[1] = STOVE; break;
    }
}

void initialize_resources(KitchenManager *manager) {
    manager->total_meals_prepared = 0;
    manager->total_failures = 0;
    pthread_mutex_init(&manager->kitchen_mutex, NULL);

    for (int i = 0; i < NUM_RESOURCES; i++) {
        manager->resources[i].type = i;
        manager->resources[i].state = RESOURCE_AVAILABLE;
        manager->resources[i].quality = MAX_RESOURCE_QUALITY;
        manager->resources[i].usage_count = 0;
        pthread_mutex_init(&manager->resources[i].mutex, NULL);
        sem_init(&manager->resources[i].semaphore, 0, 1);
    }
}

void cleanup_resources(KitchenManager *manager) {
    for (int i = 0; i < NUM_RESOURCES; i++) {
        pthread_mutex_destroy(&manager->resources[i].mutex);
        sem_destroy(&manager->resources[i].semaphore);
    }
    pthread_mutex_destroy(&manager->kitchen_mutex);
}

void update_resource_quality(KitchenManager *manager, int resource_id) {
    KitchenResource *resource = &manager->resources[resource_id];

    pthread_mutex_lock(&resource->mutex);
    resource->usage_count++;

    if (resource->usage_count % 5 == 0) {
        resource->quality -= 10;
    }
    if (resource->usage_count % 3 == 0) {
        resource->state = RESOURCE_NEEDS_CLEANING;
    }

    if (resource->quality <= MIN_RESOURCE_QUALITY) {
        resource->quality = 0;
        resource->state = RESOURCE_BROKEN;
    }

    pthread_mutex_unlock(&resource->mutex);
}


void clean_resource(KitchenManager *manager, int resource_id) {
    KitchenResource *resource = &manager->resources[resource_id];
    
    pthread_mutex_lock(&resource->mutex);
    if (resource->state == RESOURCE_NEEDS_CLEANING) {
        sleep(CLEANING_TIME);
        resource->state = RESOURCE_AVAILABLE;
        resource->quality += 5;
        if (resource->quality > MAX_RESOURCE_QUALITY) {
            resource->quality = MAX_RESOURCE_QUALITY;
        }
    }
    pthread_mutex_unlock(&resource->mutex);
}

void perform_maintenance(KitchenManager *manager, int resource_id) {
    KitchenResource *resource = &manager->resources[resource_id];
    
    pthread_mutex_lock(&resource->mutex);
    if (resource->state == RESOURCE_BROKEN) {
        sleep(MAINTENANCE_TIME);
        resource->state = RESOURCE_AVAILABLE;
        resource->quality = MAX_RESOURCE_QUALITY;
        resource->usage_count = 0;
    }
    pthread_mutex_unlock(&resource->mutex);
}

int try_acquire_resource(KitchenManager *manager, int resource_id, int timeout_sec) {
    KitchenResource *resource = &manager->resources[resource_id];
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_sec;

    pthread_mutex_lock(&resource->mutex);
    
    if (resource->state != RESOURCE_AVAILABLE) {
        pthread_mutex_unlock(&resource->mutex);
        return -1;
    }
    
    if (sem_timedwait(&resource->semaphore, &ts) != 0) {
        pthread_mutex_unlock(&resource->mutex);
        return -1;
    }
    
    resource->state = RESOURCE_IN_USE;
    pthread_mutex_unlock(&resource->mutex);
    return 0;
}

void* kitchen_manager_work(void* arg) {
 (void)arg;
    while (1) {
        pthread_mutex_lock(&kitchen.kitchen_mutex);
        
       
        for (int i = 0; i < NUM_RESOURCES; i++) {
            if (kitchen.resources[i].state == RESOURCE_NEEDS_CLEANING) {
                clean_resource(&kitchen, i);
            } else if (kitchen.resources[i].state == RESOURCE_BROKEN) {
                perform_maintenance(&kitchen, i);
            }
        }
        
        pthread_mutex_unlock(&kitchen.kitchen_mutex);
        sleep(1);
    }
    return NULL;
}

void* chef_work(void* arg) {
    Chef *chef = (Chef *)arg;

    while (1) {
        if (chef->needs[0] > chef->needs[1]) {
            ResourceType temp = chef->needs[0];
            chef->needs[0] = chef->needs[1];
            chef->needs[1] = temp;
        }

        if (try_acquire_resource(&kitchen, chef->needs[0], 10) != 0) {
            printf(COLOR_RED "[Chef %d] ❌ Failed to acquire %s\n" COLOR_RESET,
                   chef->id, resource_names[chef->needs[0]]);
            chef->failure_count++;
            sleep(2);
            continue;
        }
        printf(COLOR_GREEN "[Chef %d] ✅ Acquired %s\n" COLOR_RESET,
               chef->id, resource_names[chef->needs[0]]);

        if (try_acquire_resource(&kitchen, chef->needs[1], 10) != 0) {
            printf(COLOR_RED "[Chef %d] ❌ Timed out waiting for %s\n" COLOR_RESET,
                   chef->id, resource_names[chef->needs[1]]);
            sem_post(&kitchen.resources[chef->needs[0]].semaphore);
            kitchen.resources[chef->needs[0]].state = RESOURCE_AVAILABLE;
            chef->failure_count++;
            sleep(2);
            continue;
        }
        printf(COLOR_GREEN "[Chef %d] ✅ Acquired %s\n" COLOR_RESET,
               chef->id, resource_names[chef->needs[1]]);

        printf(COLOR_YELLOW "[Chef %d] 🍳 Cooking... (Skill Level: %d)\n" COLOR_RESET,
               chef->id, chef->skill_level);
        sleep(COOKING_TIME - chef->skill_level); 

        
        update_resource_quality(&kitchen, chef->needs[0]);
        update_resource_quality(&kitchen, chef->needs[1]);

        printf(COLOR_GREEN "[Chef %d] 🎉 Finished cooking!\n" COLOR_RESET,
               chef->id);
        chef->success_count++;

        sem_post(&kitchen.resources[chef->needs[1]].semaphore);
        sem_post(&kitchen.resources[chef->needs[0]].semaphore);
        kitchen.resources[chef->needs[1]].state = RESOURCE_AVAILABLE;
        kitchen.resources[chef->needs[0]].state = RESOURCE_AVAILABLE;

        printf(COLOR_CYAN "[Chef %d] 🛠️ Released %s and %s\n" COLOR_RESET,
               chef->id, resource_names[chef->needs[0]], resource_names[chef->needs[1]]);

        printf(COLOR_CYAN "[Chef %d] 💤 Resting...\n" COLOR_RESET, chef->id);
        sleep(REST_TIME);
    }
    return NULL;
}
void* status_reporter(void* arg) {
    (void)arg;

    while (1) {
        sleep(10);

        pthread_mutex_lock(&kitchen.kitchen_mutex);

        printf("\n======= 🛠️  Kitchen Resource Status Report =======\n");
        for (int i = 0; i < NUM_RESOURCES; i++) {
            KitchenResource *res = &kitchen.resources[i];
            printf("%s: ", resource_names[res->type]);
            switch (res->state) {
                case RESOURCE_AVAILABLE:
                    printf(COLOR_GREEN "Available" COLOR_RESET);
                    break;
                case RESOURCE_IN_USE:
                    printf(COLOR_YELLOW "In Use" COLOR_RESET);
                    break;
                case RESOURCE_NEEDS_CLEANING:
                    printf(COLOR_CYAN "Needs Cleaning" COLOR_RESET);
                    break;
                case RESOURCE_BROKEN:
                    printf(COLOR_RED "Broken" COLOR_RESET);
                    break;
            }
            printf(" (Quality: %d%%, Usage: %d)\n", res->quality, res->usage_count);
        }
        printf("===================================================\n\n");

        pthread_mutex_unlock(&kitchen.kitchen_mutex);
    }

    return NULL;
}

