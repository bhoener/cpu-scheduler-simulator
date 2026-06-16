/**
 * @file scheduler.cpp
 * @brief Implementation of the Scheduler ADT.
 *
 * Dispatcher integration contract:
 * ─────────────────────────────────
 * The Dispatcher holds the running process as a VALUE copy internally.
 * - dispatcher->step()        ticks the runner. If it finishes, calls nextProcess().
 * - dispatcher->nextProcess() pushes the current runner to waiting_queue,
 *                             pulls next from ready_queue, sets it RUNNING.
 * - dispatcher->getRunningProcess() returns a COPY – mutations don't persist.
 *
 * Because nextProcess() unconditionally pushes to waiting_queue, we cannot
 * use it. Instead, for RR quantum expiry we:
 *   1. Remove the current runner from the Dispatcher via a sentinel swap trick:
 *      we put the preempted process back into the ready_queue first, then call
 *      nextProcess() so the Dispatcher pulls a fresh one and pushes the sentinel.
 *   2. Never use Dispatcher::waiting_queue for re-admission tracking; instead
 *      waitingMeta handles all I/O-blocked processes separately.
 *
 * Process pool tracking:
 * ──────────────────────
 * processes vector holds the canonical copy of each process, it's not modified.
 * This allows reset() to restore every process to its original state.
 * All state && simulation updates happen on the copies in the queues and dispatcher.
 * Completion is recorded by PID lookup when the process finishes, stored in
 * the completed vector for benchmarking.
 */

#include "scheduler.h"
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>

using namespace std;

// --- construction / destruction ---
Scheduler::Scheduler(Resources resources, Config config)
    : ready_queue(resources.ready_queue),
      waiting_queue(resources.waiting_queue),
      processes(resources.processes),
      completed(resources.completed),
      dispatcher(resources.dispatcher),
      schedulingAlgorithm(config.schedulingAlgorithm),
      quantum(config.quantum),
      ioServiceTime(config.ioServiceTime),
      clock(0),
      rrElapsed(0)
{
    // Validate that the provided queues match the algorithm's requirements
    if ((schedulingAlgorithm == Algorithm::FCFS || schedulingAlgorithm == Algorithm::RR) && dynamic_cast<Queue<Process> *>(ready_queue) == nullptr)
        throw std::invalid_argument("Algorithm " + algo_to_string(schedulingAlgorithm) + " requires a Queue<Process> ready queue.");

    if ((schedulingAlgorithm == Algorithm::SJF || schedulingAlgorithm == Algorithm::PRIORITY) && dynamic_cast<PriorityQueue<Process> *>(ready_queue) == nullptr)
        throw std::invalid_argument("Algorithm " + algo_to_string(schedulingAlgorithm) + " requires a PriorityQueue<Process> ready queue.");
}

Scheduler::~Scheduler()
{
    // The Scheduler does not own the queues or dispatcher, so it should not delete them.
    // It also does not own the processes or completed vectors, so it should not delete them.
    
    // to avoid dangling pointers. 
    // Must be invoked only after all resources are deleted by the OS
    ready_queue = nullptr;
    waiting_queue = nullptr;
    completed = nullptr;
    dispatcher = nullptr;
}

// --- simulation control ---
bool Scheduler::tick()
{
    if (isFinished())
        return false;

    schedule(); // Phase 1 – admit new arrivals
    execute();  // Phase 2 – context switch or advance runner
    process();  // Phase 3 – re-admit waiting processes

    ++clock;
    return !isFinished();
}

void Scheduler::run(int maxTicks)
{
    for (int t = 0; t < maxTicks; ++t)
        if (!tick())
            return;

    throw runtime_error(
        "Scheduler::run() exceeded " + to_string(maxTicks) + " ticks.");
}

// --- PRIVATE METHODS && PHASES --- ---
void Scheduler::schedule()
{
    // Admit pool processes that have arrived and haven't been admitted yet.
    // We mark admitted processes in the pool by transitioning them from NEW→READY.
    for (auto &p : *processes)
    {
        if (!admitted(p.getPID()) && p.getArrivalTime() <= clock)
        {
            // push a copy into the ready queue && begin tracking
            Process copy = p;
            copy.admit();
            admitToReady(p);

            // track that it's been admitted
            admissionMeta.push_back({p.getPID(), true});
        }
    }
}

void Scheduler::execute()
{
    // --- No runner: start first available process ---
    if (!hasRunningProcess())
    {
        if (!ready_queue->isEmpty())
        {
            dispatcher->nextProcess(); // pops ready -> RUNNING; pushes sentinel to waiting
            rrElapsed = 0;
            markFirstRun(dispatcher->getRunningProcess());
        }

        return;
    }

    // --- Round Robin quantum expiry ---
    if (schedulingAlgorithm == Algorithm::RR)
    {
        ++rrElapsed;
        if (rrElapsed >= quantum && !ready_queue->isEmpty())
        {
            prempt();
            return;
        }
    }

    // --- Normal tick ---
    // Capture the runner before step(), so we can detect completion.
    Process runner = dispatcher->getRunningProcess();
    dispatcher->step(); // ticks internal runner; auto-calls nextProcess() if done

    Process afterRunner = dispatcher->getRunningProcess();

    // Completion: the PID changed (dispatcher step called nextProcess internally)
    if (afterRunner.getPID() != runner.getPID())
    {
        // The process that was running just finished
        // record the process as completed
        runner.setCompletionTime(clock + 1);
        runner.terminate();
        completed->push_back(runner);

        // If a new process started, record its first-run time
        markFirstRun(afterRunner);
        rrElapsed = 0;
    }
}

