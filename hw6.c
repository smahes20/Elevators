#include "hw6.h"
#include <stdio.h>
#include<pthread.h>
#include <stdlib.h>


static struct Elevator{
    pthread_mutex_t passengerlock;
    pthread_mutex_t elevatorlock;
    int current_floor;
    int direction;
    int occupancy;
    int startPassenger;
    int endPassenger;
    enum {ELEVATOR_ARRIVED=1, ELEVATOR_OPEN=2, ELEVATOR_CLOSED=3} state;	
    pthread_barrier_t Elevator_Barrier;
    pthread_barrier_t Passenger_Barrier;
}Elevator_control[ELEVATORS];

void scheduler_init() {	
    int i = 0;
    for(i = 0; i < ELEVATORS; i++)
    {
        pthread_mutex_init(&(Elevator_control[i].passengerlock),0);
        pthread_mutex_init(&(Elevator_control[i].elevatorlock),0);
        pthread_barrier_init(&(Elevator_control[i].Elevator_Barrier), NULL, 2);
        pthread_barrier_init(&(Elevator_control[i].Passenger_Barrier), NULL, 2);
        Elevator_control[i].startPassenger= -1;
        Elevator_control[i].endPassenger= -1;
        Elevator_control[i].current_floor= 0;
        Elevator_control[i].direction= -1;
        Elevator_control[i].occupancy= 0;
        Elevator_control[i].state=ELEVATOR_ARRIVED;
    }
   }


void passenger_request(int passenger, int from_floor, int to_floor, 
        void (*enter)(int, int), 
        void(*exit)(int, int))
{
    // wait for the elevator to arrive at our origin floor, then get in
    int selectElevator = passenger % ELEVATORS; //maybe rand()
    
    pthread_mutex_lock(&(Elevator_control[selectElevator].passengerlock));

    //elevator properties
    Elevator_control[selectElevator].startPassenger = from_floor;
    Elevator_control[selectElevator].endPassenger = to_floor;

    pthread_barrier_wait(&(Elevator_control[selectElevator].Passenger_Barrier));
    enter(passenger, selectElevator);
    Elevator_control[selectElevator].occupancy++;

    // wait for the elevator at our destination floor, then get out
    pthread_barrier_wait(&(Elevator_control[selectElevator].Elevator_Barrier));
    pthread_barrier_wait(&Elevator_control[selectElevator].Passenger_Barrier);

    exit(passenger, selectElevator);
    Elevator_control[selectElevator].occupancy--;

    pthread_barrier_wait(&(Elevator_control[selectElevator].Elevator_Barrier));
    Elevator_control[selectElevator].startPassenger = -1;
    Elevator_control[selectElevator].endPassenger = -1;
    pthread_mutex_unlock(&(Elevator_control[selectElevator].passengerlock));
}

void elevator_ready(int elevator, int at_floor, 
        void(*move_direction)(int, int), 
        void(*door_open)(int), void(*door_close)(int)) {
    if(elevator!=0) return;

    pthread_mutex_lock(&(Elevator_control[elevator].elevatorlock));
    
    if(Elevator_control[elevator].state == ELEVATOR_ARRIVED){
        if ((Elevator_control[elevator].occupancy == 0 && Elevator_control[elevator].startPassenger == Elevator_control[elevator].current_floor) 
        || (Elevator_control[elevator].occupancy == 1 && Elevator_control[elevator].endPassenger == Elevator_control[elevator].current_floor)) {
            door_open(elevator);
            Elevator_control[elevator].state=ELEVATOR_OPEN;
            pthread_barrier_wait(&Elevator_control[elevator].Passenger_Barrier);
            pthread_barrier_wait(&Elevator_control[elevator].Elevator_Barrier);
            
        }
        else {
            Elevator_control[elevator].state=ELEVATOR_CLOSED;
        }
    }
    else if(Elevator_control[elevator].state == ELEVATOR_OPEN) {
        // pthread_barrier_wait(&Elevator_Barrier);
        door_close(elevator);
        Elevator_control[elevator].state=ELEVATOR_CLOSED;
    }
    else {
        if(at_floor==0 || at_floor==FLOORS-1
        (Elevator_control[elevator].direction == 1  && Elevator_control[elevator].passenger_start < at_floor && elevators[elevator].occupancy == 0) || 
			Elevator_control[elevator].direction == 1  && Elevator_control[elevator].passenger_end < at_floor && elevators[elevator].occupancy == 1)   ||
			Elevator_control[elevator].direction == -1 && Elevator_control[elevator].passenger_start > at_floor && elevators[elevator].occupancy == 0) ||
			Elevator_control[elevator].direction == -1 && Elevator_control[elevator].passenger_end > at_floor && elevators[elevator].occupancy == 1)
        ) {
            Elevator_control[elevator].direction*=-1;
        }
        move_direction(elevator,Elevator_control[elevator].direction);
        Elevator_control[elevator].current_floor=at_floor+Elevator_control[elevator].direction;
        Elevator_control[elevator].state=ELEVATOR_ARRIVED;
        
    }
    pthread_mutex_unlock(&(Elevator_control[elevator].elevatorlock));
   
}