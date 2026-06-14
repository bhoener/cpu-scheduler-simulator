#ifndef QUEUE_H
#define QUEUE_H
#include "process.h"
#include "queue_adt.h"
using namespace std;

/**
 * @class Queue
 * Linked list implementation of the Queue ADT
 */
template <typename T>
class Queue : public QueueADT<T>
{
private:
    class ListNode
    {
    public:
        T data;
        ListNode *next;
        ListNode(T data) : data(data), next(nullptr) {};
        ListNode(T data, ListNode *next) : data(data), next(next) {};
    };
    ListNode *head;
    int size;

public:
    Queue();
    ~Queue();

    void add(const T &);

    T remove();

    T peek() const;

    int getSize() const;

    bool isEmpty() const;

    void clear();

    void print(ostream&) const;
};

template class Queue<Process>;

#endif
