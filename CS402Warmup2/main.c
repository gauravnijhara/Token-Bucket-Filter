//
//  main.c
//  CS402Warmup2
//
//  Created by Gaurav Nijhara on 2/9/16.
//  Copyright © 2016 Gaurav Nijhara. All rights reserved.
//

#include <stdio.h>
#include "my402list.h"
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>
#include "string.h"
#include <signal.h>


// check if token bucket can be stack
sigset_t quitSignal;
struct sigaction action;

My402List Q1,Q2,tokenBucket;
pthread_mutex_t Q1Mutex;
pthread_cond_t serverQ;
unsigned int packetCount = 0;
unsigned int packetsServed = 0;
unsigned int serveInterrupt = 0;
unsigned int isTraceFileMode = 0;

double lambda = 1,mu = 0.35,r = 1.5, b = 10 ,p = 3,num = 20;
char *fileName;
pthread_t packetThread,tokenThread,serverThread1,serverThread2,signalQuitThread;

typedef struct packet
{
    int ID;
    int tokensNeeded;
    double interArrivalTime;
    double serviceTime;
    struct timeval Q1EnterTime,Q2EnterTime,systemTimeOnEnter,systemTimeOnExit,serviceStartTime,serviceEndTime;
    
}packet;

packet **packetList;

void *packetArrivalMethod(void *args);
void *tokenArrivalMethod(void *args);
void *serverMethod(void *args);
void *server2Method(void *args);
void *handleQuitGracefully(void *args);
void handleQuit(int signal);


