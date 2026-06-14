#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H
#include "process.h"
#include "queue_adt.h"
using namespace std;

/**
 * @class PriorityQueue
 * Heap implementation of a queue
 * NOTE: behaves differently from a regular queue
 */
template <typename T>
class PriorityQueue : public QueueADT<T>
{
private:
    int size, capacity;
    T *data;
    static const unsigned int DEFAULT_CAPACITY = 25;
    void reallocate(int);
    void reheap(int);

public:
    PriorityQueue();
    PriorityQueue(int);
    ~PriorityQueue();
    void add(const T &);
    T remove();
    T peek() const;
    int getSize() const;
    bool isEmpty() const;
    void clear();

    void print(ostream&) const;
};

template class PriorityQueue<Process>;

#endif