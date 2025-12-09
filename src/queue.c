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
	// if (q == NULL || proc == NULL) {
    //     return;
    // }

    if (q->size >= MAX_QUEUE_SIZE) {
        // queue is full
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

	if (empty(q)) {
        return NULL;
    }

#ifdef MLQ_SCHED
	struct pcb_t *proc = q->proc[0];

    for (int i = 1; i < q->size; i++) {
        q->proc[i - 1] = q->proc[i];
    }
    q->proc[q->size - 1] = NULL; // them
    q->size--;

    return proc;

#else
    if (empty(q)) {
        return NULL;
    }

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

    // if (empty(q)) return NULL;

    // int highest_priority_index = 0;

    // for (int i = 1; i < q->size; i++) {
    //     if (q->proc[i]->priority > q->proc[highest_priority_index]->priority) {
    //             highest_priority_index = i;
    //     }
    // }

    // struct pcb_t * highest_priority_proc = q->proc[highest_priority_index];

    // for (int i = highest_priority_index; i < q->size - 1; i++) {
    //         q->proc[i] = q->proc[i + 1];
    // }

    // q->proc[q->size - 1] = NULL;
    // q->size--;

    // return highest_priority_proc;
}

struct pcb_t *purgequeue(struct queue_t *q, struct pcb_t *proc)
{
	/* TODO: remove a specific item from queue
	 */
	
	if (proc == NULL || empty(q)) {
		return NULL;
	}

#ifdef MLQ_SCHED
	// int idx = -1;
    // for (int i = 0; i < q->size; i++) {
    //     if (q->proc[i] == proc) {
    //         idx = i;
    //         break;
    //     }
    // }

	// // Not found
    // if (idx == -1)
    //     return NULL;

    // for (int i = idx; i < q->size - 1; i++) {
    //     q->proc[i] = q->proc[i + 1];
    // }
	
    // q->proc[q->size - 1] = NULL;
    // q->size--;

    // return proc;

    for (int i = 0; i < q->size; i++)
    {
        if (q->proc[i] == proc)
        {
            for (int j = i; j < q->size - 1; j++)
            {
                q->proc[j] = q->proc[j + 1];
            }
            q->size--;
            return NULL;
        }
    }
#endif
}