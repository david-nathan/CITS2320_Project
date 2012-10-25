
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

void simulateNoMemory(char* file, char* sched, int timeQuant){
    
    MEMORY harddrive;
    harddrive.num_frames = MAXJOBS * MAX_PAGES;
    harddrive.frames = calloc(harddrive.num_frames, sizeof(PAGE));
    
    
    loadJobFiles(file, harddrive);
    
    int time = 1;
    int count = 0;
    bool rrUp = false; 
    
    while(!isEmptyJOBQ(todoJobs) || !isEmptyJOBQ(readyJobs)){
        
        while(!isEmptyJOBQ(todoJobs) && peekJOBQ(todoJobs).start == time){
            JOB newJob = dequeueJOBQ(todoJobs);
            jobList[newJob.jobID] = newJob;
            enqueueJOBQ(newJob, readyJobs);
            printf("~~~~~~~~~~New process jid = %d came alive at time %d~~~~~~~~~~\n", newJob.jobID, time);
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
        if(sched == roundRobin && rrUp){
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
        time++;
        count++;
        
        //Check for timequantum
        if(count == timeQuant && j->currentline != j->length){
            rrUp = true;
         }
    }
    
    
    free(harddrive.frames);
    
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
	
	MEMORY *harddrive;
	harddrive = initialiseMemory(0,MAX_PAGES);
	
	// now that the harddisk has been created, store the job files on it
	loadJobFiles(file, harddrive);
	
	MEMORY *ram;
	// always 2 access cost and 8 frames for this project
	ram = initialiseMemory(2,8);
	
	MEMORY *cache;
	// always 1 access cost and 2 frames for this project
	cache = initialiseMemory(1,2);
	
	// initialise a parameter to keep track of the time throughout the simulation
	int time = 0;
	// keep track of how long the current job has been executing (for Round Robin)
	int count = 0;
	// keep a variable for the current job and job id
	JOB *j; int jid;
	
	// keeping looping as long as at least one job remains (whether it has begun execution or not)
	while( !isEmptyJOBQ(todoJobs) || !isEmptyJOBQ(readyJobs) ) {		
		// enqueue new jobs due to start at this time (if such jobs exist)
		while( !isEmptyJOBQ(todoJobs) && (peekJOBQ(todoJobs).start == time) ) {
			enqueueJOBQ(dequeueJOBQ(todoJobs),readyJobs);
		}
		// if there are no jobs to process, increment time and restart loop
		if( isEmptyJOBQ(readyJobs) ) {
			time++;
			continue;
		}
	
		//Fetch Next Job
        jid = peekJOBQ(readyJobs).jobID;
        j = &jobList[jid];
        
        //If Job is finished
        if(j->currentline == j->length){
            dequeueJOBQ(readyJobs);
            count = 0;
            continue;
        }
	
		// PROCESS AS FCFS
		
		//TODO
		if( !RR ) {
			//PROCESS line from HDD
			int pagenum = pagetables[jid].pageIndex[j->currentline];
			int hdd_framenum = pagetables[jid].hdd_frameIndex[pagenum];
			PAGE current_page = harddrive.frames[hdd_framenum];
			char* line = current_page.data[(jobList[jid].currentline+1)%2]; //odd line is data[0]
			processSingleLine(line, jid);
			
			// check if the line is already in cache
			if( pagetables[jid].cacheFrame[ hdd_framenum ] != -1) {
			      
				processLineFromCache(pagetables[jid].cacheFrame[ hdd_framenum ], cache, jid);
				// cost of processing from cache is 1 in the case of this project
				time += cache->accessCost;
				continue;
			}
			
			// check if in RAM
			if( pagetables[jid].RAMFrame[ hdd_framenum ] != -1) {
			    
				processLineFromRAM(pagetables[jid].RAMFrame[ hdd_framenum ], ram, cache, jid);
				// cost of filling cache and processing line is 2 in the case of this project
				time += ram->accessCost;
				continue;
			}
			
			// load the page from disk to ram
			loadPageToRAM(hdd_framenum, harddisk, ram, &harddisk->frames[hdd_framenum]);
			
			
			
			
		}		else		{
		// ELSE PROCESS AS RR
			
		}
	
	
		// time increment should only occur here when initially running with no jobs in readyJobs queue
		time++;
	}
}

/*
 * Process one line from the cache.
 */
void processLineFromCache(int cacheFrame, MEMORY *cache, int jid) {
	// determine if the line to be processed is an even or odd line
	int line = jobList[jid].currentline % 2;
	char *data = & (cache->frames[cacheFrame]->data[line]);
	processSingleLine(data,jid);
}

/*
 * Load two pages from ram to cache and process the first line in the first page. If
 * there is only one page page of this job in ram, this will cause a page fault and
 * return false, but will load the second page into ram from disk.
 */
bool processLineFromRAM(MEMORY *ram, MEMORY *cache, int jid) {
	// find the next page to be processed (since we need to move TWO pages to cache)
	int nextLine = jobList[jid].currentline + 2;
	int nextPage = pagetables[jid].pageIndex[nextLine];
	
	// if the next page is not in ram, page fault
	if( pagetables[jid].RAMFrame[nextPage] == -1 ) {
		loadPageToRAM(ram, 
	}
}

/*
 * Load one page from harddisk to ram, evicting the least recently used (LRU) frame
 * from ram in order to free up space.
 */
void loadPageToRAM(MEMORY *ram, PAGE *p, int jid) {
	// find the page number that must be removed from ram
	int lru = ram->LRU[0];
	int evictedPage = ram->frames[lru]->page_number;
	
	// remove this page **************** this currently does not find the correct jid for the evicted page **************************************
	pagetables[ ******* ].RAMFrame[evictedPage] = -1;
	
	// add the new page and its associated lines to ram
	ram->frames[lru] = p;
	// update page table to reflect change
	pagetables[jid].RAMFrame[newPage] = lru;
	
	// update LRU by moving the frame that was just accessed to the end of the list
	for(int i=0; i< (ram->num_frames-1); i++) {
		ram->LRU[i] = LRU[i+1];
	}
	ram->LRU[ram->num_frames-1] = lru;
}

MEMORY* initialiseMemory(int cost, int num_frames) {
	// initialise parameters and allocate space for arrays
	MEMORY m;
	m.num_frames = num_frames;
	m.accessCost = cost;
	m.frames = calloc(num_frames, sizeof(PAGE));
	m.LRU = calloc(num_frames, sizeof(int));
	
	// initialise LRU
	for(int i=0; i<num_frames; i++) {
		LRU[i] = 0;
	}
	
	return &m;
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
    
    
    
    simulateNoMemory(file, sched, timeQuant);
        
    
    
        

    
   
   
}
