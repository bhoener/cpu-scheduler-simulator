#include <iostream>
#include "dispatcher.h"
#include "queue_adt.h"
#include "priority_queue.h"
#include "queue.h"
#include "process.h"
#include "scheduler.h"
#include <random>
using namespace std;

int prompt()
{
    int algo;
    cout << "================================================" << endl;
    cout << "Please enter an algorithm to use: " << endl;
    cout << "| ";
    for (Algorithm Algorithm : {Algorithm::FCFS, Algorithm::SJF, Algorithm::PRIORITY, Algorithm::RR})
    {
        cout << (int)Algorithm << ": " << Scheduler::algo_to_string(Algorithm) << " | ";
    }
    cout << " (-1 to quit)" << endl;
    cout << endl;
    cin >> algo;
    return algo;
}

/**
 * @file main.cpp
 * holds the simulation testing logic and benchmarking
 */
int main()
{
    cout << "Welcome to the CPU Scheduler Simulator!" << endl;
    int algo;
    while ((algo = prompt()) != -1)
    {
        srand(42); // keep processes deterministic
        // simulation parameters
        const unsigned int NUM_PROCESSES_TO_ADD = 20;

        // to simulate high distribution of short bursts and small amount of large bursts
        // we have either short bursts or long bursts
        const unsigned short int MAX_LONG_BURST_LENGTH = 50;
        const unsigned short int MAX_SHORT_BURST_LENGTH = 20;
        const unsigned short int LONG_BURST_CHANCE = 10; // the probability of a long burst is 1/LONG_BURST_CHANCE
        const unsigned int STARVATION_TIME = 50;         // number of steps a process can go before being "starved"

        // for priority-based
        const unsigned short int PRIORITY_LEVELS = 10;

        // cap the simulation length
        const unsigned int MAX_SIMULATION_STEPS = 10000;

        // initialize scheduler and queue of simulation processes
        Config scheduler_config;
        scheduler_config.schedulingAlgorithm = (Algorithm)algo;
        scheduler_config.ioServiceTime = 10;
        scheduler_config.quantum = 10;
        scheduler_config.contextSwitchTime = 2;
        Resources scheduler_resources;
        switch (scheduler_config.schedulingAlgorithm)
        {
        case Algorithm::FCFS:
            scheduler_resources.ready_queue = new Queue<Process>();
            break;
        case Algorithm::RR:
            scheduler_resources.ready_queue = new Queue<Process>();
            break;
        case Algorithm::SJF:
            scheduler_resources.ready_queue = new PriorityQueue<Process>();
            break;
        case Algorithm::PRIORITY:
            scheduler_resources.ready_queue = new PriorityQueue<Process>();
            break;
        }
        scheduler_resources.waiting_queue = new Queue<Process>();
        scheduler_resources.processes = new Vector<Process>();
        scheduler_resources.completed = new Vector<Process>();
        scheduler_resources.dispatcher = new Dispatcher(scheduler_resources.ready_queue, scheduler_resources.waiting_queue);
        Scheduler *scheduler = new Scheduler(scheduler_resources, scheduler_config);

        for (int i = 1; i <= NUM_PROCESSES_TO_ADD; i++)
        {
            // simulate whether short or long burst and pick burst length accordingly
            int burst_length = (rand() % LONG_BURST_CHANCE == 0) ? rand() % MAX_LONG_BURST_LENGTH + 1 : rand() % MAX_SHORT_BURST_LENGTH + 1;

            // add process to queue
            Process newProc(i, "process_" + to_string(i), i, burst_length, rand() % PRIORITY_LEVELS);
            newProc.block();
            scheduler_resources.processes->add(newProc);
        }

        // run simulation
        int load_total = 0;
        int steps_taken = 0;
        for (int i = 0; i < MAX_SIMULATION_STEPS; i++)
        {
            if (scheduler->isFinished())
                break;
            scheduler->tick();
            steps_taken++;
            load_total += scheduler_resources.ready_queue->getSize();
            cout << *scheduler << endl;
        }

        int total_context_switches = 0;
        int total_wait_time = 0;
        int max_wait_time = 0;
        int total_turnaround_time = 0;
        int total_starved_processes = 0;
        for (int i = 0; i < scheduler_resources.completed->getSize(); i++)
        {
            Process process = scheduler_resources.completed->get(i);
            total_context_switches += process.getContextSwitches();
            int wait_time = process.getWaitingTime();
            total_wait_time += wait_time;
            if (wait_time > STARVATION_TIME)
                total_starved_processes++;
            total_turnaround_time += process.getTurnaroundTime();
            max_wait_time = (wait_time > max_wait_time) ? wait_time : max_wait_time;
            cout << process << endl;
        }

        cout << "================================================" << endl;
        cout << scheduler->algo_to_string(scheduler->getAlgorithm()) << endl;
        cout << "TIME TAKEN: " << scheduler->getClock() << endl;
        cout << "SIMULATION STEPS: " << steps_taken << endl;
        cout << "AVERAGE WAITING TIME: " << (double)total_wait_time / (double)scheduler_resources.completed->getSize() << endl;
        cout << "AVERAGE TURNAROUND TIME: " << total_turnaround_time / (double)scheduler_resources.completed->getSize() << endl;
        cout << "LONGEST WAITING TIME: " << max_wait_time << endl;
        cout << "NUMBER OF PRE-EMPTIVE CONTEXT SWITCHES: " << total_context_switches << endl;
        cout << "TIME SPENT CONTEXT-SWITCHING: " << total_context_switches * scheduler_config.contextSwitchTime << endl;
        cout << "AVERAGE LOAD: " << (double)load_total / (double)scheduler->getClock() << endl;
        cout << "TOTAL STARVED PROCESSES: " << total_starved_processes << endl;

        delete scheduler_resources.ready_queue;
        delete scheduler_resources.waiting_queue;
        delete scheduler_resources.processes;
        delete scheduler_resources.completed;
        delete scheduler_resources.dispatcher;
        delete scheduler;
    }

    return 0;
}