#include "queue.h"
#include <iostream>
using namespace std;

/**
 * @post constructor, init size and set head to null
 */
template <typename T>
Queue<T>::Queue() : size(0), head(nullptr) {}

/**
 * @post destructor, deallocates memory by removing one by one
 */
template <typename T>
Queue<T>::~Queue() { clear(); }

/**
 * @post inserts a value into the queue
 * @param T the value to insert
 * if empty, make the head a new node
 * otherwise, traverse to end and append
 */
template <typename T>
void Queue<T>::add(const T &val)
{
    if (size == 0)
    {
        head = new ListNode(val);
        size++;
        return;
    }
    ListNode *current = head;

    while (current->next)
    {
        current = current->next;
    }

    current->next = new ListNode(val);
    size++;
}

/**
 * @pre non-empty queue
 * @post removes first item from queue
 * @return the item removed
 * pops off the head and returns it
 * @throws out_of_range if empty
 */
template <typename T>
T Queue<T>::remove()
{
    if (size == 0)
        throw out_of_range("Cannot remove from empty queue");

    T out = head->data;
    ListNode *next = head->next;
    size--;
    delete head;
    head = next;
    return out;
}

/**
 * @pre non-empty queue
 * @post gets first item without removing
 * @return the item
 * @throws out_of_range if empty
 */
template <typename T>
T Queue<T>::peek() const
{
    if (size == 0)
        throw out_of_range("Cannot remove from empty queue");
    return head->data;
}

/**
 * @post gets size of queue
 * accesses the private field for size
 */
template <typename T>
int Queue<T>::getSize() const
{
    return size;
}

/**
 * @post checks if the queue is empty
 */
template <typename T>
bool Queue<T>::isEmpty() const
{
    return size == 0;
}

/**
 * @post removes all elements from queue, one by one
 */
template <typename T>
void Queue<T>::clear()
{
    while (!isEmpty())
        remove();
}