
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
	//printf("line: %s\n", line);

	//Checks if line is special
	if(strncmp(line, "if", 2) == 0){
		char tok_str[9][BUFSIZ];
		char *token;
		char var;
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

		var = tok_str[1][0];
		linenum = atoi(tok_str[8]);
		compare = atoi(tok_str[3]);



		//printf("PARSED LINE READS: if %c<%d %c=%c+1 goto %d\n", var, compare, var, var, linenum);




		//Check if variable already exists and if not create a new one
		int var_index = 0;
		while (var_index < job->num_vars && job->vars[var_index] != var) {
			var_index++;
		}

		printf("REACHED: vars[%d] = %c = %d\n", var_index, job->vars[var_index], job->var_values[var_index]);


                //Create Variable
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





void processLineFromCache(int cacheFrame, MEMORY *cache, int jid) {
	// determine if the line to be processed is an even or odd line
	int line = (jobList[jid].currentline + 1) % 2;
	char *data = cache->frames[cacheFrame].data[line];
	processSingleLine(data,jid);
}




/*
 * Find the pagetable corresponding to the page in 'frame' and update it to -1 to
 * effectively remove this page from ram. Note: the data in the frame will remain, so
 * this function should only be used in conjunction with a reallocation of this frame.
 */
void removeFromRAM(MEMORY *ram, int frame) {
	int pageNumber = ram->frames[frame].page_number;

	for(int i=0; i<MAXJOBS; i++) {
		// find the relevant page table and update it
		if( pagetables[i].RAMFrame[pageNumber] == frame ) {
			pagetables[i].RAMFrame[pageNumber] = -1;
		}
	}
}


/*
 * Updates the least recently used (LRU) array in ram, according to the new most recently
 * used frame(mru), i.e. moves the frame 'mru' to the end of the array, sifting other frames
 * forward accordingly.
 */
void updateLRU(MEMORY *ram, int mru) {
	// the index corresponding to the most recently used frame
	int lruIndex = 0;
	for(int i=0; i< ram->num_frames; i++ ) {
		if(ram->LRU[i] == mru) {
			lruIndex = i;
			break;
		}
	}

	for(int i=lruIndex; i< (ram->num_frames-1); i++) {
		ram->LRU[i] = ram->LRU[i+1];
	}
	// the most recently used frame is now at the end of the list
	ram->LRU[ram->num_frames-1] = mru;
}
/*
 * Load one page from harddisk to ram, evicting the least recently used (LRU) frame
 * from ram in order to free up space.
 */
void loadPageToRAM(MEMORY *ram, PAGE p, int jid) {
	// find the frame that must be removed from ram and update its pagetable entry
	int lru = ram->LRU[0];
	removeFromRAM(ram,lru);
	// add the new page and its associated lines to ram
	ram->frames[lru] = p;
	// update page table to reflect change
	pagetables[jid].RAMFrame[p.page_number] = lru;
	// update LRU by moving the frame that was just accessed to the end of the list
	updateLRU(ram,lru);
}

/*
 * Find and return the page in the harddrive associated with a specific line and job.
 */
PAGE getPage(MEMORY *harddrive, int jid, int line) {
	int pagenum = pagetables[jid].pageIndex[line];
	int hdd_framenum = pagetables[jid].hdd_frameIndex[pagenum];
	return harddrive->frames[hdd_framenum];
}

/*
 * Add the specified page to the specified frame in the cache, updating the page table accordingly.
 */
void addToCache(MEMORY *cache, PAGE p, int frame, int jid) {
	cache->frames[frame] = p;
	pagetables[jid].cacheFrame[p.page_number] = frame;
}

/*
 * Find the pagetable corresponding to the page in 'frame' and update it to -1 to
 * effectively remove this page from cache. Note: the data in the frame will remain, so
 * this function should only be used in conjunction with a reallocation of this frame.
 */
void removeFromCache(MEMORY *cache, int frame) {
	int pageNumber = cache->frames[frame].page_number;

        
        for(int i=0; i< MAXJOBS; i++){
		// find the relevant page table and update it

               if( pagetables[i].cacheFrame[pageNumber] == frame ) {
			pagetables[i].cacheFrame[pageNumber] = -1;
		}
                
	}
}
/*
 * Load two pages from ram to cache and process the first line in the first page. If
 * there is only one page page of this job in ram, this will cause a page fault and
 * return false, but will load the second page into ram from disk. If there is only
 * on page of the job remaining, it will be loaded into the cache and true will be
 * return (as no page fault will be generated).
 */
bool processLineFromRAM(MEMORY *harddrive, MEMORY *ram, MEMORY *cache, int jid, int frame) {
	// the current line (defined for conciseness)
	int line = jobList[jid].currentline;
	// find the next page to be processed (since we need to move TWO pages to cache)
	int nextLine;
	int nextPage;
	if( (line + 1) % 2 == 0) {
		nextLine = line + 1;
	} else {
		nextLine = line + 2;
	}
	// set nextPage to reflect the index of the next page, or -1 if there is no next page
	if(nextLine <= jobList[jid].length) {
		nextPage = pagetables[jid].pageIndex[nextLine];
	} else {
		nextPage = -1;
	}

	// if there is a next page for this job and it is not in ram, page fault
	if( nextPage != -1 && pagetables[jid].RAMFrame[nextPage] == -1 ) {
		PAGE p = getPage(harddrive,jid,nextLine);
		loadPageToRAM(ram, p, jid);
		return false;

		// if the next line is beyond the length of the job, allow one page to be moved to cache
	} else if ( nextPage == -1 ) {
		// delete previous page from cache
		removeFromCache(cache,0);
		// arbitrarily move the current page it to the first frame in cache
		PAGE p = getPage(harddrive,jid,line);
		cache->frames[0] = p;
		pagetables[jid].cacheFrame[p.page_number] = 0;
		// update the LRU array in RAM
		updateLRU(ram,frame);

		// otherwise move the next two pages to cache (this is the expected case)
	} else {
		// remove the previous pages
		removeFromCache(cache,0);
		removeFromCache(cache,1);
		// add the new pages and update the LRU array in ram
		addToCache(cache, getPage(harddrive,jid,line), 0, jid);
		addToCache(cache, getPage(harddrive,jid,nextLine+1), 1, jid);
		updateLRU(ram, frame);
		updateLRU(ram, pagetables[jid].RAMFrame[nextPage]);
	}

	// process the first line moved to cache. If the function reaches this point, it must have
	// moved at least one line to cache (if not it will have returned false).
	processLineFromCache(0, cache,jid);
	return true;
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
				continue;
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

	char* buffer = calloc(1,BUFSIZ);
	char* result = calloc(1,BUFSIZ);
	char* token = calloc(1,BUFSIZ);
	char* placeholder = calloc(1,BUFSIZ);
	char rrResults[MAXJOBS][BUFSIZ];

	for(int i =0; i< end; i++){
		printf("%d",results[i]);
	}

	printf("\n");

	for(int i = 0; i < end; i++){
		if(results[i] == MAXJOBS){
			snprintf(buffer,10,"%s", "X");
		} else {
			snprintf(buffer, 10, "%d", results[i]);
		}

		strcat(result, buffer);

	}
        
        printf("%s\n", result);

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
			snprintf(buffer, 10, "%d ", start);
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

/*
 * Finds the job associated with the specified frame in the specified memory
 * object. This will be either ram or cache, as specified by the 'ram' bool.
 *
 * After finding the job, this function creates a string of length 180 chars
 * and returns the memory dump of the specified frame in this string.
 */
bool dumpFrame(MEMORY *m, int frame, bool ram, char *output) {
	
	int jid = -1;

	// find the job associated with this page
	int pageNumber = m->frames[frame].page_number;
	for(int j=0; j<MAXJOBS; j++) {
		// look for the frame in the appropriate page table
		if(ram) {
			//printf("HERE ram: %d %d\n",frame,j);
			if( pagetables[j].RAMFrame[pageNumber] == frame ) {
				jid = j;
				break;
			}
		} else {
			if( pagetables[j].cacheFrame[pageNumber] == frame ) {
				jid = j;
				break;
			}
		}
	}
	// if no job has been found, the frame must be empty
	if(jid == -1) {
		return false;
	}
	if(m->frames[frame].data[0] == NULL && m->frames[frame].data[1] == NULL) {
		return false;
	}

	char *nextline = calloc(BUFSIZ,1);
	char *line = calloc(BUFSIZ,1);
	sprintf(line,"Frame %d: %s\n",frame,jobList[jid].filename);
	//printf("HERE: %d\n %s\n",frame,m->frames[frame].data[1]);
	// append the two lines
	strcat(nextline,line);
	strcat(nextline,m->frames[frame].data[0]);
	strcat(nextline,"\n");
	strcat(nextline,m->frames[frame].data[1]);
	strcat(nextline,"\n\n");
	
	// finally, append the dump of this frame to the output string
	strcat(output,nextline);
        free(line);
	free(nextline);
	return true;
}

/*
 * Print out the current contents of RAM and cache to the output string.
 * If a frame is empty, it will still be included, but no lines or job numbers
 * will be listed, instead it will just say 'EMPTY' under the frame.
 *
 * The format of this is as follows:
 *
 * 		*** RAM CONTENTS ***
 *
 * 		Frame 1: job 3
 * 		line1addaiado
 * 		line2iodaiodsa
 *
 * 		... (etc, middle 6 frames)
 *
 * 		Frame 8: job 3
 * 		line1addaiado
 * 		line2iodaiodsa
 *
 * 		*** CACHE CONTENTS ***
 *
 * 		Frame 1: job 3
 * 		line1addaiado
 * 		line2iodaiodsa
 *
 * 		Frame 2: job 3
 * 		line1addaiado
 * 		line2iodaiodsa
 */
void dumpMemory(MEMORY *ram, MEMORY *cache, char *output, bool firstJobStarted) {
	// start dumping RAM
    char *line = calloc(BUFSIZ,1);
	strcpy(line,"*** RAM CONTENTS ***\n\n");
	strcat(output,line);

	// dump each frame and append to output
	for(int i=0; i<ram->num_frames; i++) {
		if(!firstJobStarted) {
			// if this frame is empty, print EMPTY
			sprintf(line, "Frame %d: \nEMPTY\n\n",i);
			strcat(output,line);
			continue;
		}
		if( !dumpFrame(ram,i,true,output) ) {
			// if this frame is empty, print EMPTY
			sprintf(line, "Frame %d: \nEMPTY\n\n",i);
			strcat(output,line);
		}
	}

	// start dumping cache
	strcpy(line,"*** CACHE CONTENTS ***\n\n");
	strcat(output,line);

	for(int i=0; i<cache->num_frames; i++) {
		if(!firstJobStarted) {
			// if this frame is empty, print EMPTY
			sprintf(line, "Frame %d: \nEMPTY\n\n",i);
			strcat(output,line);
			continue;
		}
		if( !dumpFrame(cache,i,false,output) ) {
			// if this frame is empty, print EMPTY
			sprintf(line,"Frame %d: \nEMPTY\n\n",i);
			strcat(output,line);
			continue;
		}
	}
	
	free(line);
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
		if(isEmptyJOBQ(readyJobs)){
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
		if(!strcmp(sched,roundRobin) && count == timeQuant && j->currentline != j->length){
			rrUp = true;
		}
	}

	printResults(print, time, sched);
	free(harddrive.frames);

}

/*
 * Run a simulation with memory. This will use the default access times of 1 for cache memory
 * and 2 for RAM. The larger hard drive memory has no access cost, but will cause a page fault.
 */
void simulateWithMemory(char* file, char* sched, int timeQuant, int memDump, char* outFile){

	// initialise the three required memory objects and load the lines to harddrive
	MEMORY harddrive;
	harddrive = initialiseMemory(0, MAX_PAGES);
	MEMORY ram;
	ram = initialiseMemory(2,8);
	MEMORY cache;
	cache = initialiseMemory(1,2);

	loadJobFiles(file, harddrive);

	// initialise simulation parameters
	int time =1;
	int count = 0;
	bool ramAccess = false;
	bool rrUp = false;
	int print[MAXTIME];
	for (int i = 0; i < MAXTIME; i++) {
		print[i] = MAXJOBS;
	}
	
	// allocate a string to contain the output of this simulation
	/* ASSUMPTION: the lines in all job files are, on average, less than 80 chars long
	 */
	char* output = calloc( (MAXTIME/memDump * 10) * 220, sizeof(char));
        
	bool firstJobStarted = false;
	// ensure that at least one job remains, regardless of whether it has begun execution
	while( !isEmptyJOBQ(todoJobs) || !isEmptyJOBQ(readyJobs) ) {

		// look for new jobs to move to 'ready' at this time
		while( !isEmptyJOBQ(todoJobs) && (peekJOBQ(todoJobs).start == time) ) {
			if(!firstJobStarted) {
				firstJobStarted = true;
			}
			
			JOB newJob = dequeueJOBQ(todoJobs);
			jobList[newJob.jobID] = newJob;
			enqueueJOBQ(newJob, readyJobs);

			//printf("~~~~~~~~~~New process jid = %d came alive at time %d~~~~~~~~~~\n", newJob.jobID, time);

			// load the first two pages (four lines) of the new job from disk to ram
			for(int i=0; i<2; i++) {
				int pagenum = pagetables[newJob.jobID].pageIndex[newJob.currentline + 2*i];
				int hdd_framenum = pagetables[newJob.jobID].hdd_frameIndex[pagenum];
				loadPageToRAM(&ram, harddrive.frames[hdd_framenum], newJob.jobID);
			}
		}

		if( time == memDump ) {
			dumpMemory(&ram,&cache,output,firstJobStarted);
		}

		// if this loop is occupied by an access to ram, ignore all execution operations
		if(!ramAccess){

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
				//printf("~~~~~~~~~~Process Died jid = %d at time %d~~~~~~~~~~\n", jid, time);
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



			//PROCESS LINE FROM CACHE
			int frame;
			int pagenum = pagetables[jid].pageIndex[j->currentline];
			if((frame = pagetables[jid].cacheFrame[pagenum]) != -1){
				if(!strcmp(sched, FCFS) || cache.accessCost <= timeQuant - count){
					processLineFromCache( frame, &cache, jid);
					// cost of processing from cache is 1 in the case of this project
					print[time] = jid;

					//printf("CACHE TIME: %d\n", time);					
					count++;

					time++;
					continue;
				} else {
					rrUp = true;
					continue;
				}
			} else {
				//PROCESS LINE FROM RAM
				if((frame = pagetables[jid].RAMFrame[pagenum]) != -1){
					if(!strcmp(sched, FCFS) || ram.accessCost <= timeQuant - count){
						// if process from ram was successful, increment time
						if( processLineFromRAM(&harddrive,&ram,&cache, jid, frame) ) {
							// cost of filling cache and processing line is 2 in the case of this project
							print[time] = jid;
							print[time+1] = jid;

							//printf("RAM TIME: %d\n", time);
							count += 2;

							time++;
							// register that during the next iteration, nothing can be processed.
							ramAccess = true;
							continue;
							// otherwise, if a page fault occurred, loop with no time increment (free to fill ram)
						} else {
							// load the NEXT page from disk to ram
							loadPageToRAM(&ram, harddrive.frames[pagetables[jid].hdd_frameIndex[pagenum+1]], jid);
							rrUp = true;
							continue;
						}
					} else {
						// if there isn't enough time to move new pages to cache, move on to next job
						rrUp = true;
						continue;
					}

				} else {
					// load the page from disk to ram
					loadPageToRAM(&ram, harddrive.frames[pagetables[jid].hdd_frameIndex[pagenum]], jid);
					rrUp = true;
					continue;
				}
			}


			//Check for timequantum
			if(!strcmp(sched,roundRobin) && count == timeQuant && j->currentline != j->length){
				rrUp = true;
				continue;
			}


			// if loop was ignored due to ram access, increment time and reset to false
		} else {
			ramAccess = false;
			time++;
		}

	}

	
	printResults(print, time, sched);

	FILE *f;
	f = fopen(outFile, "w");
	fprintf(f, output);
	fclose(f);


}



int main(int argc, char *argv[]){

	todoJobs = newJOBQ(MAXJOBS);
	readyJobs = newJOBQ(MAXJOBS);

	int timeQuant;        //Time quantum for RR scheduling
	int numJobs;          //Number of Jobs
	char* sched;          //Type of schedule
	char* file;           //Name of file that contains jobs

	
	// based on the number of arguments, run the correct simulation
	switch( argc ) {

		// FCFS no memory
		case 3:
			sched = argv[1];
			if(strcmp(sched, roundRobin) == 0) {
				DieWithUserMessage("Parameter(s)", "<Time Quantum>");
			}
			if(strcmp(sched, FCFS) == 0) {
				file = argv[2];
				timeQuant = 1;
				simulateNoMemory(file,sched,timeQuant);
			} else {
				DieWithUserMessage("Parameter(s)", "<Schedule Type>");
			}
			break;

		// RR no memory
		case 4:
			sched = argv[1];                      //Type of schedule
			if(strcmp(sched, roundRobin) == 0) {
				timeQuant = atoi(argv[2]);      //Set time quantum
				file = argv[3];                 //Name of file that contains jobs
				simulateNoMemory(file,sched,timeQuant);
			} else {
				DieWithUserMessage("Parameter(s)", "<Schedule Type>");
			}
			break;

		// FCFS with memory
		case 6:
			sched = argv[3];                      //Type of schedule
			if(strcmp(sched, roundRobin) == 0) {
				DieWithUserMessage("Parameter(s)", "<Time Quantum>");
			}
			if(strcmp(argv[1],"-m") == 0) {
				if(strcmp(sched, FCFS) == 0) {
					int memDump = atoi(argv[2]);
					timeQuant = 1;					//Set time quantum
					file = argv[4];                 //Name of file that contains jobs
					char* outFile = argv[5];
					simulateWithMemory(file,sched,timeQuant,memDump,outFile);
				} else {
					DieWithUserMessage("Parameter(s)", "<Schedule Type>");
				}
			} else {
				DieWithUserMessage("Parameter(s)", "<Memory>");
			}
			break;
			
		// RR with memory
		case 7:
			sched = argv[3];                      //Type of schedule
			if(strcmp(argv[1],"-m") == 0) {
				if(strcmp(sched, roundRobin) == 0) {
					int memDump = atoi(argv[2]);
					timeQuant = atoi(argv[4]);		//Set time quantum
					if(timeQuant < 3) {
						DieWithUserMessage("Error","Please choose a larger time quantum.");
					}
					file = argv[5];                 //Name of file that contains jobs
					char* outFile = argv[6];
					simulateWithMemory(file,sched,timeQuant,memDump,outFile);
				} else {
					DieWithUserMessage("Parameter(s)", "<Schedule Type>");
				}
			} else {
				DieWithUserMessage("Parameter(s)", "<Memory>");
			}
			break;
			
		// otherwise wrong number of arguments, exit with error message
		default:
			DieWithUserMessage("Parameter(s)", "<Schedule Type> <File>");
	}

}
