/* 
 * File:   Project.c
 * Author: David Nathan 20356245 Daniel Hunt 20350022
 *
 * Created on October 29, 2012, 5:29 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h> 
#include <assert.h> 
#include "Project.h"

//Declaration of external functions
extern char *strdup(const char *);
extern void enqueueJOBQ(JOB element, JOBQ *q);
extern JOBQ *newJOBQ(int size);
extern bool isEmptyJOBQ(JOBQ *q);
extern JOB peekJOBQ(JOBQ *q);
extern JOB dequeueJOBQ(JOBQ *q);
extern void sortJOBQ(JOBQ *q);

//Declaration of global constants

const char* FCFS = "FCFS";
const char* roundRobin = "RR";

/*========= GLOBAL VARIABLES =========*/

/* Array of JOBs that are in the specified by in.file and are indexed by jobID*/
JOB jobList[MAXJOBS];
/* Queue of JOBs that haven't been created yet */
JOBQ *todoJobs;
/* Queue of JOBs that have been created but haven't finished yet*/
JOBQ *readyJobs;
/*Array of PAGETABLEs that are indexed by jobID*/
PAGETABLE pagetables[MAXJOBS];

/*========= HELPER FUNCTIONS =========*/

/**
 * Function to remove carriage-return or end-of-line character from a string
 * @param line
 */
static void trimLine(char line[]) {
    int i = 0;
    while (line[i] != '\0') {
        if (line[i] == CARRIAGE_RETURN || line[i] == EOL) {
            line[i] = '\0';
        }
        i++;
    }
}

/**
 * For non-standard errors
 * @param msg
 * @param detail
 */
static void DieWithUserMessage(const char *msg, const char *detail) {
    fputs(msg, stderr);
    fputs(": ", stderr);
    fputs(detail, stderr);
    fputc('\n', stderr);
    exit(1);
}

/**
 * For standard system errors
 * @param msg
 */
static void DieWithSystemMessage(const char *msg) {
    perror(msg);
    exit(1);
}

/*========= FUNCTIONS MEMORY AND NOMEMORY SIMULATIONS =========*/

void loadJobFiles(char* infile, MEMORY harddrive) {

    FILE *fp = fopen(infile, "r");
    if (fp == NULL) {       //Test that the infile can be opened
        DieWithSystemMessage("Unable to open JOB-List file");
    }

    int jobID = 0;          //Assigns jobID according to position in infile not job start time
    int hdFrameCount = 0;   //Count for the frames
    char jobFile[BUFSIZ];   //Buffer to hold the filename for a JOB

    while (fgets(jobFile, sizeof (jobFile), fp) != NULL) {  //Reads a newline-terminated  string from infile stream
        trimLine(jobFile);                                  //Remove trailing characters
        FILE *fpJob = fopen(jobFile, "r");
        if (fpJob == NULL) {
            DieWithSystemMessage("Unable to open JOB file");//Test that JOB file can be opened
        }

        char buffer[BUFSIZ]; //A buffer to hold each line in a JOB
        int linecount = 0;   //A count of which line is being read in
        int pagecount = 0;   //A count of which page is being created
   
        //Initialize a new JOB structure
        JOB newJob;
        newJob.currentline = 1;
        newJob.num_vars = 0;
        newJob.jobID = jobID;
        newJob.filename = strdup(jobFile);

        while (fgets(buffer, sizeof (buffer), fpJob) != NULL) { //Reads a newline-terminated string from JOB-file stream
            trimLine(buffer);                                   //Remove trailing characters

            if (linecount == 0) {  //If first line in a JOB, store the value as the start time. 
                newJob.start = atoi(buffer);
                linecount++;
                continue;
            }

            if (linecount % 2 != 0) { //Odd line number
                //Create new page and add to harddrive
                PAGE page;
                page.data[0] = strdup(buffer);
                page.page_number = pagecount;
                harddrive.frames[hdFrameCount] = page; //Add page to harddrive

                //Update page table
                pagetables[jobID].pageIndex[linecount] = pagecount;
                pagetables[jobID].hdd_frameIndex[pagecount] = hdFrameCount;
                pagetables[jobID].RAMFrame[pagecount] = -1; //False, since it isn't in RAM
                pagetables[jobID].cacheFrame[pagecount] = -1; //False, since it isn't in CACHE

                pagecount++;
                hdFrameCount++;
            } else { //Even line number
                //Update page table
                harddrive.frames[hdFrameCount - 1].data[1] = strdup(buffer);
                pagetables[jobID].pageIndex[linecount] = pagecount - 1;
            }
            linecount++;
        }
        newJob.length = linecount - 1;

        //Add new Job to queue of Jobs that haven't started 
        enqueueJOBQ(newJob, todoJobs);
        jobID++;
        fclose(fpJob);
    }
    sortJOBQ(todoJobs); //Sort todoJobs according to start time
}

