#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t *q)
{
	if (q == NULL) {
		return 1;
	}
	return (q->size == 0);
}

void enqueue(struct queue_t *q, struct pcb_t *proc)
{
	/* TODO: put a new process to queue [q] */
	if (q == NULL || proc == NULL || q->size >= MAX_QUEUE_SIZE) {
		return;
	}

	q->proc[q->size] = proc;
	q->size++;
}

struct pcb_t *dequeue(struct queue_t *q)
{
	/* TODO: return a pcb whose prioprity is the highest
	 * in the queue [q] and remember to remove it from q
	 */

	if (q == NULL || q->size == 0)
		return NULL;

#ifdef MLQ_SCHED
	struct pcb_t *proc = q->proc[0];

    for (int i = 1; i < q->size; i++) {
        q->proc[i - 1] = q->proc[i];
    }
    q->proc[q->size - 1] = NULL;
    q->size--;

    return proc;

#else
	int best_idx = 0;

    for (int i = 1; i < q->size; i++) {
        if (q->proc[i]->prio < q->proc[best_idx]->prio) {
            best_idx = i;
        }
    }

    struct pcb_t *best = q->proc[best_idx];

    for (int i = best_idx; i < q->size - 1; i++) {
        q->proc[i] = q->proc[i + 1];
    }
    q->proc[q->size - 1] = NULL;
    q->size--;

    return best;
#endif
}

struct pcb_t *purgequeue(struct queue_t *q, struct pcb_t *proc)
{
	/* TODO: remove a specific item from queue
	 */
	
	if (q == NULL || proc == NULL || q->size == 0) {
		return NULL;
	}

	int idx = -1;
    for (int i = 0; i < q->size; i++) {
        if (q->proc[i] == proc) {
            idx = i;
            break;
        }
    }

	// Not found
    if (idx == -1)
        return NULL;

    for (int i = idx; i < q->size - 1; i++) {
        q->proc[i] = q->proc[i + 1];
    }
	
    q->proc[q->size - 1] = NULL;
    q->size--;

    return proc;
}