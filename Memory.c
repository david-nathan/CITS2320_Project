

typedef struct {
	LINK *front;
	LINk *back;
	// number of jobs that can be enqueued
	int size;
} JOB_QUEUE;

typedef struct {
	JOB *job;
	LINK *next;
} LINK;

JOB_QUEUE *createQueue(int size) {
	JOB_QUEUE *q = malloc(sizeof(JOB_QUEUE));
	q->front = calloc(size, sizeof(LINK));
	q->back = NULL;
	q->size = size;
}

bool isEmpty(JOB_QUEUE *q) {
	return q->size==0;
}

void enqueue(JOB_QUEUE *q, JOB *j) {
	LINK *temp = malloc(sizeof(LINK));
	temp->job = j;
	temp->next = q->back;
	q->back = temp;
}

JOB *dequeue(JOB_QUEUE *q) {
	if(!isEmpty(q)) {
		JOB *j = q->front->job;
		q->front = front->next;
		return j;
	} else {
		fprintf(stderr,"Dequeueing from an empty queue.");
		return NULL;
	}
}

JOB *peek(JOB_QUEUE *q) {
	if(!isEmpty(q)) {
		return q->front->job;
	} else {
		fprintf(stderr,"Peeking at an empty queue.");
		return NULL;
	}
}