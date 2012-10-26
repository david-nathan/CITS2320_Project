
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h> 
#include <assert.h> 

#include "Project.h"

#define EOL    '\n'
#define CARRIAGE_RETURN    '\r'



extern char *strdup(const char *);
extern void enqueueJOBQ(JOB element, JOBQ *q);
extern JOBQ *newJOBQ(int size);
extern bool isEmptyJOBQ(JOBQ *q);
extern JOB peekJOBQ(JOBQ *q);
extern JOB dequeueJOBQ(JOBQ *q);
extern void sortJOBQ(JOBQ *q);

const char * FCFS = "FCFS";
const char * roundRobin = "RR";


/*---GLOBALS---*/

/* Array of Jobs that are in the specified file that are indexed by jobID*/
JOB jobList[MAXJOBS];

/* Queue of t */
JOBQ *todoJobs;
JOBQ *readyJobs;

PAGETABLE pagetables[MAXJOBS];

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




MEMORY initialiseMemory(int cost, int num_frames) {
	// initialise parameters and allocate space for arrays
	MEMORY m;
	m.num_frames = num_frames;
	m.accessCost = cost;
	m.frames = calloc(num_frames, sizeof(PAGE));
	m.LRU = calloc(num_frames, sizeof(int));
	
	// initialise LRU
	for(int i=0; i<num_frames; i++) {
		m.LRU[i] = i;
	}
	
	return m;
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
        char var;
        int linenum;
        int compare;
        
        int n = 0;
        
        //Isolates tokens in the given line that are separated by whitespace or tab spaces
        if((token = strtok(line, " ")) != NULL){
            do{
                strcpy(tok_str[n], token);
                n++;
            }while((token=strtok(NULL, " ")) != NULL);
        }
        
        
        var = tok_str[1][0];
        linenum = atoi(tok_str[8]);
        compare = atoi(tok_str[3]);
        
        
        
        //printf("PARSED LINE READS: if %c<%d %c=%c+1 goto %d\n", var, compare, var, var, linenum);
        
        
        
        
        //Check if variable already exists and if not create a new one
        int var_index = 0;
        
        //printf("REACHED: vars[%d] = %c = %d\n", var_index, job->vars[var_index], job->var_values[var_index]);
        
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
            }char* result = malloc(BUFSIZ);
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

void processLineFromCache(int cacheFrame, MEMORY cache, int jid) {
	// determine if the line to be processed is an even or odd line
	int line = (jobList[jid].currentline + 1) % 2;
	char *data = cache.frames[cacheFrame].data[line];
	processSingleLine(data,jid);
}


void loadJobFiles(char* file, MEMORY harddrive) {
    
    FILE *fp = fopen(file, "r");
    if(fp == NULL){
        DieWithSystemMessage("Unable to open Job-List file");
    }
    
    int jobID = 0;
    int hdFrameCount = 0;
    char jobFile[BUFSIZ];
    
    while (fgets(jobFile, sizeof(jobFile), fp) != NULL) {
        
        trimLine(jobFile); 
        FILE *fpJob = fopen(jobFile, "r");
       
        if(fpJob == NULL){
            DieWithSystemMessage("Unable to open Job file");
        }
        
        char buffer[BUFSIZ];
        int linecount = 0;
        int pagecount = 0;
        
        JOB newJob;
        newJob.currentline = 1;
        newJob.num_vars = 0;
        newJob.jobID = jobID;
        newJob.filename = strdup(jobFile);
        
        //go through a job
        while (fgets(buffer, sizeof(buffer), fpJob) != NULL) {
             trimLine(buffer);
           
            if(linecount == 0){
                newJob.start = atoi(buffer);
                linecount++;
            }
            
            
            if(linecount %2 != 0){
                //Create new page and add to harddrive
                PAGE page;
                page.data[0] = strdup(buffer);
                page.page_number = pagecount;
                //add page to harddrive
                harddrive.frames[hdFrameCount] = page;
                
                // update page table
                pagetables[jobID].pageIndex[linecount] = pagecount;
                pagetables[jobID].hdd_frameIndex[pagecount] = hdFrameCount;
                pagetables[jobID].RAMFrame[pagecount] = -1;
                pagetables[jobID].cacheFrame[pagecount] =  -1;
                
                pagecount++;
                hdFrameCount++;
            } else {
                //Even line number
                harddrive.frames[hdFrameCount - 1].data[1] = strdup(buffer);

                pagetables[jobID].pageIndex[linecount] = pagecount-1;
            }
            
            linecount++;
        }
        
        newJob.length = linecount - 1;
        
        //Add to Job to queue of Jobs
        enqueueJOBQ(newJob, todoJobs);
              
        jobID++;
        
       fclose(fpJob);
      
        
    }
    
    sortJOBQ(todoJobs);
    
}