/**
 * Reads in a single line from a Job file and processes it.
 * Checks if it is a special line and processes it accordingly.
 * @param line
 * @param jobID
 */
void processSingleLine(char* line, int jobID) {
    JOB *job = &(jobList[jobID]);
    char *copy = calloc(1, BUFSIZ);
   
    strcpy(copy, line);                //Create Copy of line so that it's not altered in strtok

    //Check if line is special by comparing first two characters to 'if'
    if (strncmp(line, "if", 2) == 0) {

        char tok_str[9][BUFSIZ];       //Array to hold the 9 tokens in a special line
        char *token;                   //A Buffer to hold the token
        char var;                      //The variable that is being evaluated in the line
        int linenum;                   //The line number that the goto returns to
        int compare;                   //The value that the variable is compared against

        int n = 0;                     //Count for the strtok while-loop

                                            
        if ((token = strtok(copy, " \t")) != NULL) { //Isolates tokens in line that are separated by whitespace or tab spaces
            do {
                strcpy(tok_str[n], token);           //Copies token into Array
                n++;
            } while ((token = strtok(NULL, " \t")) != NULL);
        }

        var = tok_str[1][0];           //Takes the first byte of the second element in the array NOTE: multibyte characters will not be stored 
        linenum = atoi(tok_str[8]);    //Assign string to integer for linenum
        compare = atoi(tok_str[3]);    //Assign string to integer for compare

        //printf("PARSED LINE: if %c<%d %c=%c+1 goto %d\n", var, compare, var, var, linenum);

        //Check if variable already exists
        int var_index = 0;
        while (var_index < job->num_vars && job->vars[var_index] != var) {
            var_index++;
        }

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

        //Evaluate line
        if (job->var_values[var_index] < compare) {
            job->var_values[var_index] = job->var_values[var_index] + 1;
            job->currentline = linenum;
        } else {
            job->currentline = job->currentline + 1;

        }
        //printf("JOB %s VARIABLE: vars[%d] = %c = %d\n",job->filename, var_index, job->vars[var_index], job->var_values[var_index]);
    } else {
        //Non-special line and so move to the next line
        job->currentline = job->currentline + 1;
    }
}

/**
 * Prints a summary of the simulation from an integer array, indexed by jobID,
 * by turning it into a string and isolating substrings of jobIDs.
 * @param results Array of jobIDs indexed by when they are processing
 * @param end     Last time-unit that was used in simulation
 * @param sched   Type of scheduling used
 */
