#include <stdio.h>
#include <stdlib.h>
#include <string.h>

	typedef struct {
		char name;
		int value;
	} VAR;
	
	typedef struct {
		int start;
		int streamPos;
		int length;
		int* queueTime;
		int* runTime;
		VAR* vars;
	} JOB;
	
	JOB readJob(char *name) {
		// open job file
		FILE *f = fopen(name, "rt");
		
		JOB newJob;
		int start;
		char *line;
		// read in the first line of the job file
		if( fgets(line, 20, f) != NULL ) {
			 sscanf(line, "%d", &start);
		} else {
			fprintf(stderr, "Job with name %s is an empty job.\n", name);
		}
		
		newJob.start = start;
		newJob.streamPos = 0;
		
		// workaround to quickly calculate the number of lines in the file
		int c;
		int count = 0;
		while ( (c=fgetc(f)) != EOF ) {
			if ( c == '\n' )
				count++;
		}
		
		// update the length parameter, ignoring the first line of the job file
		newJob.length = count-1;
		
		// close file
		fclose(f);
		
		return newJob;
	}
	
	int main(int argc, int argv[]) {
		JOB j = readJob("job1.txt");
		printf("start: %d, length: %d", j.start, j.length);
	
	
	}
