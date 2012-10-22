

#define MAXVARS 10
#define MAXJOBS 10


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
