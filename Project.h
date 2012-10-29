/* 
 * File:   Project.h
 * Author: David Nathan 20356245 Daniel Hunt 20350022
 *
 * Created on October 29, 2012, 5:29 PM
 */

#define MAXVARS 10
#define MAXTIME 1000
#define MAXJOBS 100
#define MAX_JOB_LENGTH 100
#define MAX_PAGES ((MAX_JOB_LENGTH * MAXJOBS + 1)/2)

#define EOL    '\n'
#define CARRIAGE_RETURN    '\r'

typedef struct {
    
    int jobID;
    int start;
    int currentline;
    int length;
    int num_vars;
    char vars[MAXVARS];
    int var_values[MAXVARS];
    char* filename;
	
}JOB;

typedef struct {
    JOB *elements;
    int nElements;
    int maxElements;
}JOBQ;

/* Structure for page information*/
typedef struct {
    /* page number for a given frame */
    int page_number;
    /* First two lines of file that are stored in the page*/
    char *data[2]; //needs to be calloc'd at  runtime
} PAGE;

typedef struct {
	// get the page index of a given line
    int pageIndex[MAX_JOB_LENGTH];
    // get the hdd frame index of a given page
    int hdd_frameIndex[MAX_PAGES];
    
    // the frame of a PAGE in RAM, or -1 if the PAGE is not in RAM
    int RAMFrame[MAX_PAGES];
    // the frame of a PAGE in cache, or -1 if the page is not in cache
    int cacheFrame[MAX_PAGES];


} PAGETABLE;


/* Represents physical memory: RAM, CACHE or HDD*/
typedef struct {
    /*count of frames in this memory*/
    int  num_frames;
    /*time needed to pull frames out of this memory*/
    int accessCost;
    /*array of frames which store a page.*/
    PAGE *frames; //initialised to size at runtime
    /*Least Recently Used ordering so that LRU[0] is the least recently used frame number */
    int *LRU;
    
} MEMORY;

