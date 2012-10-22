#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <stdbool.h>
#include <ctype.h>

#include "Project.h"

/*******JOB QUEUE*******/

JOBQ *newJOBQ(int size) {
    JOBQ *q = malloc(sizeof (JOBQ));
    q->nElements = 0;
    q->maxElements = size;
    q->elements = calloc(size, sizeof (JOB));
    return q;
}

bool isFullJOBQ(JOBQ *q) {
    return (q->maxElements == q->nElements);
}

void enqueueJOBQ(JOB element, JOBQ *q) {
    if (isFullJOBQ(q)) {
        fprintf(stderr, "Trying to enqueue to full JOBQ\n");
        exit(EXIT_FAILURE);
    }
    q->elements[q->nElements] = element;
    q->nElements = q->nElements + 1;
}

bool isEmptyJOBQ(JOBQ *q) {
    return (q->nElements == 0);
}

JOB dequeueJOBQ(JOBQ *q) {
    if (isEmptyJOBQ(q)) {
        fprintf(stderr, "Trying to dequeue from empty JOBSQ\n");
        exit(EXIT_FAILURE);
    }
    JOB element = q->elements[0];
    q->nElements = q->nElements - 1;
    //move down all elements one index
    for (int i = 0; i < q->nElements; i++) {
        q->elements[i] = q->elements[i + 1];
    }
    return element;
}

JOB peekJOBQ(JOBQ *q) {
    if (isEmptyJOBQ(q)) {
        fprintf(stderr, "Trying to peek at empty JOBSQ\n");
        exit(EXIT_FAILURE);
    }
    return q->elements[0];
}

void deleteJOBQ(JOBQ *q) {
    free(q->elements);
}

/* Compares the start times of two JOBs, *j1 and *j2.
 * Returns a positive number if the start time of j1 is greater than that of j2,
 * zero if they are equal, and negative otherwise.
 */
int compareStartTimes(const void *j1, const void *j2) {
    return ((JOB *) j1)->start - ((JOB *) j2)->start;
}

/*
 * Orders the supplied JOBQ in ascending order, according to the start time
 * of each JOB in the queue.
 */
void sortJOBQ(JOBQ *q) {
    qsort(q->elements, q->nElements, sizeof (JOB), compareStartTimes);
}
