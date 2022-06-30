#include "main.hh"

int genWine(int *clock, int rank) {
    srand(time(NULL) + rank);
    *clock+=1;
    return rand() % WINE_LIMIT + 1;
}

int chooseSpot(int *clock, int rank, int numSpaces) {
    srand(time(NULL) + rank);
    *clock+=1;
    return rand() % numSpaces;    
}

void send(int *clock, int msgType, int spotId, int amount, int dest, int src) {
    safeSpot spot {.spotId = spotId, .wineAmount = amount};
    message msg {.clock = *clock, .spot = spot};
    *clock+=1;
    switch (msgType)
    {
    case REQUEST:
        msg.clock = *clock;
        msg.spot.spotId =spotId;
        msg.spot.wineAmount = 0;
        for (int i = 0; i < numWineMakers; i++) {
            if(i != src){
                MPI_Send( &msg , sizeof(msg) , MPI_BYTE , i , REQUEST , MPI_COMM_WORLD);
            }  
        }
        break;
    case ACK:
        msg.clock = *clock;
        msg.spot.spotId =spotId;
        msg.spot.wineAmount = 0;
        MPI_Send( &msg , sizeof(msg) , MPI_BYTE , dest , ACK , MPI_COMM_WORLD);
        break;
    case RESPONSE:
        /* code */
        break;
    case RELEASE:
        msg.clock = *clock;
        msg.spot.spotId =spotId;
        msg.spot.wineAmount = amount;
        for (int i = 0; i < size; i++) {
            if(i != src){
                MPI_Send( &msg , sizeof(msg) , MPI_BYTE , i , RELEASE , MPI_COMM_WORLD);
            }  
        }
        break;
    }
    
    
}

packet recv(int *clock) {
    packet tmp;
    safeSpot spot;
    message msg;
    MPI_Status status;
    MPI_Recv( &msg , sizeof(msg) , MPI_BYTE , MPI_ANY_SOURCE , MPI_ANY_TAG , MPI_COMM_WORLD , &status);
    *clock = std::max(*clock, msg.clock) + 1;

    tmp.msg = msg;
    tmp.status.source = status.MPI_SOURCE;
    tmp.status.tag = status.MPI_TAG;
    
    // std::cout << "status: " << status.MPI_SOURCE << " tag: " << status.MPI_TAG << std::endl;
    // std::cout << " clock: " << msg.clock << " spot: " << msg.spot.spotId << " wine avaliable: " << msg.spot.wineAmount << std::endl;
    return tmp;
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    MPI_Comm_size( MPI_COMM_WORLD , &size);
    MPI_Comm_rank( MPI_COMM_WORLD , &rank);

   switch (argc)
   {
   case 2:
        safeSpotSize = atoi(argv[1]);
        numWineMakers = size/2;
    break;
    case 3:
        safeSpotSize = atoi(argv[1]);
        numWineMakers = atoi(argv[2]);
    break;
   default:
    safeSpotSize = SAFESPOTS;
    numWineMakers = size/2;
    break;
   }

    int *safeSpots = new int[safeSpotSize];
    int *isSpotFree = new int[safeSpotSize]; // free space: 1; taken: 0
    for (int i = 0; i < safeSpotSize; i++) {
        safeSpots[i] = 0;
        isSpotFree[i] = 1;
    }
    int wineAmount = 0;
    int selectedSpot;
    int lamportClock = 0;
    int ackCounter = 0;

    packet myPacket;

    //if (safeSpots != nullptr) std::cout << "Array created!\n";

    int i = 0;
    while(i < 5) {

        //std::cout << "My rank is: " << rank << " from " << size << " safeSpotSize: " << safeSpotSize << std::endl;

        if (wineAmount == 0) wineAmount = genWine(&lamportClock, rank);
        selectedSpot = chooseSpot(&lamportClock, rank, safeSpotSize);

        if (rank < numWineMakers) {

            if (safeSpots[selectedSpot] == 0) {
                send(&lamportClock, REQUEST, selectedSpot, wineAmount, 0, rank);
                safeSpots[selectedSpot] = 1;
            }

            myPacket = recv(&lamportClock);

            std::cout << "UmU winemaker " << rank << " received message of type " << myPacket.status.tag 
                        << " from " << myPacket.status.source << std::endl;

            // TODO
            switch (myPacket.status.tag)
            {
            case REQUEST:
                std::cout << "  Winemaker " << myPacket.status.source << " wants place no. " << myPacket.msg.spot.spotId << std::endl;
                if (myPacket.msg.spot.spotId != selectedSpot) {
                    send(&lamportClock, ACK, myPacket.msg.spot.spotId, 0, myPacket.status.source, rank);
                }
                break;
            case ACK:
                if (myPacket.msg.spot.spotId == selectedSpot) ackCounter++;
                std::cout << "  Got ACK from " << myPacket.status.source << " regarding spot " << myPacket.msg.spot.spotId << std::endl;
                break;
            case RESPONSE:
                /* code */
                break;
            case RELEASE:
                /* code */    
                break;
            }

            std::cout << "  clock: " << lamportClock << std::endl;
            
        } else {
            // std::cout << "I'm a student! OwO I chose " << selectedSpot << std::endl;
            // recv(&lamportClock);
            // std::cout << "clock: " << lamportClock << " wine needed: " << wineAmount << std::endl;
        }
        i++;
    }
    

    MPI_Finalize();
    delete [] safeSpots;
    return 0;
}