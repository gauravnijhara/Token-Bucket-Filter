//
//  my402listIO.c
//  CS402WarmUp1
//
//  Created by Gaurav Nijhara on 1/26/16.
//  Copyright © 2016 Gaurav Nijhara. All rights reserved.
//

#include "my402listIO.c"

FILE* setInput(char *fileName)
{

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
    return fp;
}



void parseInputToList(FILE* fp,My402List *list)
{
    char buffer[1026];
   // FILE *fpNew = fopen("f1", "r");
    while(fgets(buffer, sizeof(buffer)/sizeof(char), fp) != NULL) {
        
        if (strlen(buffer) > 1024) {
            fprintf(stderr, "Line longer than 1024 characters \n");
            exit(1);
        }
        
        transaction *newObject = createTransaction();
        if (newObject) {
            // parsing code
            const char s[2] = "\t";
            char *token;
            
            // type of transaction
            token = strtok(buffer, s);
	    if((strcmp(token,"+") != 0) && (strcmp(token,"-") != 0))
	    {
		fprintf(stderr, "Malformed file \n");
                exit(1);
	    }	
            strcpy(newObject->type,token);
            
            // timestamp
            token = strtok(NULL, s);
            if (strlen(token) >= 11 || atol(token) > time(0)) {
                fprintf(stderr, "Bad timestamp \n");
                exit(1);
            }
            
            time_t raw_time = atol(token);
            newObject->timeStamp = (unsigned int)atol(token);
            struct tm*  timeinfo = localtime (&raw_time);
            strftime (newObject->timeStampDesc,16,"%a %b %e %Y",timeinfo);
            
            // amount
            token = strtok(NULL, s);
	    long length = atol(token);

            if (length <1 || length >= 10000000) {
                fprintf(stderr, "Length of amount greater than desired range or less than 1 \n");
                exit(1);
            }
	    
	    char *dec = strchr(token,'.');
	    int decPosition = (int)(dec -token);
	    if ((strlen(token) - decPosition) > 3) {
                fprintf(stderr, "Decimal places more than 2 \n");
                exit(1);
            }

            strcpy(newObject->amount,token);
	   
            
            // transaction description
            token = strtok(NULL, s);
	    char *exceedField = strtok(NULL, s);
	    if (exceedField && strlen(exceedField) > 0)
	    {
		fprintf(stderr, "Too many fields \n");
                exit(1);
 	    }

	    token = strtok(token,"\n");
            if (strlen(token) < 1) {
                fprintf(stderr, "Description not empty \n");
                exit(1);
            }
            if (strlen(token) > 24) {
                strncpy(newObject->desc,token,24);
            }else {
                strncpy(newObject->desc,token,strlen(token));
            }
            newObject->desc[strlen(token)] = '\0';


            My402ListAppend(list, newObject);
            
	
        }
    }
}



void printList(My402List *list)
{
    My402ListElem *elem=NULL;
    long prevBalance = 0;
    int isWithdrawal = 0;
    
    printf("+-----------------+--------------------------+----------------+----------------+\n");
    printf("|       Date      | Description              |         Amount |        Balance |\n");
    printf("+-----------------+--------------------------+----------------+----------------+\n");
    for (elem=My402ListFirst(list); elem != NULL; elem=My402ListNext(list, elem)) {
        
        long amount,balance;
        isWithdrawal = 1;
        if (strcmp((((transaction*)elem->obj)->type), "+") != 0) {
            isWithdrawal = -1;
        }
        
        long number,decimals;
        int adec1,adec2,bdec1,bdec2;
        number = atol(strtok((((transaction*)elem->obj)->amount), "."));
        decimals = atol(strtok(NULL,"."));
        amount = number*100 + decimals;
        balance = prevBalance + (isWithdrawal)*amount;
        prevBalance = balance;
//        sscanf((((transaction*)elem->obj)->amount),"%lul",&amount);
//
        adec1 = amount%10;
        amount = amount/10;
        adec2 = amount%10;
        amount = amount/10;

        bdec1 = balance%10;
        balance = balance/10;
        bdec2 = balance%10;
        balance = balance/10;

        
        setlocale(LC_ALL, "en_US");
        //check negative for balance
        if (isWithdrawal == -1) {
            if ((int)balance >= 10000000) {
                printf("| %15s | %-24s | (%9d.%d%d) | (?,???,???.??) |\n",(((transaction*)elem->obj)->timeStampDesc),(((transaction*)elem->obj)->desc),(int)amount,adec2,adec1);
            } else {
                if (balance < 0) {
                    printf("| %15s | %-24s | (%'9d.%d%d) | (%'9lu.%d%d) |\n",(((transaction*)elem->obj)->timeStampDesc),(((transaction*)elem->obj)->desc),(int)amount,adec2,adec1,(-1)*balance,(-1)*bdec2,(-1)*bdec1);
                }else {
                    printf("| %15s | %-24s | (%'9d.%d%d) | %'10lu.%d%d  |\n",(((transaction*)elem->obj)->timeStampDesc),(((transaction*)elem->obj)->desc),(int)amount,adec2,adec1,balance,bdec2,bdec1);
                }
            }
        } else {
            if ((int)balance >= 10000000) {
                printf("| %15s | %-24s | %'10d.%d%d  | (?,???,???.??) |\n",(((transaction*)elem->obj)->timeStampDesc),(((transaction*)elem->obj)->desc),(int)amount,adec2,adec1);
            } else {
                if (balance < 0) {
                    printf("| %15s | %-24s | %'10d.%d%d  | (%'9lu.%d%d) |\n",(((transaction*)elem->obj)->timeStampDesc),(((transaction*)elem->obj)->desc),(int)amount,adec2,adec1,(-1)*balance,(-1)*bdec2,(-1)*bdec1);
                } else {
                    printf("| %15s | %-24s | %'10d.%d%d  | %'10lu.%d%d  |\n",(((transaction*)elem->obj)->timeStampDesc),(((transaction*)elem->obj)->desc),(int)amount,adec2,adec1,balance,bdec2,bdec1);
                }
            }
        }
    }
    printf("+-----------------+--------------------------+----------------+----------------+\n");
    
}