void printResults(int *results, int end, char *sched) {
    
    printf("========== SUMMARY OF RESULTS ============\n\n");
    
    char* buffer = calloc(1, BUFSIZ);//Buffer for snprintf that holds 10 Bytes
    char* result = calloc(1, BUFSIZ);      //A string that holds the summary
    char* token = calloc(1, BUFSIZ);       //A string that holds the characters that act as tokens
    char* placeholder = calloc(1, BUFSIZ); //A string that holds all characters that have been encountered
    char rrResults[MAXJOBS][BUFSIZ];       //an array of job filenames for RR simulations

    for (int i = 0; i < end; i++) {
        if (results[i] == MAXJOBS) { //If value at specified is the default value then replace it with an X
            snprintf(buffer, 10, "%s", "X");        //Writes at most 10 bytes from the string 'X' to buffer
        } else {
            snprintf(buffer, 10, "%d", results[i]); //Writes at most 10 bytes from results integer array  to buffer
        }
        strcat(result, buffer); //Concatenate buffer to result
    }

    //printf("RESULTS: %s\n",result);
    
    strcat(token, "X"); //Add idle-character to tokens
    int start = 0;   //Position of start 
    size_t position = 0;
    int length = 0;

    while ((position = strspn(result, token)) != strlen(result)) { //Determines the length of string made entirely from characters in token

        start += position; //Set the start position of a JOB
        result += position;//Move the pointer along
        strncpy(placeholder, result, 1); //Copy first character in result string to placeholder
        length = strspn(result, placeholder); //Determines the length string made of characters already encounterd. Used in FCFS

        if (!strcmp(sched, FCFS)) { //If FCFS then print to stdout start and end times
            printf("%s %d %d\n", jobList[atoi(placeholder)].filename, start, start + length - 1);
            strcat(token, placeholder); //Add placeholder to set of characters already encountered in token
        }
        if (!strcmp(sched, roundRobin)) { //If RR then write to the array of results
            snprintf(buffer, 10, "%d ", start);
            strcat(rrResults[atoi(placeholder)], buffer);
            strcat(placeholder, "X");
            strcpy(token, placeholder);
        }
    }

    if (!strcmp(sched, roundRobin)) { //If RR then print the results now.
        int i = 0;
        while (jobList[i].filename != NULL) {
            printf("%s %s\n", jobList[i].filename, rrResults[i]);
            i++;
        }
    }
    
    free(buffer);
    free(token);
    free(placeholder);
}

/*========= HELPER FUNCTIONS FOR MEMORY =========*/

/**
 * Allocates space for a MEMORY structure
 * @param cost The Access Cost
 * @param num_frames
 * @return The MEMORY structure
 */
MEMORY initialiseMemory(int cost, int num_frames) {
    // initialise parameters and allocate space for arrays
    MEMORY m;
    m.num_frames = num_frames;
    m.accessCost = cost;
    m.frames = calloc(num_frames, sizeof (PAGE));
    m.LRU = calloc(num_frames, sizeof (int));

    // initialise LRU
    for (int i = 0; i < num_frames; i++) {
        m.LRU[i] = i;
    }

    return m;
}

/**
 * If line is from CACHE, determines which line from frame is needed
 * and then processes it.
 * @param cacheFrame
 * @param cache
 * @param jid
 */
void processLineFromCache(int cacheFrame, MEMORY *cache, int jid) {
    // determine if the line to be processed is an even or odd line
    int line = (jobList[jid].currentline + 1) % 2;
    char *data = cache->frames[cacheFrame].data[line];
    processSingleLine(data, jid);
}

/**
 * Find the pagetable corresponding to the page in 'frame' and update it to -1 to
 * effectively remove this page from ram. Note: the data in the frame will remain, so
 * this function should only be used in conjunction with a reallocation of this frame.
 * @param ram
 * @param frame
 */
void removeFromRAM(MEMORY *ram, int frame) {
    int pageNumber = ram->frames[frame].page_number;

    for (int i = 0; i < MAXJOBS; i++) {
        // find the relevant page table and update it
        if (pagetables[i].RAMFrame[pageNumber] == frame) {
            pagetables[i].RAMFrame[pageNumber] = -1;
        }
    }
}

/**
 * Updates the least recently used (LRU) array in RAM, according to the new most recently
 * used frame(mru), i.e. moves the frame 'mru' to the end of the array, sifting other frames
 * forward accordingly.
 * @param ram
 * @param mru
 */
void updateLRU(MEMORY *ram, int mru) {
    
    int lruIndex = 0;   // the index corresponding to the most recently used frame
    for (int i = 0; i < ram->num_frames; i++) {
        if (ram->LRU[i] == mru) {
            lruIndex = i;
            break;
        }
    }

    for (int i = lruIndex; i < (ram->num_frames - 1); i++) {
        ram->LRU[i] = ram->LRU[i + 1];
    }
    
    ram->LRU[ram->num_frames - 1] = mru;  // the most recently used frame is now at the end of the list
}

