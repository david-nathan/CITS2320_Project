

#define MAXVARS 10
#define MAXJOBS 100
#define MAX_JOB_LENGTH 100
#define MAX_PAGES ((MAX_JOB_LENGTH * MAXJOBS + 1)/2)


typedef struct {
    
	int jobID;
	int start;
    int currentline;
	int length;
    int num_vars;
    char *vars[MAXVARS];
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