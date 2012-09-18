
#include <stdio.h>
#include <stdlib.h>
#include <string.h>    

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

int main(int argc, char *argv[]){
    
    if(argc < 3 || argc > 4){//Test for correct number of arguments
    DieWithUserMessage("Parameter(s)", "<Schedule Type> <File>");
    }
    
    char* sched = argv[1];
    
    if(strcmp(sched, roundRobin) == 0 && argc == 3){
        DieWithUserMessage("Parameter(s)", "<Time Quantum>");
    }
}
