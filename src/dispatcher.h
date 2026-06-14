#ifndef DISPATCHER_H
#define DISPATCHER_H
#include "process.h"
#include "queue_adt.h"
#include "queue.h"
#include "priority_queue.h"

/**
 * @class Dispatcher
 * @brief manages the current running process w/ ready and waiting queues
 * when a process finishes/is swapped, a new process is popped from
 * the ready queue and placed into the running process slot
 * the old running process goes back into the waiting queue
 */
class Dispatcher
{
private:
    Process running_process;
    QueueADT<Process> *ready_queue;
    QueueADT<Process> *waiting_queue;

public:
    /**
     * @brief constructor, takes pointers to ready/waiting queues
     * @param ready_queue the queue of ready programs to pull from
     * @param waiting queue of waiting programs to push to
     */
    Dispatcher(QueueADT<Process> *ready_queue, QueueADT<Process> *waiting_queue);
    ~Dispatcher();

    /**
     * @brief swaps to next process in ready queue
     */
    void nextProcess();

    /**
     * @brief ticks the running process, swaps if finished early
     */
    void step();

    /**
     * @return pointer to the current running process
     */
    Process getRunningProcess() const;

    friend
    ostream& operator <<(ostream&, const Dispatcher&);
};

#endif