void printResults(int *results, int end, char *sched){
    
  char* buffer = malloc(BUFSIZ);  
  char* result = malloc(BUFSIZ);
  char* token = malloc(BUFSIZ);
  char* placeholder = malloc(BUFSIZ);  
  char rrResults[MAXJOBS][BUFSIZ];
    
    for(int i = 0; i < end; i++){
     if(results[i] == MAXJOBS){
        snprintf(buffer,2,"%s", "X");
     } else {     
        snprintf(buffer, 2, "%d", results[i]);
     }

     strcat(result, buffer);
     
    }    
   
    strcat(token, "X");
    size_t start =0;
    size_t position=0;
    size_t length=0;
    
    while((position=strspn(result, token)) != strlen(result)){
       
       start += position;
       result += position;
       strncpy(placeholder, result, 1);
       length = strspn(result, placeholder);
              
       if(!strcmp(sched, FCFS)){
        printf("%s %d %d\n", jobList[atoi(placeholder)].filename, start, start+length-1); 
        strcat(token, placeholder);
       }
       if(!strcmp(sched, roundRobin)){
       snprintf(buffer, 4, "%d ", start);
       strcat(rrResults[atoi(placeholder)], buffer);
       strcat(placeholder, "X");
       strcpy(token, placeholder);
       }    
    }
    
    if(!strcmp(sched,roundRobin)){
       int i = 0;
       while(jobList[i].filename != NULL){
          printf("%s %s\n", jobList[i].filename, rrResults[i]);
          i++;       
       }    
    }
    
}


void simulateNoMemory(char* file, char* sched, int timeQuant){
    
    MEMORY harddrive;
    harddrive.num_frames = MAXJOBS * MAX_PAGES;
    harddrive.frames = calloc(harddrive.num_frames, sizeof(PAGE));
        
    loadJobFiles(file, harddrive);
    
    int time = 1;
    int count = 0;
    int print[MAXTIME];
    for (int i = 0; i < MAXTIME; i++) {
        print[i] = MAXJOBS;
    }
    bool rrUp = false; 
    
    while(!isEmptyJOBQ(todoJobs) || !isEmptyJOBQ(readyJobs)){
        
        while(!isEmptyJOBQ(todoJobs) && peekJOBQ(todoJobs).start == time){
            
            JOB newJob = dequeueJOBQ(todoJobs);
            jobList[newJob.jobID] = newJob;
            enqueueJOBQ(newJob, readyJobs);
            
            //printf("~~~~~~~~~~New process jid = %d came alive at time %d~~~~~~~~~~\n", newJob.jobID, time);
            
        }
         
        //IDLE if no ready jobs
        if(isEmptyJOBQ(readyJobs)) {
            time++;
            continue;
        }
        
        //Fetch Next Job
        int jid = peekJOBQ(readyJobs).jobID;
        JOB *j = &jobList[jid];
        
        //If Job is finished
        if(j->currentline == j->length){
            dequeueJOBQ(readyJobs);
            count = 0;
            continue;
        }
        
        
        //Check for RR schedule
        if(!strcmp(sched,roundRobin) && rrUp){
            JOB next = dequeueJOBQ(readyJobs);
            enqueueJOBQ(next, readyJobs);
            count = 0;
            rrUp = false;
            continue;
        }
         
       
        
        //PROCESS line from HDD
        int pagenum = pagetables[jid].pageIndex[j->currentline];
        int hdd_framenum = pagetables[jid].hdd_frameIndex[pagenum];
        PAGE current_page = harddrive.frames[hdd_framenum];
        char* line = current_page.data[(jobList[jid].currentline+1)%2]; //odd line is data[0]
        processSingleLine(line, jid);
        print[time] = jid;
        time++;
        count++;
        
        
        //Check for timequantum
        if(count == timeQuant && j->currentline != j->length){
            rrUp = true;
         }
    }
    
    printResults(print, time, sched);
    free(harddrive.frames);
    
}