/**
 * Load one page from HARDDISK to RAM, evicting the least recently used (LRU) frame
 * from RAM in order to free up space.
 * @param ram
 * @param p
 * @param jid
 */
void loadPageToRAM(MEMORY *ram, PAGE p, int jid) {
    // find the frame that must be removed from ram and update its pagetable entry
    int lru = ram->LRU[0];
    removeFromRAM(ram, lru);
    // add the new page and its associated lines to ram
    ram->frames[lru] = p;
    // update page table to reflect change
    pagetables[jid].RAMFrame[p.page_number] = lru;
    // update LRU by moving the frame that was just accessed to the end of the list
    updateLRU(ram, lru);
}

/**
 * Find and return the page in the harddrive associated with a specific line and job.
 * @param harddrive
 * @param jid
 * @param line
 * @return 
 */
PAGE getPage(MEMORY *harddrive, int jid, int line) {
    int pagenum = pagetables[jid].pageIndex[line];
    int hdd_framenum = pagetables[jid].hdd_frameIndex[pagenum];
    return harddrive->frames[hdd_framenum];
}

/**
 * Add the specified page to the specified frame in the cache, updating the page table accordingly.
 * @param cache
 * @param p
 * @param frame
 * @param jid
 */
void addToCache(MEMORY *cache, PAGE p, int frame, int jid) {
    cache->frames[frame] = p;
    pagetables[jid].cacheFrame[p.page_number] = frame;
}

/**
 * Find the pagetable corresponding to the page in 'frame' and update it to -1 to
 * remove this page from cache. Note: the data in the frame will remain, so
 * this function should only be used in conjunction with a reallocation of the frame.
 * @param cache
 * @param frame
 */
void removeFromCache(MEMORY *cache, int frame) {
    int pageNumber = cache->frames[frame].page_number;

    for (int i = 0; i < MAXJOBS; i++) {
        // find the relevant page table and update it
        if (pagetables[i].cacheFrame[pageNumber] == frame) {
            pagetables[i].cacheFrame[pageNumber] = -1;
        }
    }
}

/**
 * Load two pages from ram to cache and process the first line in the first page. If
 * there is only one page page of this job in ram, this will cause a page fault and
 * return false, but will load the second page into ram from disk. If there is only
 * on page of the job remaining, it will be loaded into the cache and true will be
 * return (as no page fault will be generated).
 * @param harddrive
 * @param ram
 * @param cache
 * @param jid
 * @param frame
 * @return 
 */
bool processLineFromRAM(MEMORY *harddrive, MEMORY *ram, MEMORY *cache, int jid, int frame) {
    // the current line (defined for conciseness)
    int line = jobList[jid].currentline;
    // find the next page to be processed (since we need to move TWO pages to cache)
    int nextLine;
    int nextPage;
    if ((line + 1) % 2 == 0) {
        nextLine = line + 1;
    } else {
        nextLine = line + 2;
    }
    // set nextPage to reflect the index of the next page, or -1 if there is no next page
    if (nextLine <= jobList[jid].length) {
        nextPage = pagetables[jid].pageIndex[nextLine];
    } else {
        nextPage = -1;
    }

    // if there is a next page for this job and it is not in ram, page fault
    if (nextPage != -1 && pagetables[jid].RAMFrame[nextPage] == -1) {
        PAGE p = getPage(harddrive, jid, nextLine);
        loadPageToRAM(ram, p, jid);
        return false;

        // if the next line is beyond the length of the job, allow one page to be moved to cache
    } else if (nextPage == -1) {
        // delete previous page from cache
        removeFromCache(cache, 0);
        // arbitrarily move the current page it to the first frame in cache
        PAGE p = getPage(harddrive, jid, line);
        cache->frames[0] = p;
        pagetables[jid].cacheFrame[p.page_number] = 0;
        // update the LRU array in RAM
        updateLRU(ram, frame);

        // otherwise move the next two pages to cache (this is the expected case)
    } else {
        // remove the previous pages
        removeFromCache(cache, 0);
        removeFromCache(cache, 1);
        // add the new pages and update the LRU array in ram
        addToCache(cache, getPage(harddrive, jid, line), 0, jid);
        addToCache(cache, getPage(harddrive, jid, nextLine + 1), 1, jid);
        updateLRU(ram, frame);
        updateLRU(ram, pagetables[jid].RAMFrame[nextPage]);
    }

    // process the first line moved to cache. If the function reaches this point, it must have
    // moved at least one line to cache (if not it will have returned false).
    processLineFromCache(0, cache, jid);
    return true;
}





