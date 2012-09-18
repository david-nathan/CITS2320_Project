
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

int countLines(char* str_filename){
     FILE *fd;
     if((fd = fopen(str_filename, "r")) = NULL){
              DieWithSystemMessage("Unable to open file");
     }
     
      unsigned int line_count = 0;
  while ( (ch = fgetc(fd)) != EOF){
     if (ch == EOL || ch == CARRIAGE_RETURN) ++line_count;
         }
 
  if (fd) {
     fclose(fd);
  }
 
  return line_count;

      } 

int main(int argc, char *argv[]){
    
    if(argc < 3 || argc > 4){//Test for correct number of arguments
    DieWithUserMessage("Parameter(s)", "<Schedule Type> <File>");
    }
    
    char* sched = argv[1]; //Type of schedule 
    char* file = argv[2]; //Name of file that contains jobs
    int timeQuant;        //Time quantum for RR scheduling
    
    if(strcmp(sched, roundRobin) == 0){ 
       if(argc == 3){//Test for correct number of arguments for RR
        DieWithUserMessage("Parameter(s)", "<Time Quantum>");
        }       
        timeQuant = atoi(argv[3]); //Set time quantum
    } 
    
    FILE *infile = fopen(sched, "r");
    if(infile == NULL){ //Test that file was opened
    DieWithSystemMessage("Unable to open jobs file");
    }
    
    
    
    
    
    

       

    
    


}
