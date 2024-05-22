#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

//sortqueue.h
#ifndef _SORTQUEUE_H
#define _SORTQUEUE_H
#include <stdint.h>
#include <stdbool.h>

typedef struct sortqueue {
    uint32_t in;
    uint32_t size;
    void** buffer;
} sortqueue_t;

typedef void (*free_cb)(void *ptr);

sortqueue_t *sq_new(uint32_t size);
bool sq_put(sortqueue_t *queue, uint32_t seq, void* data);
void** sq_get(sortqueue_t *queue, uint32_t *count);
void sq_free(sortqueue_t *queue, free_cb cb);
#endif

static inline uint32_t roundup_pow_of_two(uint32_t x)
{
    uint32_t i, b = 0;
    for(i = 0; i < 32; i++) {
        b = 1UL << i;
        if(x <= b)
            break;
    }
    return b;
}

sortqueue_t *sq_new(uint32_t size)
{
    if (size < 1)
        return NULL;
    sortqueue_t* queue = malloc(sizeof(sortqueue_t));
    if (queue == NULL)
        return NULL;
    if (size & (size - 1))
        queue->size = roundup_pow_of_two(size);
    else
        queue->size = size;
    queue->buffer = calloc(queue->size, sizeof(void*));
    if(queue->buffer == NULL) {
        free(queue);
        return NULL;
    }
    queue->in = 0;
    return queue;
}

bool sq_put(sortqueue_t *queue, uint32_t seq, void* data)
{
    if (seq - queue->in >= queue->size)
        return false;
    uint32_t index = seq & (queue->size - 1);
    if (queue->buffer[index] != NULL)
        return false;
    else
        queue->buffer[index] = data;
    return true;
}

void** sq_get(sortqueue_t *queue, uint32_t *count)
{
    void **ptr = NULL;
    uint32_t once, len;
    for (len = 0; len < queue->size; len++)
        if (queue->buffer[(queue->in + len) & (queue->size - 1)] == NULL)
            break;
    if (len > 0) {
        ptr = malloc(len * sizeof(void*));
        once = MIN(len, queue->size - (queue->in & (queue->size -1)));
        memcpy(ptr, queue->buffer + (queue->in & (queue->size - 1)), once * sizeof(void*));
        memset(queue->buffer + (queue->in & (queue->size - 1)), 0x0, once * sizeof(void*));
        if (len > once) {
            memcpy(ptr + once, queue->buffer, (len - once) * sizeof(void*));
            memset(queue->buffer, 0x0, (len - once) * sizeof(void*));
        }
        queue->in += len;
    }
    *count = len;
    return ptr;
}

void sq_free(sortqueue_t *queue, free_cb cb)
{
    uint32_t i;
    if (queue) {
        if (queue->buffer) {
            for (i = 0; i < queue->size; i++)
                if (queue->buffer[i])
                    cb(queue->buffer[i]);
            free(queue->buffer);
        }
        free(queue);
    }
}
