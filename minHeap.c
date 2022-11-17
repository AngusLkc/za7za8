#include <stdio.h>
#include <memory.h>
#include <malloc.h>
#include <assert.h>

/* 堆顶下标从1开始：父节点等于i/2、左子节点等于2*i、右子节点等于2*i+1 */

typedef struct _tHeap {
    int* data;
    int size;
    int count;
} Heap, *pHeap;

pHeap create(int size) {
	if (size < 2)
		return NULL;
    pHeap heap = (pHeap)malloc(sizeof(Heap));
    assert(heap);
    heap->data = (int*)malloc(sizeof(heap->data[0]) * (heap->size + 1));
    assert(heap->data);
	heap->size = size;
	heap->count = 1;
    return heap;
}

void destroy(pHeap heap) {
    if (heap) {
        if (heap->data)
            free(heap->data);
        free(heap);
    }
}

//上浮
void shiftUp(pHeap heap, int idx, int val) {
	int parent = idx >> 1; //父节点在idx/2位置
    while (parent >= 1) {
        if (heap->data[parent] > val) {
            heap->data[idx] = heap->data[parent];
			idx = parent;
			parent >>= 1; //继续上浮
        }
		else
			break;
    }
	heap->data[idx] = val;
}

//下沉
void shiftDown(pHeap heap, int idx, int val) {
	int child = idx << 1; //左子节点在idx*2位置
    while (child < heap->count) {
        if (child + 1 < heap->count)
            child = heap->data[child] < heap->data[child + 1] ? child : child + 1; //取左右子节点较小值
        if (val > heap->data[child]) {
			heap->data[idx] = heap->data[child];
            idx = child;
			child <<= 1; //继续下沉
        }
		else
			break;
    }
	heap->data[idx] = val;
}

void insert(pHeap heap, int val) {
    if (heap->count > heap->size) {
        heap->size <<= 1; //2倍扩容
        heap->data = (int*)realloc(heap->data, heap->size + 1);
        assert(heap->data);
    }
    shiftUp(heap, heap->count++, val);
}

int top(pHeap heap) {
    return heap->data[1];
}

void pop(pHeap heap) {
    if (heap->count > 0) {
        heap->count--;
        shiftDown(heap, 1, heap->data[heap->count + 1]);
    }
}