/**
 * Finds the job associated with the specified frame in the specified memory
 * object. This will be either ram or cache, as specified by the 'ram' bool.
 * After finding the job, this function creates a string of length 180 chars
 * and returns the memory dump of the specified frame in this string.
 * @param m
 * @param frame
 * @param ram
 * @param output
 * @return 
 */
bool dumpFrame(MEMORY *m, int frame, bool ram, char *output) {

    int jid = -1;

    // find the job associated with this page
    int pageNumber = m->frames[frame].page_number;
    for (int j = 0; j < MAXJOBS; j++) {
        // look for the frame in the appropriate page table
        if (ram) {
            //printf("HERE ram: %d %d\n",frame,j);
            if (pagetables[j].RAMFrame[pageNumber] == frame) {
                jid = j;
                break;
            }
        } else {
            if (pagetables[j].cacheFrame[pageNumber] == frame) {
                jid = j;
                break;
            }
        }
    }
    // if no job has been found, the frame must be empty
    if (jid == -1) {
        return false;
    }
    if (m->frames[frame].data[0] == NULL && m->frames[frame].data[1] == NULL) {
        return false;
    }

    char *nextline = calloc(BUFSIZ, 1);
    char *line = calloc(BUFSIZ, 1);
    sprintf(line, "Frame %d: %s\n", frame, jobList[jid].filename);
    //printf("HERE: %d\n %s\n",frame,m->frames[frame].data[1]);
    // append the two lines
    strcat(nextline, line);
    strcat(nextline, m->frames[frame].data[0]);
    strcat(nextline, "\n");
    strcat(nextline, m->frames[frame].data[1]);
    strcat(nextline, "\n\n");

    // finally, append the dump of this frame to the output string
    strcat(output, nextline);
    free(line);
    free(nextline);
    return true;
}

/**
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
 * @param ram
 * @param cache
 * @param output
 * @param firstJobStarted
 */
void dumpMemory(MEMORY *ram, MEMORY *cache, char *output, bool firstJobStarted) {
    // start dumping RAM
    char *line = calloc(BUFSIZ, 1);
    strcpy(line, "*** RAM CONTENTS ***\n\n");
    strcat(output, line);

    // dump each frame and append to output
    for (int i = 0; i < ram->num_frames; i++) {
        if (!firstJobStarted) {
            // if this frame is empty, print EMPTY
            sprintf(line, "Frame %d: \nEMPTY\n\n", i);
            strcat(output, line);
            continue;
        }
        if (!dumpFrame(ram, i, true, output)) {
            // if this frame is empty, print EMPTY
            sprintf(line, "Frame %d: \nEMPTY\n\n", i);
            strcat(output, line);
        }
    }

    // start dumping cache
    strcpy(line, "*** CACHE CONTENTS ***\n\n");
    strcat(output, line);

    for (int i = 0; i < cache->num_frames; i++) {
        if (!firstJobStarted) {
            // if this frame is empty, print EMPTY
            sprintf(line, "Frame %d: \nEMPTY\n\n", i);
            strcat(output, line);
            continue;
        }
        if (!dumpFrame(cache, i, false, output)) {
            // if this frame is empty, print EMPTY
            sprintf(line, "Frame %d: \nEMPTY\n\n", i);
            strcat(output, line);
            continue;
        }
    }

    free(line);
}

/*========= SIMULATION FUNCTIONS =========*/



