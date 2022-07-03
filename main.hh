#pragma once 

#include <mpi.h>
#include <stdio.h>
#include <random>
#include <iostream>
#include <ctime>

#define REQUEST 10
#define ACK 20
#define RESPONSE 30
#define RELEASE 40

#define WINE_LIMIT 20
#define SAFESPOTS 6

int rank;
int size;
int safeSpotSize;
int numStudents;
int numWineMakers;
int lamportClock;
//int wineAmount; // Wine produced by winemakers OR wine needed by students

//int *safeSpots = nullptr;

struct safeSpot {int spotId; int wineAmount;};
struct message {int clock; safeSpot spot;};
struct msgStatus {int source; int tag;};
struct packet {message msg; msgStatus status;};

int genWine(int * clock, int rank);
void send(int *clock, int msgType, int spotId, int amount, int dest, int src);
packet recv(int *clock);
int chooseSpot(int * clock, int rank, int numSpaces);