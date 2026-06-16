#ifndef SCHEDULER_H
#define SCHEDULER_H

/**
 * @file scheduler.h
 * @brief ADT for the discrete-time CPU scheduling simulation.
 *
 * The scheduler manages the two queues and the dispatcher. It does not own them.
 * Instead, it operates on Process objects passed as a pointer by the OS at construction. 
 * It is the single authoritative entry point for running a workload through the 
 * supported algorithms, handling the ready/waiting queues and the dispatcher 
 * according to the active algorithm's logic.
 *
 * Lifecycle
 * ---------
 *  1. Construct: provide the necessary resources and configure parameters
 *  2. run(): drive the simulation to completion; OR call tick() manually.
 *  3. metrics will be available in the completed vector for inspection after completion.
 *  5. reset() – clear runtime state so the same workload can be re-run with
 *     a different algorithm without re-loading processes.
 *
 * Each simulation tick does three things in order:
 *  a) schedule()  – admit newly-arrived processes into the ready queue and
 *                   apply the chosen algorithm's ordering.
 *  b) execute()   – let the Dispatcher perform a context switch if needed,
 *                   then advance the running process by one tick.
 *  c) process()   – scan the waiting queue and re-admit processes whose I/O
 *                   (or other blocking event) has resolved.
 *
 * Algorithm
 * ------------------------------------
 *  Algo::FCFS classic FCFS (non-preemptive)
 *  Algo::SJF  SJF (non-preemptive)
 *  Algo::PRIORITY priority (non-preemptive)
 *  Algo::PRIORITY priority (non-preemptive)
 *  Algo::RR Round Robin (quantum driven; preemptive by nature)
 * 
 * Note: construction of the scheduler requires the OS to provide the correct 
 * concrete queue types for the active algorithm: 
 * FCFS or RR requires a Queue<Process>, while; 
 * SJF or PRIORITY requires a PriorityQueue<Process>
 * 
 */

#include "process.h"
#include "queue_adt.h"
#include "queue.h"
#include "priority_queue.h"
#include "dispatcher.h"

#include <vector>
#include <iostream>
#include <string>


enum class Algorithm
{
    FCFS,     // First-Come, First-Served  (ordered by arrival time)
    SJF,      // Shortest-Job-First        (ordered by burst / remaining time)
    PRIORITY, // Priority scheduling       (ordered by priority value)
    RR        // Round Robin               (time-quantum based)
};

// Resources managed by the Scheduler, given to it by the OS at construction
struct Resources
{
    // The concrete ready queue type depends on the active algorithm: 
    // FCFS or RR requires a Queue<Process>, while; 
    // SJF or PRIORITY requires a PriorityQueue<Process>
    QueueADT<Process> *ready_queue; 
    
    Queue<Process> *waiting_queue;

    // All processes to be scheduled, in pristine form (not mutated by the simulation)
    // schedule() admits from it to ready_queue when clock >= arrivalTime
    vector<Process>* processes; 

    // Terminated processes, in finish order
    // schedule() / execute() move processes here when they complete
    // benchmarking / metrics methods read from this vector to compute results
    vector<Process>* completed;
    
    Dispatcher *dispatcher;
};

// congiguration parameters for the Scheduler, set at construction or via setters before run()
struct Config
{
    Algorithm schedulingAlgorithm = Algorithm::FCFS;
    int quantum = 10;      // RR time-slice length in ticks
    int ioServiceTime = 2; // Ticks a process stays in waiting before re-admit
};

// Scheduler
class Scheduler
{
private:
    // --- resources ---
    QueueADT<Process> *ready_queue;
    Queue<Process> *waiting_queue;
    vector<Process> *processes;
    vector<Process> *completed;
    Dispatcher *dispatcher;

    // --- configuration ---
    Algorithm schedulingAlgorithm;
    int quantum;
    int ioServiceTime;

    // --- simulation state ---
    int clock;     // Current simulation tick (starts at 0)
    int rrElapsed; // Ticks the current process has used

    // Tracks whether each process has been admitted from the processes vector
    // so we can detect firstRunTime(response time) && avoid duplicate admission
    std::vector<std::pair<int, bool>> admissionMeta; // { process_id, bool hasBeenAdmitted }

    // Tracks when each process entered the waiting queue so ioServiceTime
    // expiry can be detected without storing extra state inside Process.
    std::vector<std::pair<int, int>> waitingMeta; // { process_id, tick_it_entered_waiting }
    
   
    // --- PRIVATE METHODS && PHASES ---
    /**
     * @brief Admit newly-arrived processes from the processes vector into the ready queue
     * With the given algorithm-specific ordering */
    void schedule();