int main(int argc, const char * argv[]) {
    
    printf("Emulation Parameters:\n");
    int i;
    for (i = 1; i < argc; i++) {
        
        if (strcmp(argv[i],"-lambda") == 0) {
            
            if ((i+1 != argc-1) && (1 == sscanf(argv[i+1],"%lf",&lambda))) {
                if (lambda < 0) {
                    fprintf(stderr,"Malformed Command , lambda value must be greater than 0");
                    exit(0);
                }
            }else {
                fprintf(stderr,"Malformed Command , lambda value not provided");
                exit(0);
            }
        }else if (strcmp(argv[i],"-mu") == 0) {
            
            if ((i+1 != argc-1) && (1 == sscanf(argv[i+1],"%lf",&mu))) {
                if (lambda < 0) {
                    fprintf(stderr,"Malformed Command , Mu value must be greater than 0");
                    exit(0);
                }
            }else {
                fprintf(stderr,"Malformed Command , Mu value not provided");
                exit(0);
            }
        }else if (strcmp(argv[i],"-r") == 0) {
            
            if ((i+1 != argc-1) && (1 == sscanf(argv[i+1],"%lf",&r))) {
                if (lambda < 0) {
                    fprintf(stderr,"Malformed Command , Rate of tokens must be greater than 0");
                    exit(0);
                }
            }else {
                fprintf(stderr,"Malformed Command , Rate of tokens value not provided");
                exit(0);
            }
        }else if (strcmp(argv[i],"-b") == 0) {
            
            if ((i+1 != argc-1) && (1 == sscanf(argv[i+1],"%lf",&b))) {
                if (lambda < 0) {
                    fprintf(stderr,"Malformed Command , Bucket size must be greater than 0");
                    exit(0);
                }
            }else {
                fprintf(stderr,"Malformed Command , Bucket size value not provided");
                exit(0);
            }
        }else if (strcmp(argv[i],"-p") == 0) {
            
            if ((i+1 != argc-1) && (1 == sscanf(argv[i+1],"%lf",&p))) {
                if (lambda < 0) {
                    fprintf(stderr,"Malformed Command , token per packet must be greater than 0");
                    exit(0);
                }
            }else {
                fprintf(stderr,"Malformed Command , tokens per packet value not provided");
                exit(0);
            }
        }else if (strcmp(argv[i],"-n") == 0) {
            
            if ((i+1 != argc-1) && (1 == sscanf(argv[i+1],"%lf",&num))) {
                if (num < 0) {
                    fprintf(stderr,"Malformed Command , Packet count must be greater than 0");
                    exit(0);
                }
            }else {
                fprintf(stderr,"Malformed Command , Packet count value not provided");
                exit(0);
            }
        }else if (strcmp(argv[i],"-t") == 0) {
        
            if ((i+1 != argc-1) && (1 == sscanf(argv[i+1],"%s",fileName))) {
                
                
                isTraceFileMode = 1;
                
                FILE *fp;
                struct stat fileCheck;
                
                if( stat(fileName,&fileCheck) == 0 )
                {
                    if( fileCheck.st_mode & S_IFREG )
                    {
                        if(access(fileName,R_OK) != 0)
                        {
                            fprintf(stderr, " Input file %s cannot be opened : access denied \n",fileName);
                            exit(1);
                        }
                        else
                        {
                            fp = fopen(fileName, "r");
                            if (!fp) {
                                fprintf(stderr, "Malformed command or file: %s doesn't exist \n",fileName);
                                exit(1);
                            }
                            
                            
                            char buffer[1026];
                            int iterator = 0;
                            while(fgets(buffer, sizeof(buffer)/sizeof(char), fp) != NULL) {
                                
                                if (!packetList) {
                                    
                                    int tokenCount = atoi(buffer);
                                    packetList = (packet**)malloc(sizeof(tokenCount));
                                    
				    int i;
                                    for (i = 0; i < tokenCount; i++) {
                                        packet *newPacket = (packet*)malloc(sizeof(packet));
                                        newPacket->ID = i+1;
                                        packetList[i] = newPacket;
                                    }
                                    
                                }
                                else {
                                    
                                    // parsing code
                                    const char s[2] = "\t";
                                    char *token;
                                    
                                    // inter-arrival time
                                    token = strtok(buffer, s);
                                    double interArrivalTime = atoll(token);
                                    packetList[iterator]->interArrivalTime = interArrivalTime;
                                    
                                    // tokens needed
                                    token = strtok(NULL, s);
                                    int tokensNeeded = atoi(token);
                                    packetList[iterator]->tokensNeeded = tokensNeeded;
                                    
                                    // service time
                                    token = strtok(NULL, s);
                                    double serviceTime = atoll(token);
                                    packetList[iterator]->serviceTime = serviceTime;
                                    
                                }
                                iterator++;
                            }

                            // file opened here
                            fclose(fp);
                        }
                    }
                    else if( fileCheck.st_mode & S_IFDIR )
                    {
                        fprintf(stderr, " Input path %s is a directory \n",fileName);
                        exit(1);
                        
                    }
                    else
                    {
                        fprintf(stderr, " Input file %s does not exist \n",fileName);
                        exit(1);
                    }
                }
                else
                {
                    fprintf(stderr, " Input file should be %s does not exist \n",fileName);
                    exit(1);
                }

            }else {
                fprintf(stderr,"Malformed Command , Filename not in proper format ");
                exit(0);
            }
        }
        else {
            fprintf(stderr,"Malformed Command , Filename not in proper format ");
            exit(0);
        }
    }
    
    if (!isTraceFileMode) {
        
	int i;
        for (i = 0; i < num; i++) {
            packet *newPacket = (packet*)malloc(sizeof(packet));
            newPacket->ID = i+1;
            newPacket->interArrivalTime = (1/lambda);
            newPacket->tokensNeeded = p;
            newPacket->serviceTime = (1/mu);
            packetList[i] = newPacket;
        }
    }
    
    sigemptyset(&quitSignal);
    sigaddset(&quitSignal, SIGINT);
    pthread_sigmask(SIG_BLOCK, &quitSignal, NULL);

    pthread_create(&packetThread, NULL,packetArrivalMethod,NULL);
    pthread_create(&tokenThread, NULL,tokenArrivalMethod, NULL);
    pthread_create(&serverThread1, NULL,serverMethod, NULL);
    pthread_create(&serverThread2, NULL,server2Method, NULL);
    pthread_create(&signalQuitThread, NULL,handleQuitGracefully, NULL);

    
    pthread_join(packetThread, NULL);
    pthread_join(tokenThread, NULL);
    pthread_join(serverThread1, NULL);
    pthread_join(serverThread2, NULL);
    pthread_join(signalQuitThread,NULL);
    
    return 0;
}

