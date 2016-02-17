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
FILE *fp;
sigset_t quitSignal;
struct sigaction action;

My402List Q1,Q2,tokenBucket;
pthread_mutex_t Q1Mutex;
pthread_cond_t serverQ;
unsigned int packetCount = 0;
unsigned int packetsServed = 0;
unsigned int serveInterrupt = 0;
unsigned int isTraceFileMode = 0;
unsigned int allThreadsKilled = 0;

double lambda = 1,mu = 0.35,r = 1.5, b = 10 ,p = 3;
long int num = 20;
char fileName[128];
pthread_t packetThread,tokenThread,serverThread1,serverThread2,signalQuitThread;
double mainTimeLine = 0;
double timeOffset = 0;

typedef struct packet
{
    int ID;
    int tokensNeeded;
    double interArrivalTime;
    double serviceTime;
    struct timeval Q1EnterTime,Q2EnterTime,systemTimeOnEnter,systemTimeOnExit,serviceStartTime,serviceEndTime;
    
}packet;

void *packetArrivalMethod(void *args);
void *tokenArrivalMethod(void *args);
void *serverMethod(void *args);
void *server2Method(void *args);
void *handleQuitGracefully(void *args);
void handleQuit(int signal);


int main(int argc, const char * argv[]) {
    
    printf("\n\nEmulation Parameters:\n");
    
    int i;
    for (i = 1; i < argc; i++) {
        
        if (strcmp(argv[i],"-lambda") == 0) {
            
            if ((i+1 != argc) && (1 == sscanf(argv[i+1],"%lf",&lambda))) {
                if (lambda < 0) {
                    fprintf(stderr,"Malformed Command , lambda value must be greater than 0");
                    exit(0);
                }
                i+=1;
            }else {
                fprintf(stderr,"Malformed Command , lambda value not provided");
                exit(0);
            }
        }else if (strcmp(argv[i],"-mu") == 0) {
            
            if ((i+1 != argc) && (1 == sscanf(argv[i+1],"%lf",&mu))) {
                if (mu < 0) {
                    fprintf(stderr,"Malformed Command , Mu value must be greater than 0");
                    exit(0);
                }
                i+=1;
            }else {
                fprintf(stderr,"Malformed Command , Mu value not provided");
                exit(0);
            }
        }else if (strcmp(argv[i],"-r") == 0) {
            
            if ((i+1 != argc) && (1 == sscanf(argv[i+1],"%lf",&r))) {
                if (r < 0) {
                    fprintf(stderr,"Malformed Command , Rate of tokens must be greater than 0");
                    exit(0);
                }
                i+=1;
            }else {
                fprintf(stderr,"Malformed Command , Rate of tokens value not provided");
                exit(0);
            }
        }else if (strcmp(argv[i],"-B") == 0) {
            
            if ((i+1 != argc) && (1 == sscanf(argv[i+1],"%lf",&b))) {
                if (b < 0) {
                    fprintf(stderr,"Malformed Command , Bucket size must be greater than 0");
                    exit(0);
                }
                i+=1;
            }else {
                fprintf(stderr,"Malformed Command , Bucket size value not provided");
                exit(0);
            }
        }else if (strcmp(argv[i],"-P") == 0) {
            
            if ((i+1 != argc) && (1 == sscanf(argv[i+1],"%lf",&p))) {
                if (p < 0) {
                    fprintf(stderr,"Malformed Command , token per packet must be greater than 0");
                    exit(0);
                }
                i+=1;
            }else {
                fprintf(stderr,"Malformed Command , tokens per packet value not provided");
                exit(0);
            }
        }else if (strcmp(argv[i],"-n") == 0) {
            
            if ((i+1 != argc) && (1 == sscanf(argv[i+1],"%ld",&num))) {
                if (num < 0) {
                    fprintf(stderr,"Malformed Command , Packet count must be greater than 0");
                    exit(0);
                }
                i+=1;
            }else {
                fprintf(stderr,"Malformed Command , Packet count value not provided");
                exit(0);
            }
        }else if (strcmp(argv[i],"-t") == 0) {
        
            if ((i+1 != argc) && (1 == sscanf(argv[i+1],"%s",fileName))) {
                
                isTraceFileMode = 1;
                
                struct stat fileCheck;
                
               // char *fileName = "/Users/gauravnijhara/Projects/CS402Warmup2/CS402Warmup2/files/f0.txt";
                
                if( stat(fileName,&fileCheck) == 0 )
                {
                    if( fileCheck.st_mode & S_IFREG )
                    {
                        if(access(fileName,R_OK) != 0)
                        {
                            fprintf(stderr, " Input file %s cannot be opened : access denied \n",fileName);
                            exit(0);
                        }
                        else
                        {
                            fp = fopen(fileName, "r");
                            if (!fp) {
                                fprintf(stderr, "Malformed command or file: %s doesn't exist \n",fileName);
                                exit(1);
                            }
                            
                            
                            char buffer[1026];
                            if(fgets(buffer, sizeof(buffer)/sizeof(char), fp) != NULL) {
                                
                                num = atol(buffer);
                            }
                            // file opened here
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
                
                i+=1;
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
    
    printf("number to arrive = %ld\n",num);
    if (!isTraceFileMode) {
        printf("\t\tlambda = %f\n",lambda);
        printf("\t\tmu = %f\n",mu);
    }
    printf("\t\tr = %f\n",r);
    printf("\t\tB = %f\n",b);
    if (!isTraceFileMode) {
        printf("\t\tP = %f\n",p);
    }else {
        printf("\t\ttsfile = %s\n",fileName);
    }

    struct timeval temp;
    gettimeofday(&temp, NULL);
    mainTimeLine = 0 ;
    timeOffset = (-1)*(temp.tv_sec*1000000 + temp.tv_usec);
    
    printf("%012.3lfms : Emulation Begins\n\n",000000000.000);
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
    allThreadsKilled = 1;
    pthread_join(signalQuitThread,NULL);
    
    printf("%012.3lfms : Emulation ends\n\n\n",mainTimeLine);
    pthread_exit(0);
}

void *packetArrivalMethod(void *args)
{
    long long prevTokenArrivalTime=0,currentTime,elapsedTime = 0;
    struct timeval time;
    
    gettimeofday(&time,NULL);
    currentTime = time.tv_sec*1000000 + time.tv_usec;
    prevTokenArrivalTime = currentTime;

    // create first packet
    while (packetCount < num) {
        
        packet *newPacket = (packet*)malloc(sizeof(packet));
        
        if (isTraceFileMode) {
            char buffer[1026];
            if(fgets(buffer, sizeof(buffer)/sizeof(char), fp) != NULL) {
                
                // parsing code
                const char s[2] = " ";
                char *token;
                
                // inter-arrival time
                token = strtok(buffer, s);
                double interArrivalTime = atof(token)*1000;
                newPacket->interArrivalTime = interArrivalTime;
                
                // tokens needed
                token = strtok(NULL, s);
                int tokensNeeded = atoi(token);
                newPacket->tokensNeeded = tokensNeeded;
                
                // service time
                token = strtok(NULL, s);
                double serviceTime = atof(token)*1000;
                newPacket->serviceTime = serviceTime;
                
            }
        }else {
            newPacket->interArrivalTime = (1/lambda)*1000000;
            newPacket->tokensNeeded = p;
            newPacket->serviceTime = (1/mu)*1000000;
            
        }
        newPacket->ID = ++packetCount;

        gettimeofday(&time,NULL);
        elapsedTime = time.tv_sec*1000000 + time.tv_usec - currentTime;

        long int sleeptime = ((newPacket->interArrivalTime) - elapsedTime);
        
      //  printf("\n sleeptime %ld \n",sleeptime);
        
     //   printf("\n------%d------\n",(unsigned int)sleeptime);
        usleep((sleeptime<0)?0:(unsigned int)sleeptime);
        
        gettimeofday(&time,NULL);
        currentTime = time.tv_sec*1000000 + time.tv_usec;
        mainTimeLine =  (time.tv_sec*1000000 + time.tv_usec + timeOffset)/1000;
        
        
        // packetCount
        printf("%012.3lfms : packet%d arrives , need %d tokens, inter-arrival time = %08.3lfms \n",mainTimeLine,newPacket->ID,newPacket->tokensNeeded,((double)(currentTime-prevTokenArrivalTime))/1000);
        
        prevTokenArrivalTime = currentTime;

        newPacket->systemTimeOnEnter = time;
        
        pthread_mutex_lock(&Q1Mutex);
        {
            
            if (newPacket->tokensNeeded > b) {
                packetsServed++;
                gettimeofday(&time,NULL);
                elapsedTime = time.tv_sec*1000000 + time.tv_usec - currentTime;
                pthread_mutex_unlock(&Q1Mutex); 
                continue;
            }
            
            
            gettimeofday(&newPacket->Q1EnterTime, NULL);
            mainTimeLine = (newPacket->Q1EnterTime.tv_sec*1000000 + newPacket->Q1EnterTime.tv_usec + timeOffset)/1000;
            printf("%012.3lfms : packet%d enters Q1\n",mainTimeLine,newPacket->ID);
            
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
            My402ListAppend(&Q1,(void*)newPacket);
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
            

            
            if (newPacket->tokensNeeded <= tokenBucket.num_members) {
                
                
                int i = 0;
                while (i < newPacket->tokensNeeded) {
                    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
                    My402ListUnlink(&tokenBucket,My402ListFirst(&tokenBucket));
                    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
                    i++;
                }
                
                
                struct timeval temp;
                gettimeofday(&temp, NULL);
                mainTimeLine = (temp.tv_sec*1000000 + temp.tv_usec + timeOffset)/1000;
                
                pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
                My402ListUnlink(&Q1, My402ListFirst(&Q1));
                pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);

                printf("%012.3lfms : p%d leaves Q1, time in Q1 = %08.3lfms, token bucket now has %d token\n",mainTimeLine,newPacket->ID,((double)((temp.tv_sec*1000000 + temp.tv_usec) - (newPacket->Q1EnterTime.tv_sec*1000000 + newPacket->Q1EnterTime.tv_usec)))/1000,Q1.num_members);

                
                pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
                My402ListAppend(&Q2,(packet*)newPacket);
                pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
                
                gettimeofday(&newPacket->Q2EnterTime, NULL);
                mainTimeLine = (newPacket->Q2EnterTime.tv_sec*1000000 + newPacket->Q2EnterTime.tv_usec + timeOffset)/1000;
                printf("%012.3lfms : packet%d enters Q2\n",mainTimeLine,newPacket->ID);



            }
        }
        
        if (!My402ListEmpty(&Q2)) {
            pthread_cond_broadcast(&serverQ);
        }

        pthread_mutex_unlock(&Q1Mutex);
        
    }

    //printf("\n packet thread exit \n");
    pthread_exit(0);
    
}


void *tokenArrivalMethod(void *args)
{
    long long currentTime,elapsedTime=0;
    struct timeval time;

    while (packetsServed < num || !serveInterrupt) {
        
        long int sleeptime = ((1/r)*1000000 - elapsedTime);

      //  printf("\n------%d token------\n",(unsigned int)sleeptime);
        
        usleep(((sleeptime<0)?0:(unsigned int)sleeptime));

        gettimeofday(&time,NULL);
        currentTime = time.tv_sec*1000000 + time.tv_usec;

        pthread_mutex_lock(&Q1Mutex); 
        {

            if (tokenBucket.num_members <= b) {
                int *token = (int*)malloc(sizeof(int));
                pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
                My402ListAppend(&tokenBucket,token);
                pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
            }
            
            if (!My402ListEmpty(&Q1)) {
               
            
            packet *dequePacket = (packet*)My402ListFirst(&Q1)->obj;

            if (dequePacket->tokensNeeded <= tokenBucket.num_members) {
                
                
                    int i = 0;
                    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
                    while (i < dequePacket->tokensNeeded) {
                        My402ListUnlink(&tokenBucket,My402ListFirst(&tokenBucket));
                        i++;
                    }
                    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);

                    struct timeval temp;
                    gettimeofday(&temp, NULL);
                    mainTimeLine = (temp.tv_sec*1000000 + temp.tv_usec + timeOffset)/1000;
                
                    printf("%012.3lfms : p%d leaves Q1, time in Q1 = %08.3lfms, token bucket now has %d token\n",mainTimeLine,dequePacket->ID,((double)((temp.tv_sec*1000000 + temp.tv_usec) - (dequePacket->Q1EnterTime.tv_sec*1000000 + dequePacket->Q1EnterTime.tv_usec)))/1000,Q1.num_members);
                
                    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
                    My402ListUnlink(&Q1, My402ListFirst(&Q1));
                    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);

                    gettimeofday(&temp, NULL);
                    mainTimeLine = (temp.tv_sec*1000000 + temp.tv_usec + timeOffset)/1000;

                    printf("%012.3lfms : packet%d enters Q2\n",mainTimeLine,dequePacket->ID);
                
                    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
                    My402ListAppend(&Q2,(packet*)dequePacket);
                    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);

                }
            
                if (!My402ListEmpty(&Q2)) {
                    pthread_cond_broadcast(&serverQ);
                }
            }
            pthread_mutex_unlock(&Q1Mutex); 
            
            if (packetsServed == num) {
	        pthread_mutex_lock(&Q1Mutex); 
                pthread_cond_broadcast(&serverQ);
                pthread_mutex_unlock(&Q1Mutex); 
                break;
            }

        }
        
        gettimeofday(&time,NULL);
        elapsedTime = time.tv_sec*1000000 + time.tv_usec - currentTime;

    }

    //printf("\n token thread exit \n");
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
        
        
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
        My402ListUnlink(&Q2,My402ListFirst(&Q2));
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
        
        struct timeval temp;
        gettimeofday(&temp, NULL);
        mainTimeLine = (temp.tv_sec*1000000 + temp.tv_usec + timeOffset)/1000;

        printf("%012.3lfms : p%d leaves Q2, time in Q2 = %08.3lfms\n",mainTimeLine,dequePacket->ID,((double)((temp.tv_sec*1000000 + temp.tv_usec) - (dequePacket->Q2EnterTime.tv_sec*1000000 + dequePacket->Q2EnterTime.tv_usec)))/1000);
        
        packetsServed++;
        
        pthread_mutex_unlock(&Q1Mutex); 
        
        gettimeofday(&dequePacket->serviceStartTime,NULL);
        elapsedTime = dequePacket->serviceStartTime.tv_sec*1000000 + dequePacket->serviceStartTime.tv_usec - currentTime;

        mainTimeLine = (dequePacket->serviceStartTime.tv_sec*1000000 + dequePacket->serviceStartTime.tv_usec + timeOffset)/1000;

        printf("%012.3lfms : p%d begins service at S1, requesting %dms of service\n",mainTimeLine,dequePacket->ID,100);
     
	//printf("my sleep time is %lf",dequePacket->serviceTime);

        long int sleeptime = (dequePacket->serviceTime) - elapsedTime;

      //  printf("\n------%d server 1 ------\n",(unsigned int)sleeptime);

        usleep(((sleeptime<0)?0:(unsigned int)sleeptime));

        gettimeofday(&dequePacket->serviceEndTime,NULL);
        currentTime = dequePacket->serviceEndTime.tv_sec*1000000 + dequePacket->serviceEndTime.tv_usec;
        
        mainTimeLine = (dequePacket->serviceEndTime.tv_sec*1000000 + dequePacket->serviceEndTime.tv_usec + timeOffset)/1000;

        //printf("packets : %d , num : %ld",packetsServed,num);

        printf("%012.3lfms : p%d departs from S1, service time = %08.3lfms , time in system = %08.3lfms\n",mainTimeLine,dequePacket->ID,((double)((dequePacket->serviceEndTime.tv_sec*1000000 + dequePacket->serviceEndTime.tv_usec) - (dequePacket->serviceStartTime.tv_sec*1000000 + dequePacket->serviceStartTime.tv_usec)))/1000,((double)((dequePacket->serviceEndTime.tv_sec*1000000 + dequePacket->serviceEndTime.tv_usec) - (dequePacket->Q1EnterTime.tv_sec*1000000 + dequePacket->Q1EnterTime.tv_usec)))/1000);
        
        if (packetsServed == num || serveInterrupt) {
            if (packetsServed == num) {
                pthread_mutex_lock(&Q1Mutex); 
                pthread_cond_broadcast(&serverQ);
                pthread_mutex_unlock(&Q1Mutex); 
            }
            break;
        }
       
    }
		//printf("\n server 1 thread exit \n");
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
        
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
        My402ListUnlink(&Q2,My402ListFirst(&Q2));
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);

        struct timeval temp;
        gettimeofday(&temp, NULL);
        
        mainTimeLine = (temp.tv_sec*1000000 + temp.tv_usec + timeOffset)/1000;

        printf("%012.3lfms : p%d leaves Q2, time in Q2 = %08.3lfms\n",mainTimeLine,dequePacket->ID,((double)(temp.tv_sec*1000000 + temp.tv_usec) - (dequePacket->Q2EnterTime.tv_sec*1000000 + dequePacket->Q2EnterTime.tv_usec))/1000);
        
        packetsServed++;
        
        pthread_mutex_unlock(&Q1Mutex); 
        
        gettimeofday(&dequePacket->serviceStartTime,NULL);
        elapsedTime = dequePacket->serviceStartTime.tv_sec*1000000 + dequePacket->serviceStartTime.tv_usec - currentTime;
        
        mainTimeLine = (dequePacket->serviceStartTime.tv_sec*1000000 + dequePacket->serviceStartTime.tv_usec + timeOffset)/1000;

        printf("%012.3lfms : p%d begins service at S2, requesting %dms of service\n",mainTimeLine,dequePacket->ID,100);
        
        long int sleeptime = (dequePacket->serviceTime) - elapsedTime;
        
       // printf("\n------%d server 2 ------\n",(unsigned int)sleeptime);

        usleep(((sleeptime<0)?0:(unsigned int)sleeptime));

        
        gettimeofday(&dequePacket->serviceEndTime,NULL);
        currentTime = dequePacket->serviceEndTime.tv_sec*1000000 + dequePacket->serviceEndTime.tv_usec;
        
	//printf("packets : %d , num : %ld",packetsServed,num);

            mainTimeLine = (dequePacket->serviceStartTime.tv_sec*1000000 + dequePacket->serviceStartTime.tv_usec + timeOffset)/1000;

           printf("%012.3lfms : p%d departs from S2, service time = %08.3lfms , time in system = %08.3lfms\n",mainTimeLine,dequePacket->ID,((double)((dequePacket->serviceEndTime.tv_sec*1000000 + dequePacket->serviceEndTime.tv_usec) - (dequePacket->serviceStartTime.tv_sec*1000000 + dequePacket->serviceStartTime.tv_usec)))/1000,((double)((dequePacket->serviceEndTime.tv_sec*1000000 + dequePacket->serviceEndTime.tv_usec) - (dequePacket->Q1EnterTime.tv_sec*1000000 + dequePacket->Q1EnterTime.tv_usec)))/1000);


        if (packetsServed == num || serveInterrupt) {
            if (packetsServed == num) {
                pthread_mutex_lock(&Q1Mutex); 
                pthread_cond_broadcast(&serverQ);
                pthread_mutex_unlock(&Q1Mutex); 
            }
            break;
        }
        
    }
	//printf("\n server 2 thread exit \n");
    pthread_exit(0);
}

void *handleQuitGracefully(void *args)
{
    while (!allThreadsKilled) {
        pthread_sigmask(SIG_UNBLOCK, &quitSignal, NULL);
        action.sa_handler = handleQuit;
        sigaction(SIGINT, &action, NULL);
        usleep(2000000);
    }
 //   printf("\n control c thread exit \n");
    pthread_exit(0);
}

void handleQuit(int signal)
{
    pthread_cancel(tokenThread);
    pthread_cancel(packetThread);
    
    // add a bool to condition and broadcast condition
    serveInterrupt = 1;
    pthread_cond_broadcast(&serverQ);
    
    pthread_mutex_lock(&Q1Mutex); 
    
    printf("errors here");
    // traverse Q1 and Q2 to prompt delete
    My402ListUnlinkAll(&Q1);
    My402ListUnlinkAll(&Q2);
    
    pthread_mutex_unlock(&Q1Mutex); 
    pthread_exit(0);
}