    /**
     * @brief Process the waiting queue: re-admit processes whose I/O service
     *        time has elapsed back into the ready queue as READY. 
     *  
     *         Implementation not complete. But, not required either as IO bursts
     *         and waiting_queue functionality are not implemented.
     * */
    void process();

    /**
     * @brief Perform context switch when the running process has finished; 
     * Also advances the runner by one tick. When the process finishes, 
     * it is moved to the completed vector with its completion time set.
     *
     * For Round Robin: increments rrElapsed and triggers a preemption
     * when rrElapsed == quantum. */
    void execute();

    // --- HELPER METHODS ---
    /** 
     * @brief Perform a premptive context switch 
     * pushing the runner back into the ready_queue */
    void prempt();

    /** @brief Returns true if there is a real running process
     *         (i.e. pid != -1 and state == RUNNING). */
    bool hasRunningProcess() const;

    /** @brief Admit a single process into the ready queue after setting the
     *        correct compareBy field for the active algorithm. */
    void admitToReady(Process& p);

    /** @brief Marks a process as having run for the first time. */
    void markFirstRun(Process& p);

     /** @brief Returns true if this process has been admitted from the process vector */
    bool admitted(int pid);
    
    /** @brief returns the process if found in the waiting queue 
     *  This function is not implemented because it implementation requires
     *  traversal of a Queue ADT which is not provided.
     *  Fortunately, this functionality is not required as IO Bursts are not implemented.
     *  All process exists only within the dispatcher or the ready queue */
    Process getWaitingProcess(int pid);

    /** @brief Maps algorithms to the corresponding CompareBy 
     *  field for Process ordering in the ready queue. */
    CompareBy toCompareBy(Algorithm algorithm);
public:
     // --- Construction / destruction ---

    /**
     * @brief Full constructor.
     * @param resources      All resources the Scheduler needs to operate, given by the OS
     * @param config         Configuration parameters for the Scheduler,
     *                       set at construction or via setters before run();
     */
    Scheduler(Resources resources, Config config);
    ~Scheduler();


    // --- Simulation control ---
    /**
     * @brief Advance the simulation by exactly one clock tick.
     *        Order: schedule() → execute() → process()
     * @return true  while at least one process is still unfinished.
     * @return false when every process from the processes vector has TERMINATED. */
    bool tick();
    
    /**
     * @brief Run the simulation to completion (calls tick() in a loop).
     * @param maxTicks Safety ceiling; throws std::runtime_error if exceeded.
     *                 Defaults to 100 000 ticks. */
    void run(int maxTicks = 100000);

    // --- Configuration setters (must be called before run()) ---

    /** @throws std::invalid_argument if the algorithm doesn't match the ready queue type
     *          Queue for FCFS/RR, PriorityQueue for SJF/PRIORITY) */
    void setQuantum    (int  q)   { quantum = q;                }
    void setIOServiceTime(int t)  { ioServiceTime = t;          }
    void setAlgorithm  (Algorithm algorithm)   {
       if ((algorithm == Algorithm::FCFS || algorithm == Algorithm::RR) && dynamic_cast<Queue<Process>*>(ready_queue) == nullptr)
            throw std::invalid_argument("Algorithm " + algo_to_string(algorithm) + " requires a Queue<Process> ready queue.");
        
        if ((algorithm == Algorithm::SJF || algorithm == Algorithm::PRIORITY) && dynamic_cast<PriorityQueue<Process>*>(ready_queue) == nullptr)
            throw std::invalid_argument("Algorithm " + algo_to_string(algorithm) + " requires a PriorityQueue<Process> ready queue.");
    
        schedulingAlgorithm = algorithm;
    }

    // --- getters ---
    Algorithm getAlgorithm()    const { return schedulingAlgorithm; }
    int  getQuantum()      const { return quantum;             }
    int  getClock()        const { return clock;               }
    
    // determines if all processes have completed by comparing the 
    // size of the completed vector to the size of the processes vector
    bool isFinished() const {
        return (int)completed->size() == (int)processes->size();
    }

    // --- Reset ---
    /** @brief Clear all runtime state so the same processes vector can be
     *         re-run with a different algorithm. Does NOT clear the vector. 
     *          
     *         Note: clears completed processes. Invoke after bench marking. */
    void reset();

    // --- Stream insertion ---

    /** Prints clock, dispatcher status, and queue sizes. */
    friend std::ostream& operator<<(std::ostream& os, const Scheduler& s);

    // ── Static utility ───────────────────────────────────────────────────────
    static std::string algo_to_string(Algorithm algorithm);

};

#endif // SCHEDULER_H