void *packetArrivalMethod(void *args)
{
    long long prevTokenArrivalTime=0,currentTime,elapsedTime = 0;
    struct timeval time;
    
    while (packetCount < num) {
        
        usleep((unsigned int)fabs((packetList[packetCount]->interArrivalTime)*1000000L - elapsedTime/1000000L));
        
        gettimeofday(&time,NULL);
        currentTime = time.tv_sec + time.tv_usec*1000000L;
        

        packet *newPacket = packetList[packetCount++];
        
        // packetCount
        printf("\n packet%d arrives , need %d tokens, inter-arrival time = %lld",newPacket->ID,newPacket->tokensNeeded,(currentTime-prevTokenArrivalTime));
        
        prevTokenArrivalTime = currentTime;

        newPacket->systemTimeOnEnter = time;
        
        pthread_mutex_lock(&Q1Mutex);
        {
            
            if (newPacket->tokensNeeded > b) {
                packetsServed++;
                gettimeofday(&time,NULL);
                elapsedTime = time.tv_sec + time.tv_usec*1000000L - currentTime;
                pthread_mutex_unlock(&Q1Mutex);
                continue;
            }
            
            
            gettimeofday(&newPacket->Q1EnterTime, NULL);
            printf("\n packet%d enters Q1",newPacket->ID);
            
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
            My402ListAppend(&Q1,(void*)newPacket);
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
            
            packet *dequePacket = (packet*)My402ListFirst(&Q1)->obj;

            
            if (dequePacket->tokensNeeded <= tokenBucket.num_members) {
                
                
                int i = 0;
                while (i < dequePacket->tokensNeeded) {
                    My402ListUnlink(&tokenBucket,My402ListFirst(&tokenBucket));
                    i++;
                }
                
                
                struct timeval temp;
                gettimeofday(&temp, NULL);
                printf("\n p%d leaves Q1, time in Q1 = %ld, token bucket now has %d token",dequePacket->ID,(temp.tv_sec + temp.tv_usec*1000000L) - (dequePacket->Q1EnterTime.tv_sec + dequePacket->Q1EnterTime.tv_usec*1000000L),Q1.num_members);
                
                pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
                My402ListUnlink(&Q1, My402ListFirst(&Q1));
                pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);

                gettimeofday(&newPacket->Q2EnterTime, NULL);
                printf("\n packet%d enters Q2",newPacket->ID);
                
                pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
                My402ListAppend(&Q2,(packet*)dequePacket);
                pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);


            }
        }
        
        if (!My402ListEmpty(&Q2)) {
            pthread_cond_broadcast(&serverQ);
        }

        pthread_mutex_unlock(&Q1Mutex);
        
        
        gettimeofday(&time,NULL);
        elapsedTime = time.tv_sec + time.tv_usec*1000000L - currentTime;

    }
    pthread_exit(0);
    
}


void *tokenArrivalMethod(void *args)
{
    long long currentTime,elapsedTime=0;
    struct timeval time;

    while (1) {
        
        usleep((unsigned int)fabs((1/r)*1000000L -elapsedTime/1000000L));

        gettimeofday(&time,NULL);
        currentTime = time.tv_sec + time.tv_usec*1000000L;

        pthread_mutex_lock(&Q1Mutex);
        {

            if (tokenBucket.num_members <= 100) {
                int *token = (int*)malloc(sizeof(int));
                My402ListAppend(&tokenBucket,token);
            }
            
            if (Q1.num_members <= 0) {
                pthread_mutex_unlock(&Q1Mutex);
                continue;
            }
            
            packet *dequePacket = (packet*)My402ListFirst(&Q1)->obj;

            if (dequePacket->tokensNeeded <= tokenBucket.num_members) {
                
                
                    int i = 0;
                    while (i < dequePacket->tokensNeeded) {
                        My402ListUnlink(&tokenBucket,My402ListFirst(&tokenBucket));
                        i++;
                    }
                    
                    struct timeval temp;
                    gettimeofday(&temp, NULL);
                    
                    printf("\n p%d leaves Q1, time in Q1 = %ld, token bucket now has %d token",dequePacket->ID,(temp.tv_sec + temp.tv_usec*1000000L) - (dequePacket->Q1EnterTime.tv_sec + dequePacket->Q1EnterTime.tv_usec*1000000L),Q1.num_members);
                
                    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
                    My402ListUnlink(&Q1, My402ListFirst(&Q1));
                    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);

                    printf("packet%d enters Q2\n",dequePacket->ID);
                
                    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
                    My402ListAppend(&Q2,(packet*)dequePacket);
                    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);

                }
            
                if (!My402ListEmpty(&Q2)) {
                    pthread_cond_broadcast(&serverQ);
                }
            
            pthread_mutex_unlock(&Q1Mutex);
            
            if (packetsServed == num) {
                break;
            }

        }
        
        gettimeofday(&time,NULL);
        elapsedTime = time.tv_sec + time.tv_usec*1000000L - currentTime;

    }
    pthread_exit(0);
}

