#include <iostream>
#include "dispatcher.h"
#include "queue_adt.h"
#include "priority_queue.h"
#include "queue.h"
#include "process.h"
#include "scheduler.h"
#include <random>
using namespace std;

/**
 * @file main.cpp
 * holds the simulation testing logic and benchmarking
 */
int main()
{
    // simulation parameters
    const unsigned int NUM_PROCESSES_TO_ADD = 100;
    const unsigned int MAX_CONCURRENT_PROCESSES = 15;

    // to simulate high distribution of short bursts and small amount of large bursts
    // we have either short bursts or long bursts
    const unsigned short int MAX_LONG_BURST_LENGTH = 500;
    const unsigned short int MAX_SHORT_BURST_LENGTH = 20;
    const unsigned short int LONG_BURST_CHANCE = 10; // the probability of a long burst is 1/LONG_BURST_CHANCE

    // cap the simulation length
    const unsigned int MAX_SIMULATION_STEPS = 10000;

    // initialize scheduler and queue of simulation processes
    Scheduler *scheduler = new Scheduler(); // new RoundRobin(), new SJF(), etc.
    Queue<Process> *processes_to_add = new Queue<Process>();

    for (int i = 1; i <= NUM_PROCESSES_TO_ADD; i++)
    {
        // simulate whether short or long burst and pick burst length accordingly
        int burst_length = (rand() % LONG_BURST_CHANCE == 0) ? rand() % MAX_LONG_BURST_LENGTH : rand() % MAX_SHORT_BURST_LENGTH;
        
        // add process to queue
        Process newProc(i, "process_" + to_string(i), i, burst_length);
        newProc.block();
        processes_to_add->add(newProc);
    }

    // run simulation, store steps taken (will stop at MAX_SIMULATION_STEPS if not done)
    int steps_taken = MAX_SIMULATION_STEPS;
    for (int i = 0; i < MAX_SIMULATION_STEPS; i++)
    {
        // if all processes are completed, exit early
        if (scheduler->get_process_count == 0 && processes_to_add->isEmpty())
        {
            steps_taken = i + 1;
            break;
        }

        // if there is room, squeeze in another process
        if (scheduler->get_process_count < MAX_CONCURRENT_PROCESSES)
            scheduler->load_process(processes_to_add->remove());

        // tick the simulation and output
        scheduler->step();
        scheduler->log();
    }

    // log statistics
    cout << "==================================" << endl;
    cout << "Processes in simulation: " << NUM_PROCESSES_TO_ADD << endl;
    cout << "Steps taken: " << steps_taken << endl;

    delete scheduler;
    delete processes_to_add;
    return 0;
}