/**
 * Runs the execution of the simulation without any use of CACHE and RAM. It reads
 * all JOBs into the HARDDRIVE.
 * @param file
 * @param sched
 * @param timeQuant
 */
void simulateNoMemory(char* file, char* sched, int timeQuant) {

    MEMORY harddrive;
    harddrive.num_frames = MAXJOBS * MAX_PAGES;
    harddrive.frames = calloc(harddrive.num_frames, sizeof (PAGE));

    loadJobFiles(file, harddrive); //Loads files into harddrive memory

    int time = 1; //Sets to 1
    int count = 0; //Used for RR
    int print[MAXTIME]; //Array of jobIDs being processed that are being indexed by time 
    for (int i = 0; i < MAXTIME; i++) {
        print[i] = MAXJOBS;
    }
    bool rrUp = false; //Used to determine if RR count is up

    //loops until there are no more JOBs to start and all jobs that have been created have finished processing
    while (!isEmptyJOBQ(todoJobs) || !isEmptyJOBQ(readyJobs)) {
        //JOB is created and so is moved over into the readyJobs queue
        while (!isEmptyJOBQ(todoJobs) && peekJOBQ(todoJobs).start == time) {
            JOB newJob = dequeueJOBQ(todoJobs);
            jobList[newJob.jobID] = newJob;
            enqueueJOBQ(newJob, readyJobs);
        }

        //IDLE if no ready jobs
        if (isEmptyJOBQ(readyJobs)) {
            time++;
            continue;
        }

        //Fetch Next Job
        int jid = peekJOBQ(readyJobs).jobID;
        JOB *j = &jobList[jid];

        //If Job is finished
        if (j->currentline == j->length + 1) {
            dequeueJOBQ(readyJobs);
            count = 0;
            continue;
        }

        //Check for if RR time-quantum is up
        if (!strcmp(sched, roundRobin) && rrUp) {
            JOB next = dequeueJOBQ(readyJobs);
            enqueueJOBQ(next, readyJobs);
            count = 0;
            rrUp = false;
            continue;
        }

        //PROCESS line from HDD
        int pagenum = pagetables[jid].pageIndex[j->currentline];   //The currentline indexes current page number in HDD
        int hdd_framenum = pagetables[jid].hdd_frameIndex[pagenum];//pagenum indexes HDD frame in PAGETABLE
        PAGE current_page = harddrive.frames[hdd_framenum];        //The current PAGE 
        char* line = current_page.data[(jobList[jid].currentline + 1) % 2]; //odd line is data[0]                              
        processSingleLine(line, jid); //Process the line
        print[time] = jid; //Update the printing
        time++; //increment time
        count++;//increment count

        //Check for timequantum
        if (!strcmp(sched, roundRobin) && count == timeQuant && j->currentline != j->length + 1) {
            rrUp = true;
        }
    }

    printResults(print, time, sched);
    free(harddrive.frames);

}

/**
 * Run a simulation with memory. This will use the default access times of 1 for cache memory
 * and 2 for RAM. The larger hard drive memory has no access cost, but will cause a page fault.
 * @param file
 * @param sched
 * @param timeQuant
 * @param memDump
 * @param outFile
 */
