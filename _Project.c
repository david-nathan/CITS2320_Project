
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  

#include "Project.h"  

#define EOL    '\n'
#define CARRIAGE_RETURN    '\r'



extern char *strdup(const char *);
extern void enqueueJOBQ(JOB element, JOBQ *q);
extern JOBQ *newJOBQ(int size);

const char * FCFS = "FCFS";
const char * roundRobin = "RR";


/*---GLOBALS---*/

JOB jobList[MAXJOBS];
JOBQ *todoJobs;


/*---HELPER FUNCTIONS---*/
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

void trimLine(char line[]){
    int i = 0;
    while(line[i] != '\0') {
        if(line[i] == CARRIAGE_RETURN || line[i]== EOL){
        line[i] = '\0';
        }
        i++;
    }
}
/**
 * Reads in a single line from a Job file and processes it.
 * Checks if it is a special line and carries out the functionality accordingly.
 *
 */
void processSingleLine(char* line, int jobID){
    JOB *job = &(jobList[jobID]);
    
    //Checks if line is special 
    if(strncmp(line, "if", 2) == 0){
        char tok_str[9][BUFSIZ];
        char *token;
        char *var;
        int linenum;
        int compare;
        
        int n = 0;
        
        //Isolates tokens in the given line that are separated by whitespace or tab spaces
        if((token = strtok(line, " \t")) != NULL){
            do{
                strcpy(tok_str[n], token);
                n++;
            }while((token=strtok(NULL, " \t")) != NULL);
        }
        
        trimLine(tok_str[1]); //remove null-byte
        var = tok_str[1];
        linenum = atoi(tok_str[8]);
        compare = atoi(tok_str[3]);
        
        //Check if variable already exists and if not create a new one
        int var_index = 0;
        while (var_index < job->num_vars && job->vars[var_index] != var) {
            var_index++;
        }
        
        if (var_index == job->num_vars) {
            //variable does not exist, create a new variable with value 0
            job->vars[job->num_vars] = var;
            job->var_values[job->num_vars] = 0;
            //check if max number of variables has been reached
            if (job->num_vars == MAXVARS) {
                DieWithUserMessage("Max Variables", "The maximum allow number of variables for a job has been exceeded");
            }
            job->num_vars = job->num_vars + 1;
        }
        
        //process line
        if (job->var_values[var_index] < compare) {
            job->var_values[var_index] = job->var_values[var_index] + 1;
            job->currentline = linenum;
        } else {
            job->currentline = job->currentline + 1;
        }
        
    } else {
        job->currentline = job->currentline + 1;
    }
}

void loadJobFiles(char* file) {
    
    FILE *fp = fopen(file, "r");
    if(fp == NULL){
        DieWithSystemMessage("Unable to open Job-List file");
    }
    
    int jobID = 0;
    char jobFile[BUFSIZ];
    
    while (fgets(jobFile, sizeof(jobFile), fp) != NULL) {
        
        trimLine(jobFile); 
        FILE *fpJob = fopen(jobFile, "r");
       
        if(fpJob == NULL){
            DieWithSystemMessage("Unable to open Job file");
        }
        
        char buffer[BUFSIZ];
        int linecount = 0;
        
        JOB newJob;
        newJob.currentline = 1;
        newJob.num_vars = 0;
        newJob.jobID = jobID;
        
        newJob.filename = strdup(jobFile);
        
        while (fgets(buffer, sizeof(buffer), fpJob) != NULL) {
             trimLine(buffer);
           
            if(linecount == 0){
                newJob.start = atoi(buffer);
                linecount++;
            }
            
            //TODO: Page table stuff
            
            linecount++;
        }
        
        newJob.length = linecount - 1;
        
        //Add to Job to queue of Jobs
        enqueueJOBQ(newJob, todoJobs);
       
        printf("NEWJOB Q'd: %s\n", newJob.filename);
       
        jobID++;
        
       fclose(fpJob);
      
        
    }
    
}

void simulate(char* file, char* sched, int timeQuant){
    
    loadJobFiles(file);
}


int main(int argc, char *argv[]){

    todoJobs = newJOBQ(MAXJOBS);
    
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
    } else { 
        if(strcmp(sched, FCFS) == 0){
        timeQuant = 1;
    }
    }
 
    simulate(file, sched, timeQuant);
        
    
    printf("Number of JOBs: %d\n", todoJobs->nElements);  
        

    
   
   
}