void simulateWithMemory(char* file, char* sched, int timeQuant){

      MEMORY harddrive;
      harddrive = initialiseMemory(0, MAX_PAGES);
      MEMORY ram;
      ram = initialiseMemory(2,8);
      MEMORY cache;
      cache = initialiseMemory(1,2);
      
      loadJobFiles(file, harddrive);
      
      int time =1;
      int count = 0;
      bool rrUp;
      
      bool fromCache;
      
      while( !isEmptyJOBQ(todoJobs) || !isEmptyJOBQ(readyJobs) ) {
            while( !isEmptyJOBQ(todoJobs) && (peekJOBQ(todoJobs).start == time) ) {
                  JOB newJob = dequeueJOBQ(todoJobs);
                  jobList[newJob.jobID] = newJob;
                  enqueueJOBQ(newJob, readyJobs);          
            }
      
      
       //IDLE if no ready jobs
        if(isEmptyJOBQ(readyJobs)) {
            time++;
            continue;
        }
        
        //Fetch Next Job
        int jid = peekJOBQ(readyJobs).jobID;
        JOB *j = &jobList[jid];
        
        //If Job is finished
        if(j->currentline == j->length){
            dequeueJOBQ(readyJobs);
            count = 0;
            continue;
        }
        
        
        //Check for RR schedule
        if(!strcmp(sched,roundRobin) && rrUp){
            JOB next = dequeueJOBQ(readyJobs);
            enqueueJOBQ(next, readyJobs);
            count = 0;
            rrUp = false;
            continue;
        }
      
        //TODO load first TWO pages in RAM if new
        
        //PROCESS LINE FROM CACHE
        int frame;
        int pagenum = pagetables[jid].pageIndex[j->currentline];
        if(frame = pagetables[jid].cacheFrame[pagenum] != -1){
       	   processLineFromCache( frame, cache, jid);
	   // cost of processing from cache is 1 in the case of this project
	   time += cache.accessCost;

	   continue;       
        }
      
      }
      

}



int main(int argc, char *argv[]){

    todoJobs = newJOBQ(MAXJOBS); 
    readyJobs = newJOBQ(MAXJOBS);
    
    if(argc < 3 || argc > 4){//Test for correct number of arguments
    DieWithUserMessage("Parameter(s)", "<Schedule Type> <File>");
    }
    
    
    
    int timeQuant;        //Time quantum for RR scheduling
    int numJobs;          //Number of Jobs
    char* sched;          //Type of schedule 
    char* file;           //Name of file that contains jobs
    
    if(argc==3){
     sched = argv[1];         
        if(strcmp(sched, roundRobin) == 0){ 
              DieWithUserMessage("Parameter(s)", "<Time Quantum>");
           }            
        if(strcmp(sched, FCFS) == 0){
          file = argv[2]; 
          timeQuant = 1;
               } else {
             DieWithUserMessage("Parameter(s)", "<Schedule Type>");
             }      
    }
    
    if(argc==4){
      sched = argv[1];                      //Type of schedule 
      
        if(strcmp(sched, roundRobin) == 0){  
            timeQuant = atoi(argv[2]);      //Set time quantum
            file = argv[3];                 //Name of file that contains jobs           
          }else{
             DieWithUserMessage("Parameter(s)", "<Schedule Type>");
          }        
        }
    
    
    
    simulateWithMemory(file, sched, timeQuant);
           
}
