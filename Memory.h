typedef struct {
	// size of a page, i.e. the number of lines it can hold. Always two in this project.
	int size;
	// an array containing the lines stored in this page, of length 'size'
	char** lines;
} PAGE;

typedef struct {
	// number of frames in this memory object
	int size;
	// the time cost to access from this memory
	int cost;
	// an array of frames of length 'size', to store the contents of this memory
	PAGE* frame;
	// an array of length 'size' to store when each frame was last accessed
	int* lastAccessed;
} MEMORY;

typedef struct {
	// the frame of a PAGE in RAM, or -1 if the PAGE is not in RAM
	int RAMFrame[MAX_PAGES];
	// the frame of a PAGE in cache, or -1 if the page is not in cache
	int cacheFrame[MAX_PAGES];
} PAGE_TABLE;