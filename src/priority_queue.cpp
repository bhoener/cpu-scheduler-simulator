#include "priority_queue.h"
#include <cstring>
#include <iostream>
using namespace std;

/**
 * @post constructor taking capacity as an argument
 * allocates an array in heap memory for the data
 */
template <typename T>
PriorityQueue<T>::PriorityQueue(int cap)
{
    size = 0;
    capacity = cap;
    data = new T[cap]();
}

/**
 * @post empty constructor
 * uses DEFAULT_CAPACITY static constant of the class
 * to set capacity
 */
template <typename T>
PriorityQueue<T>::PriorityQueue() : PriorityQueue(DEFAULT_CAPACITY) {};

/**
 * @post destructor
 * deallocates the memory used to store data
 */
template <typename T>
PriorityQueue<T>::~PriorityQueue()
{
    delete[] data;
}

/**
 * @post resizes the array used to store the heap
 * @param newSize the new size to allocate
 */
template <typename T>
void PriorityQueue<T>::reallocate(int newSize)
{
    T *newData = new T[newSize]();
    memcpy(newData, data, newSize * sizeof(T));
    delete[] data;
    data = newData;
    capacity = newSize;
}

/**
 * @post inserts an entry into the heap
 * @param entry the entry to insert
 * checks & reallocates if more capacity is needed
 * then repeatedly moves parents down if less than entry
 * and inserts the entry in the opened spot
 */
template <typename T>
void PriorityQueue<T>::add(const T &entry)
{
    if (size >= capacity - 1)
        reallocate(capacity * 2);

    int idx = ++size;
    int parent = idx / 2;

    while (parent > 0 && entry < data[parent - 1])
    {
        data[idx - 1] = data[parent - 1];
        idx = parent;
        parent = idx / 2;
    }

    data[idx - 1] = entry;
}

/**
 * @post turns back into valid heap
 * @param root the node to start from (index starts at 1)
 * store the old root for later, then repeatedly move the min child up
 * until no longer less than orphan
 * then insert old root
 */
template <typename T>
void PriorityQueue<T>::reheap(int root)
{
    cout << "before:";
    for (int i = 0; i < size; i++)
        cout << data[i] << " ";
    cout << endl;
    T orphan = data[root - 1];
    int minChild = (data[root * 2 - 1] < data[root * 2]) ? root * 2 - 1 : root * 2;
    while (minChild <= size)
    {
        if (data[minChild] < orphan)
        {
            data[root - 1] = data[minChild];
            root = minChild + 1;
            minChild = (data[root * 2 - 1] < data[root * 2]) ? root * 2 - 1 : root * 2;
        }
        else
        {
            break;
        }
    }

    data[root - 1] = orphan;
}

/**
 * @pre non-empty heap
 * @post remove top item from heap
 * @return the value removed
 * takes first element, swaps with last
 * @throws out_of_range if empty
 */
template <typename T>
T PriorityQueue<T>::remove()
{
    if (isEmpty())
        throw out_of_range("Cannot remove from empty heap");

    T root = data[0];
    data[0] = data[--size];
    reheap(1);
    return root;
}

/**
 * @pre non-empty heap
 * @post peeks first element without removing
 * @return the element
 * @throws out_of_range if empty
 */
template <typename T>
T PriorityQueue<T>::peek() const
{
    if (isEmpty())
        throw out_of_range("Cannot get from empty heap");
    return data[0];
}

/**
 * @post getter method for size
 * @return the size private field
 */
template <typename T>
int PriorityQueue<T>::getSize() const
{
    return size;
}

/**
 * @post checks if heap is empty
 * @return whether empty
 */
template <typename T>
bool PriorityQueue<T>::isEmpty() const
{
    return size == 0;
}

/**
 * @post clears the heap
 * NOTE: doesn't deallocate memory
 */
template <typename T>
void PriorityQueue<T>::clear()
{
    size = 0;
}