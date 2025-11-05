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
	q->size--;
	return proc;

#else
	if (empty(q))
		return NULL;

	int best_priority_index = 0;
	// to be fixed later
	for (int i = 1; i < q->size; i++) {
		if (q->proc[i]->priority < q->proc[best_priority_index]->priority) {
			best_priority_index = i;
		}
	}

	struct pcb_t *best_priority_proc = q->proc[best_priority_index];

	// shift
	for (int i = best_priority_index; i < q->size - 1; i++) {
		q->proc[i] = q->proc[i + 1];
	}
	q->proc[q->size - 1] = NULL;
	q->size--;

	return best_priority_proc;

#endif	 
	return NULL;
}

struct pcb_t *purgequeue(struct queue_t *q, struct pcb_t *proc)
{
	/* TODO: remove a specific item from queue
	 */
	
	if (q == NULL || proc == NULL || q->size == 0) {
		return NULL;
	}

	// Tìm chỉ số của process trong queue
	int index_to_remove = -1;
	for (int i = 0; i < q->size; i++) {
		if (q->proc[i] == proc) {
			index_to_remove = i;
			break;
		}
	}

	// Nếu không tìm thấy phần tử cần xóa
	if (index_to_remove == -1) {
		return NULL;
	}

	// Di chuyển các phần tử còn lại để lấp đầy khoảng trống
	for (int i = index_to_remove; i < q->size - 1; i++) {
		q->proc[i] = q->proc[i + 1];
	}

	// Đặt phần tử cuối cùng về NULL để tránh truy cập không hợp lệ
	q->proc[q->size - 1] = NULL;

	// Giảm kích thước của queue
	q->size--;

	// Trả về phần tử đã bị xóa (tùy chọn)
	return proc; 
}