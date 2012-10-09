
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
	int length;
	char* filename;
	
	};
	
	
int start_cmp(const struct JOB *j1, const struct JOB *j2){

	return (int)(j1->start - j2->start);

}	

int *FCFS_sched(const struct JOB *jobs, int num_jobs){
	
	int* times = (int*)malloc(num_jobs*sizeof(int));
	int variable_count = 0;
	int variable_capacity = 5;
	char *variables = malloc(variable_capacity*sizeof(char));
	
	
		
	for(int i=0; i<num_jobs; i++){	
		char *line = malloc(BUFSIZ);
		char joblines[jobs[i].length][BUFSIZ];
				
		FILE *fp = fopen(jobs[i].filename, "r");
		
		int line_num=0;
		while(fgets(line, BUFSIZ, fp) != NULL){
			strcpy(joblines[line_num],line);
			line_num++;		
		}
				
		int count =0;
		for(int j =1; j< jobs[i].length; j++){
			if(strncmp(joblines[j],"if", 2) == 0){			
			 char tok_str[9][BUFSIZ];
			 int n=0;
			 char *token;
			 
			 if((token=strtok(joblines[j]," ")) != NULL){
			 	do{
			 	strcpy(tok_str[n], token);
			 	n++;
			 	}while((token=strtok(NULL," ")) != NULL);			 
			 }
			 for(int i=0; i<9; i++){
			 printf("Token[%d]: %s\n", i, tok_str[i]);
			 }						
			}
			
			count++;			
		}
		
		times[i] = count;
		free(line);			
		fclose(fp);
		}
	int *p = &(times[0]);
	
		
	return p;
}
	
	

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
   	jobs[i].length = length;
   	jobs[i].start = start;
   	free(buf);
   }   
      
   qsort(jobs, numJobs, sizeof(struct JOB), (int(*)(const void*,const void*))start_cmp); //Orders the Array according to start time
   
   int *jobtimes = FCFS_sched(jobs, numJobs);
      
      for(int i =0; i<numJobs; i++){
   printf("File: %s, start: %d time: %d\n",jobs[i].filename, jobs[i].start, jobtimes[i]);   
         }
   
   free(jobfile);
   
   
   
   
   
}
