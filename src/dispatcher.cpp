#include "dispatcher.h"
#include "queue_adt.h"
#include <iostream>
using namespace std;

/**
 * @brief constructor, takes pointers to ready/waiting queues
 * @param ready_queue the queue of ready programs to pull from
 * @param waiting queue of waiting programs to push to
 */
Dispatcher::Dispatcher(QueueADT<Process> *ready_queue, QueueADT<Process> *waiting_queue) : ready_queue(ready_queue), waiting_queue(waiting_queue)
{
    running_process = Process();
    running_process.terminate();
}
Dispatcher::~Dispatcher() {}

/**
 * @brief swaps to next process in ready queue
 */
void Dispatcher::nextProcess()
{
    if (!(running_process.getState() == ProcessState::TERMINATED))
        running_process.block();
    waiting_queue->add(running_process);
    try
    {
        running_process = ready_queue->remove();
        running_process.schedule();
    }
    catch (out_of_range)
    {
        cout << "No processes in ready queue" << endl;
        running_process = Process();
        running_process.terminate();
    }
}

/**
 * @brief ticks the running process, swaps if finished early
 */
void Dispatcher::step()
{
    running_process.tick();
    if (running_process.isFinished())
        nextProcess();
}

/**
 * @return reference to the current running process
 */
Process& Dispatcher::getRunningProcess()
{
    return running_process;
}

/**
 * @brief stream insertion operator to report status
 */
ostream &operator<<(ostream &os, const Dispatcher &disp)
{
    os << "RUNNING PROCESS:" << endl;
    os << disp.running_process.toString();
    os << endl;
    os << "READY QUEUE:" << endl;
    disp.ready_queue->print(os);
    os << endl;
    os << "WAITING QUEUE" << endl;
    disp.waiting_queue->print(os);
    return os;
}