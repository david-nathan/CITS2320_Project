#include <stdio.h>
#include <stdlib.h>
#include <string.h>
	

	typedef struct {
		int[] queueTime;
		int[] runTime;
		int start;
		int streamPos;
		int length;
		VAR[] vars;
	} JOB;
	
	typdef struct {
		char name;
		int value;
	} VAR;
	
	JOB readJob(char *name) {
		// open job file
		FILE *f = fopen(name, "rt");
		
		int start;
		char *line;
		// read in the first line of the job file
		if( fgets(line, 20, f) != NULL ) {
			 sscanf(line, "%d", &start);
		} else {
			fprintf(stderr, "Job with name %s is an empty job.\n", name);
			return NULL;
		}
		
		// workaround to quickly calculate the number of lines in the file
		int c;
		int count = 0;
		while ( (c=fgetc(f)) != EOF ) {
			if ( c == '\n' )
				count++;
		}
		
		// close file
		fclose(f);
		
		return start;
	}
