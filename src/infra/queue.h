// queue.h

#ifndef __QUEUE_H_
#define	__QUEUE_H_

struct _queue_element;

typedef void(*dtor)(void* e);
typedef void*(*clone)(void* e);

typedef struct _queue {
	struct _queue_element* first;
	struct _queue_element* last;
	long length;
	dtor fd;
	clone fc;
} queue;

queue* queue_create(dtor fd, clone fc);
void queue_destroy(queue* q);

long queue_length(queue* q);

void queue_pushhead(queue* q, void* e);
void* queue_pophead(queue* q);
void* queue_peekhead(queue* q);
void queue_pushtail(queue* q, void* e);
void* queue_poptail(queue* q);
void* queue_peertail(queue* q);

void queue_enum(queue* q, int(*f)(void* e, void* ctx), void* ctx);
void queue_select(queue* q, int(*condition)(void* e, void* ctx), void* ctxi, queue** pqueue);


#endif


