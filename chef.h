#ifndef CHEF_H
#define CHEF_H

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define NUM_RESOURCES 6
#define NUM_CHEFS 10
#define COOKING_TIME 5
#define REST_TIME 3
#define MAX_RESOURCE_QUALITY 100
#define MIN_RESOURCE_QUALITY 0
#define CLEANING_TIME 2
#define MAINTENANCE_TIME 5

// Resource states
typedef enum {
    RESOURCE_AVAILABLE,
    RESOURCE_IN_USE,
    RESOURCE_NEEDS_CLEANING,
    RESOURCE_BROKEN
} ResourceState;

// Resource types
typedef enum {
    STOVE,
    KNIFE,
    CHOPPING_BOARD,
    VEGETABLES,
    COOKING_POT,
    SPICES
} ResourceType;

// Chef skill levels
typedef enum {
    NOVICE,
    INTERMEDIATE,
    EXPERT
} ChefSkill;

// Resource structure
typedef struct {
    ResourceType type;
    ResourceState state;
    int quality;
    int usage_count;
    pthread_mutex_t mutex;
    sem_t semaphore;
} KitchenResource;

// Chef structure
typedef struct {
    int id;
    ResourceType needs[2];
    ChefSkill skill_level;
    int success_count;
    int failure_count;
} Chef;

// Kitchen manager structure
typedef struct {
    KitchenResource resources[NUM_RESOURCES];
    int total_meals_prepared;
    int total_failures;
    pthread_mutex_t kitchen_mutex;
} KitchenManager;
extern KitchenManager kitchen;
// Function declarations
void initialize_chef_needs(Chef *chef);
void initialize_resources(KitchenManager *manager);
void cleanup_resources(KitchenManager *manager);
int try_acquire_resource(KitchenManager *manager, int resource_id, int timeout_sec);
void* chef_work(void* arg);
void* kitchen_manager_work(void* arg);
void update_resource_quality(KitchenManager *manager, int resource_id);
void perform_maintenance(KitchenManager *manager, int resource_id);
void clean_resource(KitchenManager *manager, int resource_id);
void* status_reporter(void* arg);
#endif // CHEF_H 
