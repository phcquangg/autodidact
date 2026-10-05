#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#define BUFFER_SIZE 8

typedef struct {
	uint8_t buffer[BUFFER_SIZE];
	size_t head;
	size_t tail;
	size_t count;
} circular_buffer;

void cb_init(circular_buffer *cb)
{
	cb->head = 0;
	cb->tail = 0;
	cb->count = 0;
}

bool is_cb_full(const circular_buffer *cb)
{
	return cb->count == BUFFER_SIZE;
}

bool is_cb_empty(const circular_buffer *cb)
{
	return cb->count == 0;
}

bool cb_push(circular_buffer *cb, uint8_t data)
{
	if (is_cb_full(cb)) return false; // buffer is full, write rejected

	cb->buffer[cb->head] = data;

	// modulo works fine, head = 7, +1 =8 %8 = 0
	// but since % requires CPU division (slow), bitwise & takes only one CPU's clock circle.
	// and when BUFFER_SIZE is a power of 2, substraction creates a binary mask of all 1
	// 0111(2) = 7 | 1000(2) = 8 | 1001(2) = 9
	// so when we & any integer with BUFFER_SIZE -1 (this case it's 7), it strips away all higher buts and keeps only the values from 0 -> 7
	cb->head = (cb->head + 1) & (BUFFER_SIZE -1);
	cb->count++;

	return true;
}

bool cb_pop(circular_buffer *cb, uint8_t *data)
{
	if (is_cb_empty(cb)) return false;

	*data =  cb->buffer[cb->tail];
	cb->tail = (cb->tail +1) & (BUFFER_SIZE -1);
	cb->count--;

	return true;
}

int main()
{
	circular_buffer cb;
	cb_init(&cb);

	printf("INIT: head: %ld | tail: %ld | count: %ld\n", cb.head, cb.tail, cb.count);

	cb_push(&cb, 10);
	printf("PUSH#1: head: %ld | tail: %ld | count: %ld\n", cb.head, cb.tail, cb.count);
	cb_push(&cb, 20);
	printf("PUSH#2: head: %ld | tail: %ld | count: %ld\n", cb.head, cb.tail, cb.count);
	cb_push(&cb, 30);
	printf("PUSH#3: head: %ld | tail: %ld | count: %ld\n", cb.head, cb.tail, cb.count);

	uint8_t val;
	while (!is_cb_empty(&cb)) {
		cb_pop(&cb, &val);
		printf("head: %ld | tail: %ld | count: %ld\n", cb.head, cb.tail, cb.count);
		printf("Popped: %d\n", val);
	}

	return 0;
}

