#include "main.hh"

// to produce or on demand
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

int compareClocks(int *selfClock, int *recvClock, int currentProc, int recvProc){
    std::cout << "      my (" << currentProc <<  ") clock: " << *selfClock << " recv (" << recvProc << ") clock: " << *recvClock << "\n";
    if (*selfClock < *recvClock) { *selfClock += 1; return -1; }
    if (*selfClock > *recvClock) { *selfClock += 1; return 1; }
    if (*selfClock == *recvClock){
        *selfClock += 1;
        if (currentProc < recvProc) return 1;
        else return -1;
    }
    return 0;
}

void send(int *clock, int msgType, int spotId, int amount, int dest, int src, int lowerlimit, int upperlimit, Queue &requestQueue) {
    safeSpot spot;
    message msg;
    packet pck;
    *clock+=1;
    msg.clock = *clock;
    msg.spot.spotId =spotId;
    switch (msgType)
    {
    case REQUEST:
        msg.spot.wineAmount = 0;
        pck.msg = msg;
        pck.status.source = src;
        pck.status.tag = REQUEST;
        requestQueue.push(pck);
        for (int i = lowerlimit; i < upperlimit; i++) {
            if(i != src){
                MPI_Send( &msg , sizeof(msg) , MPI_BYTE , i , REQUEST , MPI_COMM_WORLD);
            }  
        }
        break;
    case ACK:
        msg.spot.wineAmount = 0;
        MPI_Send( &msg , sizeof(msg) , MPI_BYTE , dest , ACK , MPI_COMM_WORLD);
        break;
    case RESPONSE:
        msg.spot.wineAmount = 0;
        MPI_Send( &msg , sizeof(msg) , MPI_BYTE , dest , RESPONSE , MPI_COMM_WORLD);
        std::cout << "resp sent\n";
        break;
    case RELEASE:
        msg.spot.wineAmount = amount;
        for (int i = lowerlimit; i < upperlimit; i++) {
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
    Queue requestQueue;
    for (int i = 0; i < safeSpotSize; i++) {
        safeSpots[i] = 0;
        isSpotFree[i] = 1;
    }
    int wineAmount = 0;
    int selectedSpot = -1;
    int lamportClock = 0;
    int ackCounter = 0;
    int requestFlag = 0;
    int releaseFlag = 0;

    packet myPacket;

    //if (safeSpots != nullptr) std::cout << "Array created!\n";
    std::cout << "NumWIneMakers: " << numWineMakers << std::endl;
    int i = 0;
    while(i < 20) {

        //std::cout << "My rank is: " << rank << " from " << size << " safeSpotSize: " << safeSpotSize << std::endl;

        if (wineAmount == 0) wineAmount = genWine(&lamportClock, rank);
        if (selectedSpot == -1) selectedSpot = chooseSpot(&lamportClock, rank, safeSpotSize);

        if (rank < numWineMakers) {

            if (selectedSpot != -1 && safeSpots[selectedSpot] == 0 && isSpotFree[selectedSpot] == 1 && requestFlag == 0) {
                send(&lamportClock, REQUEST, selectedSpot, wineAmount, 0, rank, 0, numWineMakers, requestQueue); 
                requestFlag = 1;
                // if (!requestQueue.empty()) std::cout << "#sus\n"; 
                //safeSpots[selectedSpot] = 1;  ??? wut
            }

            if (ackCounter == numWineMakers -1 && releaseFlag == 0) {
                send(&lamportClock, RELEASE, selectedSpot, wineAmount, 0, rank, 0, size, requestQueue);
                releaseFlag = 1;
                safeSpots[selectedSpot] = wineAmount;
                lamportClock++;
                std::cout << "  Going to spot " << selectedSpot << " with " << wineAmount << " wine units.\n";
            }

            myPacket = recv(&lamportClock);

            std::cout << "UmU winemaker " << rank << " received message of type " << myPacket.status.tag 
                        << " from " << myPacket.status.source << std::endl;

            // TODO
            switch (myPacket.status.tag)
            {
            case REQUEST:
                std::cout << "  Winemaker " << myPacket.status.source << " wants place no. " << myPacket.msg.spot.spotId << std::endl;
                requestQueue.push(myPacket);
                if (myPacket.msg.spot.spotId != selectedSpot) {
                    send(&lamportClock, ACK, myPacket.msg.spot.spotId, 0, myPacket.status.source, rank, 0, 0, requestQueue);
                    isSpotFree[myPacket.msg.spot.spotId] = 0;
                    lamportClock++;
                } 
                else{
                    int leader = compareClocks(&lamportClock, &myPacket.msg.clock, rank, myPacket.status.source);
                    //std::cout << "leader " << leader << "\n";
                    if(leader == -1){
                        std::cout << "Lost battle to winemaker " << myPacket.status.source << std::endl;
                        send(&lamportClock, RESPONSE, myPacket.msg.spot.spotId, 0, myPacket.status.source, rank, 0, 0, requestQueue);
                        isSpotFree[myPacket.msg.spot.spotId] = 0;
                        lamportClock++;
                        ackCounter = 0;
                        selectedSpot = -1;
                        requestFlag = 0;
                    } else {
                        std::cout << "Won battle; requesting ACK from " << myPacket.status.source << std::endl;
                    }
                }
                // TODO: rywalizacja o to samo miejsce

                break;
            case ACK:
                if (myPacket.msg.spot.spotId == selectedSpot) {ackCounter++; lamportClock++;}
                std::cout << "  Got ACK from " << myPacket.status.source << " regarding spot " << myPacket.msg.spot.spotId 
                            << ". Current ACKs: " << ackCounter << std::endl;
                break;
            case RESPONSE:
                std::cout << " Response from " << myPacket.status.source << "\n";
                break;
            case RELEASE:
                if (myPacket.msg.spot.wineAmount > 0 ) {
                    isSpotFree[myPacket.msg.spot.spotId] = 0;
                    safeSpots[myPacket.msg.spot.spotId] = myPacket.msg.spot.wineAmount;
                    lamportClock+=2;

                    if (myPacket.status.source < numWineMakers) {
                         std::cout << "      Winemaker " << myPacket.status.source << " is in spot " << myPacket.msg.spot.spotId
                            << " with " << myPacket.msg.spot.wineAmount << " wine units\n";  
                    } else {
                        std::cout << "      Student " << myPacket.status.source << " bought wine from spot " << myPacket.msg.spot.spotId << std::endl; 
                    }
                } else {
                    isSpotFree[myPacket.msg.spot.spotId] = 1;
                    safeSpots[myPacket.msg.spot.spotId] = 0;
                    lamportClock+=2;

                    std::cout << "      Student " << myPacket.status.source << " emptied spot " << myPacket.msg.spot.spotId << std::endl; 
                }
                
                break;
            }

            std::cout << "P  " << rank << " status: " << lamportClock << "clock " << selectedSpot << "spot " 
                        <<  std::endl;
            
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