void *serverMethod(void *args)
{
    long long currentTime = 0,elapsedTime;

    while (1) {
        
        pthread_mutex_lock(&Q1Mutex);
        

        while (My402ListEmpty(&Q2) && packetsServed != num && !serveInterrupt) {
            pthread_cond_wait(&serverQ, &Q1Mutex);
        }
        
        if (packetsServed == num || serveInterrupt) {
            pthread_mutex_unlock(&Q1Mutex);
            break;
        }

        packet *dequePacket = (packet*)My402ListFirst(&Q2)->obj;
        struct timeval temp;
        gettimeofday(&temp, NULL);
        
        My402ListUnlink(&Q2,My402ListFirst(&Q2));

        printf("\n p%d leaves Q2, time in Q2 = %ld",dequePacket->ID,(temp.tv_sec + temp.tv_usec*1000000L) - (dequePacket->Q2EnterTime.tv_sec + dequePacket->Q2EnterTime.tv_usec*1000000L));
        
        packetsServed++;
        
        pthread_mutex_unlock(&Q1Mutex);
        
        gettimeofday(&dequePacket->serviceStartTime,NULL);
        elapsedTime = dequePacket->serviceStartTime.tv_sec + dequePacket->serviceStartTime.tv_usec*1000000L - currentTime;

        printf("\n p%d begins service at S1, requesting %dms of service",dequePacket->ID,100);
     
        usleep((unsigned int)fabs((1/mu)*1000000L - elapsedTime/1000000L));

        gettimeofday(&dequePacket->serviceEndTime,NULL);
        currentTime = dequePacket->serviceEndTime.tv_sec + dequePacket->serviceEndTime.tv_usec*1000000L;

        printf("\n p% departs from S1, service time = %lldms , time in system = %ldms",dequePacket->ID,(currentTime - (dequePacket->serviceStartTime.tv_sec + dequePacket->serviceStartTime.tv_usec*1000)),(dequePacket->serviceStartTime.tv_sec + dequePacket->serviceStartTime.tv_usec*1000000L) - (dequePacket->serviceEndTime.tv_sec + dequePacket->serviceEndTime.tv_usec*1000000L));
        
        if (packetsServed == num || serveInterrupt) {
            if (packetsServed == num) {
                pthread_cond_broadcast(&serverQ);
            }
            printf("Emulation ends");
            break;
        }
       
    }
    pthread_exit(0);
}

void *server2Method(void *args)
{
    long long currentTime = 0,elapsedTime;
    
    while (1) {
        
        pthread_mutex_lock(&Q1Mutex);
        
        while (My402ListEmpty(&Q2) && packetsServed != num && !serveInterrupt) {
            pthread_cond_wait(&serverQ, &Q1Mutex);
        }
        
        if (packetsServed == num || serveInterrupt) {
            pthread_mutex_unlock(&Q1Mutex);
            break;
        }

        packet *dequePacket = (packet*)My402ListFirst(&Q2)->obj;
        struct timeval temp;
        gettimeofday(&temp, NULL);
        
        My402ListUnlink(&Q2,My402ListFirst(&Q2));
        
        printf("\n p%d leaves Q2, time in Q2 = %ld",dequePacket->ID,(temp.tv_sec + temp.tv_usec*1000000L) - (dequePacket->Q2EnterTime.tv_sec + dequePacket->Q2EnterTime.tv_usec*1000000L));
        
        packetsServed++;
        
        pthread_mutex_unlock(&Q1Mutex);
        
        gettimeofday(&dequePacket->serviceStartTime,NULL);
        elapsedTime = dequePacket->serviceStartTime.tv_sec + dequePacket->serviceStartTime.tv_usec*1000000L - currentTime;
        
        printf("\n p%d begins service at S2, requesting %dms of service",dequePacket->ID,100);
        
        usleep((unsigned int)fabs((1/mu)*1000000L - elapsedTime/1000000L));
        
        gettimeofday(&dequePacket->serviceEndTime,NULL);
        currentTime = dequePacket->serviceEndTime.tv_sec + dequePacket->serviceEndTime.tv_usec*1000000L;
        
           printf("\n p% departs from S2, service time = %lldms , time in system = %ldms",dequePacket->ID,(currentTime - (dequePacket->serviceStartTime.tv_sec + dequePacket->serviceStartTime.tv_usec*1000)),(dequePacket->serviceStartTime.tv_sec + dequePacket->serviceStartTime.tv_usec*1000000L) - (dequePacket->serviceEndTime.tv_sec + dequePacket->serviceEndTime.tv_usec*1000000L));

        if (packetsServed == num || serveInterrupt) {
            if (packetsServed == num) {
                pthread_cond_broadcast(&serverQ);
            }
            printf("Emulation ends");
            break;
        }
        
    }
    return(0);
}

void *handleQuitGracefully(void *args)
{
    while (1) {
        pthread_sigmask(SIG_UNBLOCK, &quitSignal, NULL);
        action.sa_handler = handleQuit;
        sigaction(SIGINT, &action, NULL);
        usleep(100000000);
    }
}

void handleQuit(int signal)
{
    pthread_cancel(tokenThread);
    pthread_cancel(packetThread);
    
    // add a bool to condition and broadcast condition
    serveInterrupt = 1;
    pthread_cond_broadcast(&serverQ);
    
    pthread_mutex_lock(&Q1Mutex);
    
    // traverse Q1 and Q2 to prompt delete
    My402ListUnlinkAll(&Q1);
    My402ListUnlinkAll(&Q2);
    
    pthread_mutex_unlock(&Q1Mutex);
    pthread_exit(0);
}