void simulateWithMemory(char* file, char* sched, int timeQuant, int memDump, char* outFile) {

    // initialise the three required memory objects and load the lines to harddrive
    MEMORY harddrive;
    harddrive = initialiseMemory(0, MAX_PAGES);
    MEMORY ram;
    ram = initialiseMemory(2, 8);
    MEMORY cache;
    cache = initialiseMemory(1, 2);

    loadJobFiles(file, harddrive);

    // initialise simulation parameters
    int time = 1;
    int count = 0;
    bool ramAccess = false;
    bool rrUp = false;
    int print[MAXTIME];
    for (int i = 0; i < MAXTIME; i++) {
        print[i] = MAXJOBS;
    }

    /* allocate a string to contain the output of this simulation
     * ASSUMPTION: the lines in all job files are, on average, less than 80 chars long
     */
    char* output = calloc((MAXTIME / memDump * 10) * 220, sizeof (char));

    bool firstJobStarted = false;
    // ensure that at least one job remains, regardless of whether it has begun execution
    while (!isEmptyJOBQ(todoJobs) || !isEmptyJOBQ(readyJobs)) {

        // look for new jobs to move to 'ready' at this time
        while (!isEmptyJOBQ(todoJobs) && (peekJOBQ(todoJobs).start == time)) {
            if (!firstJobStarted) {
                firstJobStarted = true;
            }

            JOB newJob = dequeueJOBQ(todoJobs);
            jobList[newJob.jobID] = newJob;
            enqueueJOBQ(newJob, readyJobs);

            // load the first two pages (four lines) of the new job from disk to ram
            for (int i = 0; i < 2; i++) {
                int pagenum = pagetables[newJob.jobID].pageIndex[newJob.currentline + 2 * i];
                int hdd_framenum = pagetables[newJob.jobID].hdd_frameIndex[pagenum];
                loadPageToRAM(&ram, harddrive.frames[hdd_framenum], newJob.jobID);
            }
        }

        if (time == memDump) {
            dumpMemory(&ram, &cache, output, firstJobStarted);
        }

        // if this loop is occupied by an access to ram, ignore all execution operations
        if (!ramAccess) {

            //IDLE if no ready jobs
            if (isEmptyJOBQ(readyJobs)) {
                time++;
                continue;
            }

            //Fetch Next Job
            int jid = peekJOBQ(readyJobs).jobID;
            JOB *j = &jobList[jid];

            //If Job is finished
            if (j->currentline == j->length + 1) {
                dequeueJOBQ(readyJobs);
                count = 0;                
                continue;
            }

            //Check for RR schedule
            if (!strcmp(sched, roundRobin) && rrUp) {
                JOB next = dequeueJOBQ(readyJobs);
                enqueueJOBQ(next, readyJobs);
                count = 0;
                rrUp = false;
                continue;
            }

            //PROCESS LINE FROM CACHE
            int frame;
            int pagenum = pagetables[jid].pageIndex[j->currentline];
            if ((frame = pagetables[jid].cacheFrame[pagenum]) != -1) {
                //If schedule is FCFS or there is enough time in RR to process from CACHE
                if (!strcmp(sched, FCFS) || cache.accessCost <= timeQuant - count) {                   
                    processLineFromCache(frame, &cache, jid);
                    // cost of processing from cache is 1 in the case of this project
                    print[time] = jid;				
                    count++;
                    time++;
                    continue;
                } else {
                    rrUp = true;
                    continue;
                }
            } else {
                //PROCESS LINE FROM RAM
                if ((frame = pagetables[jid].RAMFrame[pagenum]) != -1) {
                   //If schedule is FCFS or there is enough time in RR to process from CACHE
                    if (!strcmp(sched, FCFS) || ram.accessCost <= timeQuant - count) {
                        //If process from ram was successful, increment time
                        if (processLineFromRAM(&harddrive, &ram, &cache, jid, frame)) {
                            //Cost of filling cache and processing line is 2 in the case of this project
                            print[time] = jid;
                            print[time + 1] = jid;
                            count += 2;
                            time++;
                            //Indicates that during the next iteration, nothing can be processed.
                            ramAccess = true;
                            continue;
                            //Otherwise, if a page fault occurred, loop with no time increment (free to fill RAM)
                        } else {
                            //Load the NEXT page from HDD to RAM
                            loadPageToRAM(&ram, harddrive.frames[pagetables[jid].hdd_frameIndex[pagenum + 1]], jid);
                            rrUp = true;
                            continue;
                        }
                    } else {
                        //If there isn't enough time to move new pages to CACHE, move on to next JOB
                        rrUp = true;
                        continue;
                    }
                } else {
                    //Load the page from HDD to RAM
                    loadPageToRAM(&ram, harddrive.frames[pagetables[jid].hdd_frameIndex[pagenum]], jid);
                    rrUp = true;
                    continue;
                }
            }
            //Check for timequantum
            if (!strcmp(sched, roundRobin) && count == timeQuant && j->currentline != j->length + 1) {
                rrUp = true;
                continue;
            }
            //If loop was ignored due to ram access, increment time and reset to false
        } else {
            ramAccess = false;
            time++;
        }
    }
    printResults(print, time, sched);
    //Write to outFile for DUMP
    FILE *f;
    f = fopen(outFile, "w");
    if (f == NULL) {       //Test that the outfile can be opened
        DieWithSystemMessage("Unable to open outfile");
    }
    fprintf(f, output);
    fclose(f);
}

