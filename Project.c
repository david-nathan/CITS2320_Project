
#include <stdio.h>
#include <stdlib.h>
#include <string.h>    

#define EOL    '\n'
#define CARRIAGE_RETURN    '\r'


const char * FCFS = "FCFS";
const char * roundRobin = "RR";


void DieWithUserMessage(const char *msg, const char *detail) {
    fputs(msg, stderr);
    fputs(": ",stderr);
    fputs(detail, stderr);
    fputc('\n', stderr);
    exit(1);
}

void DieWithSystemMessage(const char *msg) {
    perror(msg);
    exit(1);
}

struct JOB {
	
	int start;
	char* filename;
	
	};

int countLines(char* str_filename){
     FILE *fd = fopen(str_filename, "r");
     
     if(fd == NULL){ DieWithSystemMessage("Unable to open file"); } //Test that file was opened
              
     unsigned int line_count = 0;
     char ch;
      
     while ( (ch = fgetc(fd)) != EOF) //loop through each character in file. 
         {
      if (ch == EOL || ch == CARRIAGE_RETURN){ ++line_count;} //increase line_count if end-of-line or carriage-return is encountered
          }
         
     if (fd) { fclose(fd);}
     
     return line_count;
      } 

int main(int argc, char *argv[]){
    
    if(argc < 3 || argc > 4){//Test for correct number of arguments
    DieWithUserMessage("Parameter(s)", "<Schedule Type> <File>");
    }
    
    char* sched = argv[1]; //Type of schedule 
    char* file = argv[2]; //Name of file that contains jobs
    int timeQuant;        //Time quantum for RR scheduling
    int numJobs;          //Number of Jobs
    
    if(strcmp(sched, roundRobin) == 0){ 
       if(argc == 3){//Test for correct number of arguments for RR
        DieWithUserMessage("Parameter(s)", "<Time Quantum>");
        }       
        timeQuant = atoi(argv[3]); //Set time quantum
    } 
    
   numJobs = countLines(file);
   
   printf("Number of Jobs: %d\n", numJobs);
   
   char* jobfile = malloc(BUFSIZ);
   char jobfiles[numJobs][BUFSIZ];
   struct JOB jobs[numJobs];
   
   FILE *fp= fopen(file, "r");
   if(fp == NULL){ DieWithSystemMessage("Unable to open file"); } //Test that file was opened
   
   int n=0;
   while(fgets(jobfile, BUFSIZ, fp) != NULL)
   { 	
   	 jobfile[strlen(jobfile)-1] ='\0';
   	 strcpy(jobfiles[n],jobfile);
   	 n++;
   }   
   
   for(int i=0; i< numJobs; i++){
   	int length = countLines(jobfiles[i]);
   	char *buf = malloc(BUFSIZ);
   	FILE *file = fopen(jobfiles[i], "r");
   	fgets(buf, BUFSIZ, file);
   	int start = atoi(buf);
   	jobs[i].filename = jobfiles[i];
   	jobs[i].start = start;
   	
	printf("File: %s, start: %d length: %d\n",jobs[i].filename, jobs[i].start, length);
   }
   
   
   
   
   
   
   
   
}
