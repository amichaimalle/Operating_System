#include <stdio.h>
//#include <unistd.h>
#include <stdlib.h>
//#include <sys/fcntl.h>
#include <signal.h>
//#include <sys/wait.h>
#include <time.h>
#include <pthread.h>

#define N 10 //number of slot in each buffer
#define M 4 //number of buffer = number of corner
#define FIN_PROB 0.1 //probability of a process to finish
#define MIN_INTER_ARRIVAL_IN_NS 8000000 //8ms
#define MAX_INTER_ARRIVAL_IN_NS 9000000 //9ms
#define INTER_MOVES_IN_NS 100000 //0.1ms
#define SIM_TIME 2 //2s

typedef struct slot {
    int occupied;
    int isEntryPoint;
    pthread_mutex_t mutex;
    struct slot *nextSlot;
}slot;

//prototype
slot *init();
int *findPrevSlotOccupied(slot *entryPoint);
void *CreateNewCars(void *Slot);
void *move(void *Slot);
void printStatus(slot *head);
void exitHandler(slot *head);

//global variable
int simRunning = 1;

int main(){
    struct timespec simTime;
    slot *tempSlot;
    slot *head = init();
    tempSlot = head;
    pthread_t carGeneration[M];
    for (int i=0; i<(M*(N-1)); i++){
        if (tempSlot->isEntryPoint){
            if(pthread_create(&carGeneration[i], NULL, CreateNewCars ,tempSlot) != 0){
                perror("Cannot create thread\n");
                exit(EXIT_FAILURE);
            }
        }
        tempSlot = tempSlot->nextSlot;
    }
    for (int i=0; i<10; i++){
        simTime.tv_sec = 0;
        simTime.tv_nsec = 200000000;
        nanosleep(&simTime, NULL);
        printStatus(head);
    }
    simRunning = 0;
    simTime.tv_sec = 0;
    simTime.tv_nsec = MAX_INTER_ARRIVAL_IN_NS;
    nanosleep(&simTime, NULL);
    exitHandler(head);
    return 0;
}
slot *init(){
    //create the first slot
    slot *firstSlot = (slot*)malloc(sizeof(slot));
    firstSlot->occupied = 0;
    firstSlot->nextSlot = NULL;
    //initialize the mutex
    if(pthread_mutex_init(&firstSlot->mutex, NULL) != 0){
        perror("Cannot initialize mutex\n");
        exit(EXIT_FAILURE);
    }
    // create the rest of the slots
    slot *currentSlot = firstSlot;
    for (int i=1; i<(M*(N-1)); i++){
        slot *newSlot = (slot*)malloc(sizeof(slot));
        newSlot->occupied = 0;
        if (i%(N-1)==0) {
            newSlot->isEntryPoint = 1;
        } else newSlot->isEntryPoint = 0;
        if(pthread_mutex_init(&newSlot->mutex, NULL) != 0){
            perror("Cannot initialize mutex\n");
            exit(EXIT_FAILURE);
        } //initialize the mutex
        currentSlot->nextSlot = newSlot;
        currentSlot = newSlot;
    }
    //last slot Point to the first slot
    currentSlot->nextSlot = firstSlot;
    return firstSlot;
}
void *CreateNewCars(void *Slot) {
    slot *entryPoint = (slot *) Slot;
    struct timespec req;
    req.tv_sec = 0;
    pthread_t newCar;
    int *prevSlotOccupied = findPrevSlotOccupied(entryPoint);
    while (simRunning) {
        req.tv_nsec = MIN_INTER_ARRIVAL_IN_NS + (rand() % (MAX_INTER_ARRIVAL_IN_NS - MIN_INTER_ARRIVAL_IN_NS));
        nanosleep(&req, NULL);
        //create a new car
        //pthread_t *newCar = (pthread_t *) malloc(sizeof(pthread_t));
        while (entryPoint->occupied ==1 || *prevSlotOccupied==1);
        pthread_mutex_lock(&entryPoint->mutex);
        entryPoint->occupied = 1;
        //entryPoint->occupied_by = newCar;
        pthread_mutex_unlock(&entryPoint->mutex);
        if(pthread_create(&newCar, NULL, move ,entryPoint) != 0){
            perror("Cannot create thread\n");
            exit(EXIT_FAILURE);
        }
    }
    pthread_exit(NULL);
    return NULL;
}
int *findPrevSlotOccupied(slot *entryPoint){
    slot *tempSlot = entryPoint;
    while (tempSlot->nextSlot != entryPoint){
        tempSlot = tempSlot->nextSlot;
    }
    return &tempSlot->occupied;
}
void *move(void *Slot) {
    int newArrival = 1;
    slot *currentSlot = (slot *)Slot;
    slot *nextSlot = currentSlot->nextSlot;
    //pthread_t *carId = (pthread_t *)pthread_self();
    struct timespec moveReq;
    moveReq.tv_sec = 0;
    moveReq.tv_nsec = INTER_MOVES_IN_NS;
    while (simRunning){
        //move the car
        nanosleep(&moveReq, NULL);
        if ((currentSlot->isEntryPoint == 1) && (newArrival == 0)) {
            if ((double)rand() / (double)RAND_MAX < FIN_PROB) {
                //the car will leave the parking
                pthread_mutex_lock(&currentSlot->mutex);
                currentSlot->occupied = 0;
                //currentSlot->occupied_by = NULL;
                pthread_mutex_unlock(&currentSlot->mutex);
                pthread_exit(NULL);
            }
        }
        //check if the next slot is occupied
        while (nextSlot->occupied == 1){};
            //free the current slot
            pthread_mutex_lock(&currentSlot->mutex);
            currentSlot->occupied = 0;
            //currentSlot->occupied_by = NULL;
            pthread_mutex_unlock(&currentSlot->mutex);
            //move the car to the next slot
            pthread_mutex_lock(&nextSlot->mutex);
            nextSlot->occupied = 1;
            //nextSlot->occupied_by = carId;
            pthread_mutex_unlock(&nextSlot->mutex);
            //update the current slot
            currentSlot = nextSlot;
            nextSlot = nextSlot->nextSlot;
        newArrival = 0;
    }
    pthread_exit(NULL);
    return NULL;
}
void printStatus(slot *head){
    slot *tempSlot = head;
    char status[N][N];
    for (int i=N-1; i>=0; i--){
        for (int j=0; j<N; j++){
            if ((i==0) || (i==N-1) || (j==0) || (j==N-1)){
                if (tempSlot->occupied == 1){
                    status[i][j] = '*';
                } else status[i][j] = ' ';
                tempSlot = tempSlot->nextSlot;
            } else status[i][j] = '@';
        }
    }
    for (int i=0; i<N; i++){
        for (int j=0; j<N; j++){
            printf("%c", status[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}
void exitHandler(slot *head){
    slot *tempSlot = head->nextSlot;
    while (tempSlot->nextSlot != head){
        slot *slotToEnd = tempSlot;
        tempSlot = tempSlot->nextSlot;
        if (pthread_mutex_destroy(&slotToEnd->mutex) != 0){
            perror("Cannot destroy mutex\n");
            exit(EXIT_FAILURE);
        }
        free(slotToEnd);
    }
}