void printIntro(){
    printf("\nDavid Nathan 20356245 Daniel Hunt 20350022\n");
    printf("\nCITS2230 Operating Systems.\nUsage:\n");
    char *intro =
            "\nPART 1\n"
            "Project FCFS in.file\n"
            "Project RR d in.file\n"
            "\nPART 2/3\n"
            "Project -m d1 FCFS in.file out.file\n"
            "Project -m d1 RR d in.file out.file\n\n";
    printf("%s", intro);
    
}

int main(int argc, char *argv[]) {
    
    //Initialise Global Variables
    todoJobs = newJOBQ(MAXJOBS);
    readyJobs = newJOBQ(MAXJOBS);

    int timeQuant; //Time quantum for RR scheduling
    char* sched;   //Type of schedule
    char* file;    //Name of file that contains jobs

    //Based on the number of arguments, run the correct simulation
    switch (argc) {
            // FCFS no memory
        case 3:
            printIntro();
            sched = argv[1];
            if (strcmp(sched, roundRobin) == 0) {
                DieWithUserMessage("Parameter(s)", "<Time Quantum>");
            }
            if (strcmp(sched, FCFS) == 0) {
                file = argv[2];
                timeQuant = 1;
                simulateNoMemory(file, sched, timeQuant);
            } else {
                DieWithUserMessage("Parameter(s)", "<Schedule Type>");
            }
            break;
            // RR no memory
        case 4:
            printIntro();
            sched = argv[1]; //Type of schedule
            if (strcmp(sched, roundRobin) == 0) {
                timeQuant = atoi(argv[2]); //Set time quantum
                file = argv[3]; //Name of file that contains jobs
                simulateNoMemory(file, sched, timeQuant);
            } else {
                DieWithUserMessage("Parameter(s)", "<Schedule Type>");
            }
            break;
            // FCFS with memory
        case 6:
            printIntro();
            sched = argv[3]; //Type of schedule
            if (strcmp(sched, roundRobin) == 0) {
                DieWithUserMessage("Parameter(s)", "<Time Quantum>");
            }
            if (strcmp(argv[1], "-m") == 0) {
                if (strcmp(sched, FCFS) == 0) {
                    int memDump = atoi(argv[2]);
                    timeQuant = 1; //Set time quantum
                    file = argv[4]; //Name of file that contains jobs
                    char* outFile = argv[5];
                    simulateWithMemory(file, sched, timeQuant, memDump, outFile);
                } else {
                    DieWithUserMessage("Parameter(s)", "<Schedule Type>");
                }
            } else {
                DieWithUserMessage("Parameter(s)", "<Memory>");
            }
            break;
            // RR with memory
        case 7:
            printIntro();
            sched = argv[3]; //Type of schedule
            if (strcmp(argv[1], "-m") == 0) {
                if (strcmp(sched, roundRobin) == 0) {
                    int memDump = atoi(argv[2]);
                    timeQuant = atoi(argv[4]); //Set time quantum				        
                    file = argv[5]; //Name of file that contains jobs
                    char* outFile = argv[6];
                    simulateWithMemory(file, sched, timeQuant, memDump, outFile);
                } else {
                    DieWithUserMessage("Parameter(s)", "<Schedule Type>");
                }
            } else {
                DieWithUserMessage("Parameter(s)", "<Memory>");
            }
            break;
            //Otherwise wrong number of arguments, exit with error message
        default:
            printIntro();
            DieWithUserMessage("Parameter(s)", "<Schedule Type> <File>");
    }

}
