#ifndef QUEUE_ADT_H
#define QUEUE_ADT_H
#include "process.h"
using namespace std;

/**
 * @class QueueADT
 * abstract class for queue, used by Queue and PriorityQueue
 */
template <typename T>
class QueueADT
{
public:
    virtual void add(const T &) = 0;
    virtual T remove() = 0;
    virtual T peek() const = 0;
    virtual int getSize() const = 0;
    virtual bool isEmpty() const = 0;
    virtual void clear() = 0;
};

template class QueueADT<int>;
template class QueueADT<Process>;

#endif