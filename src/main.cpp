#include <iostream>
#include "dispatcher.h"
#include "queue_adt.h"
#include "priority_queue.h"
#include "queue.h"
#include "process.h"
#include <random>
using namespace std;

int main()
{
    // temporary test for dispatcher, can be removed if needed
    QueueADT<Process> * ready_queue = new PriorityQueue<Process>();
    QueueADT<Process> * waiting_queue = new PriorityQueue<Process>();
    Dispatcher myDispatcher(ready_queue, waiting_queue);

    for (int i = 1; i < 8; i++) {
        Process newProc(i, "process" + to_string(i), i, rand() % 10 + 1);
        newProc.admit();
        ready_queue->add(newProc);
    }

    for (int i = 9; i < 20; i++) {
        Process newProc(i, "process" + to_string(i), i, rand() % 10 + 1);
        newProc.block();
        waiting_queue->add(newProc);
    }

    for (int i = 0; i < 50; i++) {
        myDispatcher.step();
        cout << myDispatcher.getRunningProcess().toString() << endl;
    }
    

    delete ready_queue;
    delete waiting_queue;
    return 0;
}