// see header for important information
void Scheduler::process()
{
    vector<pair<int, int>> stillWaiting;

    for (auto &[process_id, tick_it_entered_waiting] : waitingMeta)
    {
        if (clock - tick_it_entered_waiting >= ioServiceTime)
        {
            Process p = getWaitingProcess(process_id);
            p.unblock();
            admitToReady(p);
        }
        else
        {
            stillWaiting.emplace_back(process_id, tick_it_entered_waiting);
        }
    }

    waitingMeta = move(stillWaiting);
}

// --- HELPER METHODS ---

void Scheduler::prempt() {
    // Get the current runner's state before we displace it
    Process evicted = dispatcher->getRunningProcess();

    // We need to put evicted back into the ready queue with updated remainingTime.
    // The problem: Dispatcher::nextProcess() pushes the runner to waiting_queue.
    // Solution:
    //   1. Push the evicted process (with correct remainingTime) into ready_queue.
    //      It will end up behind the incoming process in PQ ordering (or at tail in FIFO).
    //   2. Call dispatcher->nextProcess() – it will push the old runner to waiting_queue
    //      (we discard that copy) and pull the correct next process from ready_queue.
    //   3. The ready_queue entry we just added IS the evicted process, so the next
    //      pop from the other side will eventually pull it.
    //
    // This works because:
    //   - For FIFO (RR): evicted goes to tail, incoming processes ahead of it
    //   - For PriorityQueue: evicted's key determines where it lands naturally

    // Mutate the evicted copy: RUNNING → READY, count the context switch
    evicted.preempt();                      // RUNNING → READY
    evicted.incrementContextSwitches();
    admitToReady(evicted);                  // push back with correct remainingTime

    // Now call nextProcess() – this pops the BEST process from ready_queue.
    // For FIFO that is the process at the head (the one before the evicted one).
    // For PQ that is the process with highest priority/shortest time.
    dispatcher->nextProcess(); // also dumps old runner to waiting_queue (we ignore it)
    
    rrElapsed = 0;
    markFirstRun(dispatcher->getRunningProcess());
};


bool Scheduler::hasRunningProcess() const {
    Process runner = dispatcher->getRunningProcess();
    return runner.getPID() != -1 &&
           runner.getState() == ProcessState::RUNNING;
}

void Scheduler::admitToReady(Process& p) {
    p.setCompareBy(toCompareBy(schedulingAlgorithm));
    ready_queue->add(p);
}

bool Scheduler::admitted(int pid) {
    for (auto &[process_id, hasBeenAdmitted] : admissionMeta)
        if (pid == process_id && hasBeenAdmitted) return true;

    return false;
};

// Not implementated
// see: scheduler.header for more information
Process Scheduler::getWaitingProcess(int pid){
    return Process();
}

void Scheduler::markFirstRun(Process& p) {
    if (p.getFirstRunTime() < 0) {
        p.setFirstRunTime(clock);
    }
}


std::string Scheduler::algo_to_string(Algorithm algorithm) {
    switch (algorithm) {
        case Algorithm::FCFS:     return "FCFS";
        case Algorithm::SJF:      return "SJF";
        case Algorithm::PRIORITY: return "PRIORITY";
        case Algorithm::RR:       return "RR";
    }
    return "UNKNOWN";
}

CompareBy Scheduler::toCompareBy(Algorithm algorithm) {
    switch (algorithm) {
        case Algorithm::FCFS:     return CompareBy::ARRIVAL_TIME;
        case Algorithm::SJF:      return CompareBy::BURST_TIME;
        case Algorithm::PRIORITY: return CompareBy::PRIORITY;
        case Algorithm::RR:       return CompareBy::ARRIVAL_TIME; // RR is FIFO, so compare by arrival time
    }

    return CompareBy::ARRIVAL_TIME; // default fallback
}

// -- reset -- 
void Scheduler::reset() {
    ready_queue->clear();

    // this ensures that the dispatcher is running a sentinel process
    ready_queue->add(Process()); // removed when dispatcher nextProcess is invoked
    dispatcher->nextProcess(); // removed when waiting_queue clear is invoked
    waiting_queue->clear(); 

    clock           = 0;
    rrElapsed       = 0;
    completed->clear();
    
    waitingMeta.clear();
    admissionMeta.clear();
}

// -- stream insertions --
ostream& operator<<(ostream& os, const Scheduler& s) {
    os << "=== Scheduler [tick " << s.clock << "] "
       << Scheduler::algo_to_string(s.schedulingAlgorithm);
    os << *s.dispatcher;
    os << "Completed: " << s.completed->size()
       << " / " << s.processes->size() << "\n";
    return os;
}