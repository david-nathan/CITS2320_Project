
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>  

#include "Project.h"  

#define EOL    '\n'
#define CARRIAGE_RETURN    '\r'



extern char *strdup(const char *);
extern void enqueueJOBQ(JOB element, JOBQ *q);
extern JOBQ *newJOBQ(int size);
extern bool isEmptyJOBQ(JOBQ *q);
extern JOB peekJOBQ(JOBQ *q);
extern JOB dequeueJOBQ(JOBQ *q);

const char * FCFS = "FCFS";
const char * roundRobin = "RR";


/*---GLOBALS---*/

/* Array of Jobs that are in the specified file that are indexed by jobID*/
JOB jobList[MAXJOBS];

/* Queue of t */
JOBQ *todoJobs;
JOBQ *readyJobs;

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
                
                //TODO: Page table
                
                pagecount++;
                hdFrameCount++;
            } else {
                //Even line number
                harddrive.frames[hdFrameCount - 1].data[1] = strdup(buffer);
                //TODO: Page table
            }
            
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

void simulateNoMemory(char* file, char* sched, int timeQuant){
    
    MEMORY harddrive;
    harddrive.num_frames = MAXJOBS * MAX_PAGES;
    harddrive.frames = calloc(harddrive.num_frames, sizeof(PAGE));
    
    
    loadJobFiles(file, harddrive);
    
    int time = 1;
    int count = 0;
    
    
    while(!isEmptyJOBQ(todoJobs) || !isEmptyJOBQ(readyJobs)){
        
        while(!isEmptyJOBQ(todoJobs) && peekJOBQ(todoJobs).start == time){
            JOB newJob = dequeueJOBQ(todoJobs);
            jobList[newJob.jobID] = newJob;
            enqueueJOBQ(newJob, readyJobs);
            printf("~~~~~~~~~~New process pid = %d came alive at time %d~~~~~~~~~~\n", newJob.jobID, time);
        }
        
        //IDLE if no ready jobs
        if(isEmptyJOBQ(readyJobs)) {
            time++;
        }
        
        //Fetch Next Job
        int jid = peekJOBQ(readyJobs).start;
        JOB *j = &jobList[jid];
        
        //If Job is finished
        if(j->currentline == j->length){
            dequeueJOBQ(readyJobs);
            count = 0;
        }
        
        
    }
    
    
}

void simulateWithMemory(char* file, char* sched, int timeQuant){
	// bool to state whether this is a RR or FCFS simulation
	bool RR;
	
	// timeQuant should be -1 if not RR
	if(timeQuant == -1) {
		RR = false;
	} else {
		RR = true;
	}
	
	MEMORY disk;
	// allocate the disk size according to the number of jobs and their max length (in lines)
	disk.size = MAX_PAGES;
	// no cost, however a page fault is caused on access, and can only transfer to ram
	disk.cost = 0;
	disk.frame = calloc(disk.size, sizeof(PAGE));
	// allocate space, although in this simulation this array will not be necessary
	disk.lastAccessed = calloc(disk.size, sizeof(int));
	
	// now that the memory objects have been created, store the job files on the disk
	loadJobFiles(file, disk);
	
	MEMORY ram;
	// always 8 frames for this project
	ram.size = 8;
	ram.cost = 2;
	ram.frame = calloc(ram.size, sizeof(PAGE));
	ram.lastAccessed = calloc(ram.size, sizeof(int));
	
	/* Note: no allocation for LRU (lastAccessed) for this MEMORY object, as it will not be used */
	MEMORY cache;
	// always 2 frames for this project
	cache.size = 2;
	cache.cost = 1;
	cache.frame = calloc(cache.size, sizeof(PAGE));
	// allocate space, although in this simulation this array will not be necessary
	cache.lastAccessed = calloc(cache.size, sizeof(int));
	
	// initialise the page table
	PAGE_TABLE table;
	table.RAMFrame = calloc(MAX_PAGES, sizeof(int));
	table.cacheFrame = calloc(MAX_PAGES, sizeof(int));
	
	// initialise a parameter to keep track of the time throughout the simulation
	int time = 0;
	// keep track of how long the current job has been executing (for Round Robin)
	int count = 0;
	// keep a variable for the current job
	JOB j;
	
	// keeping looping as long as at least one job remains (whether it has begun execution or not)
	while( !isEmpty(todoJobs) || !isEmpty(readyJobs) ) {		
		// enqueue new jobs due to start at this time (if such jobs exist)
		while( !isEmpty(todoJobs) && (peek(todoJobs).start == time) ) {
			enqueueJOBQ(todoJobs.dequeueJOBQ(),readyJobs);
		}
		// if there are no jobs to process, increment time and restart loop
		if( isEmpty(readyJobs) ) {
			time++;
			continue;
		}
	
		// PROCESS AS FCFS
		if( !RR ) {
			j = peekJOBQ(readyJobs);
			int line = j.currentLine;
			
			// check if the line is already in cache
			if( cacheFrame[ ********* ] != -1) {
				processLineFromCache(cacheFrame[ ******** ] ,j);
				// cost of processing from cache is 1
				time++;
			}
			
			// check if in RAM
			if( RAMFrame[ ********* ] != -1) {
				processLineFromRAM(RAMFrame [ ********* ] , j);
				// cost of filling cache and processing line is 2
				time += 2;
			}
			
			// if not in cache or RAM, page fault, need to load a page from disk
			loadPageToRAM( ********* , RAM, j);
			
			
		}		else		{
		// ELSE PROCESS AS RR
			
		}
	
	
		// time increment should only occur here when initially running with no jobs in readyJobs queue
		time++;
	}
}

int main(int argc, char *argv[]){

    todoJobs = newJOBQ(MAXJOBS); 
    readyJobs = newJOBQ(MAXJOBS);
    
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
 
    simulateNoMemory(file, sched, timeQuant);
        
    
    printf("Number of JOBs: %d\n", todoJobs->nElements);  
        

    
   
   
}
