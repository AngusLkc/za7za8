#include <stdio.h>
#include <memory.h>
#include <malloc.h>
#include <assert.h>

/* @desc     堆结构体
 * @data     数据数组
 * @capacity 数组容量
 * @len      元素数量
 */
typedef struct _tHeap {
    int* data;
    int capacity;
    int len;
} Heap, *pHeap;

/* @desc               创建堆结构体
 * @param[in] capacity 指定数组容量
 * @return             非NULL成功,否则失败
 */
pHeap createHeap(int capacity) {
    pHeap heap = (pHeap)malloc(sizeof(Heap));
    assert(heap);
    assert(capacity > 0);
    memset(heap, 0, sizeof(Heap));
    heap->capacity = capacity;
    heap->data = (int*)malloc(sizeof(heap->data[0]) * heap->capacity);
    assert(heap->data);
    return heap;
}

/* @desc           销毁堆结构体
 * @param[in] heap 需要销毁的结构体指针
 */
void destroyHeap(pHeap heap) {
    if (heap) {
        if (heap->data) {
            free(heap->data);
            heap->data = NULL;
        }
        free(heap);
    }
	return;
}

/* @desc               元素上浮(插入)
 * @param[in] heap     需要操作的堆结构指针
 * @param[in] position 指定要上移的元素
 *  - position         是元素编号,所以对应到数组元素时候需要-1
 */
void shiftUp(pHeap heap, int position) {
    while (position >> 1) {
        int parent = position >> 1;
        /*与父元素进行比较,如果大于父元素则进行交换*/
        if (heap->data[parent - 1] > heap->data[position - 1]) {
            int temp = heap->data[parent - 1];
            heap->data[parent - 1] = heap->data[position - 1];
            heap->data[position - 1] = temp;
            position = parent;
        } else
			break;
    }
}

/* @desc               元素下沉(弹出)
 * @param[in] heap     需要操作的堆结构指针
 * @param[in] position 指定要下沉的元素
 *  - position         是元素编号,所以对应到数组元素时候需要-1
 */
void shiftDown(pHeap heap, int position) {
    while (position << 1 <= heap->len) {
        int target = position << 1;
        /*指定左右孩子中的较小者*/
        if (target + 1 <= heap->len)
            if (heap->data[target] < heap->data[target - 1])
                target += 1;
        /*父节点大于较小者则交换*/
        if (heap->data[position - 1] > heap->data[target - 1]) {
            int temp = heap->data[target - 1];
            heap->data[target - 1] = heap->data[position - 1];
            heap->data[position - 1] = temp;
            position = target;
        } else
			break;
    }
}

/* @desc           插入元素
 * @param[in] heap 需要操作的堆结构指针
 * @param[in] v    需要插入的元素
 */
void insertHeap(pHeap heap, int v) {
    /*满了扩容*/
    if (heap->capacity == heap->len) {
        heap->capacity <<= 1;
        heap->data = (int*)realloc(heap->data, heap->capacity);
        assert(heap->data);
    }
    /*插入到堆尾*/
    heap->data[heap->len++] = v;
    /*调整堆结构,对堆尾元素进行上浮*/
    shiftUp(heap, heap->len);
}

/* @desc           返回堆顶元素
 * @param[in] heap 需要操作的堆结构指针
 * @return         返回堆顶元素
 */
int topHeap(pHeap heap) {
    return heap->data[0];
}

/* @desc           弹出堆顶元素
 * @param[in] heap 需要操作的堆结构指针
 */
void popHeap(pHeap heap) {
    if (heap->len > 0) {
        /*将堆尾元素放入堆顶*/
        heap->data[0] = heap->data[heap->len - 1];
        --heap->len;
        /*调整堆结构,对新的堆顶元素进行下沉*/
        shiftDown(heap, 1);
    }
}

int main() {
    pHeap heap = createHeap(16);
    insertHeap(heap, 3);
    insertHeap(heap, 1);
    insertHeap(heap, 4);
    insertHeap(heap, 7);
    insertHeap(heap, 6);
    insertHeap(heap, -4);
    while (heap->len) {
        int v = topHeap(heap);
        popHeap(heap);
        printf("%d ", v);
    }
    destroyHeap(heap);
    return 0;
}
