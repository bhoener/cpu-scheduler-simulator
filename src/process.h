#ifndef PROCESS_H
#define PROCESS_H

/**
 * @file Process.h
 * @brief ADT representing a single OS process in the CPU scheduling simulation
 *
 * Encapsulates every piece of state the scheduler needs:
 *   - Identity         : PID, name
 *   - Scheduling input : arrival time, burst time, priority
 *   - Execution state  : remaining burst, context-switch metadata
 *   - Metrics output   : waiting time, turnaround time, completion time
 *   - Lifecycle state  : ProcessState enum (NEW -> READY -> RUNNING -> WAITING -> TERMINATED)
 *
 * Design notes
 * -------------
 *  - All time values are in discrete simulation ticks (integers).
 *  - The class is intentionally value-semantic (copyable) so schedulers can
 *    clone processes for simulation without affecting the originals.
 *  - Derived metrics (waiting time, turnaround time, response time) are
 *    computed lazily via const methods once the raw timestamps are set.
 *  - operator< is defined for priority-queue ordering (used by SJF and
 *    Priority scheduling)
 *  - The comparison field is selectable at construction.
 */

#include <string>
#include <iostream>
#include <stdexcept>
#include <sstream>

//  ProcessState  –  models the 5-state OS process lifecycle
enum class ProcessState {
    NEW,         // Process created, not yet admitted to ready queue
    READY,       // Waiting in the ready queue for CPU
    RUNNING,     // Currently executing on CPU
    WAITING,     // Blocked on I/O or an event (reserved for future I/O simulation)
    TERMINATED   // Execution complete
};

//  CompareBy  –  selects which field drives operator< (for priority queues)
enum class CompareBy {
    ARRIVAL_TIME,     // FCFS order
    BURST_TIME,       // SJF (shortest-job-first)
    REMAINING_TIME,   // SRTF (shortest-remaining-time-first)
    PRIORITY          // Priority scheduling
};


class Process {
    private:
    // -- Identity fields -------------------------------------------------------
    int         pid_;
    std::string name_;

    // -- Scheduling input fields -----------------------------------------------
    int arrivalTime_;      // Tick the process enters the system
    int burstTime_;        // Original total CPU requirement
    int priority_;         // Lower value = higher urgency (convention)

    // -- Runtime / execution state fields -------------------------------------
    int remainingTime_;    // Decremented each tick; reaches 0 on completion
    int contextSwitches_;  // Count of times preempted / re-scheduled
    int firstRunTime_;     // Tick of first CPU assignment (-1 = not yet run)
    int lastStartTime_;    // Tick most recently assigned to CPU

    // -- Completion / metrics fields -------------------------------------------
    int completionTime_;   // Tick the process finished (-1 = not yet finished)

    // -- Lifecycle state -------------------------------------------------------
    ProcessState state_;
    bool strictTransitions_;  // If true, enforce legal state transitions

    // -- Priority-queue ordering -----------------------------------------------
    CompareBy compareBy_;

    // -- Internal helpers ------------------------------------------------------
    bool isLegalTransition(ProcessState from, ProcessState to) const;
public:

    // -- Construction ---------------------------------------------------------

    /**
     * @brief Full constructor.
     * @param pid         Unique process ID (positive integer).
     * @param name        Human-readable label (e.g. "P1").
     * @param arrivalTime Tick at which the process enters the system.
     * @param burstTime   Total CPU ticks required to complete.
     * @param priority    Scheduling priority (0 = highest; default 0).
     *                    Relevant only for priority-based algorithms.
     * @param compareBy   Ordering field for priority-queue comparisons.
     */
    Process(int         pid,
            std::string name,
            int         arrivalTime,
            int         burstTime,
            int         priority   = 0,
            CompareBy   compareBy  = CompareBy::ARRIVAL_TIME);

    /**
     * @brief Default constructor – produces a sentinel / placeholder process.
     *        PID = -1, all times = 0.
     */
    Process();

    // -- Identity -------------------------------------------------------------

    int         getPID()  const { return pid_;  }
    std::string getName() const { return name_; }

    void setPID (int pid)              { pid_  = pid;  }
    void setName(const std::string& n) { name_ = n;    }

    // -- Scheduling inputs (set once at creation) ------------------------------

    int getArrivalTime() const { return arrivalTime_; }
    int getBurstTime()   const { return burstTime_;   }
    int getPriority()    const { return priority_;    }

    void setArrivalTime(int t);
    void setBurstTime  (int t);
    void setPriority   (int p);

    // -- Execution state (mutated by the scheduler at runtime) -----------------

    /** Remaining CPU time still needed (decremented each tick). */
    int  getRemainingTime() const { return remainingTime_; }
    void setRemainingTime(int t);

    /** Decrement remaining time by one tick; clamps to 0. */
    void tick();

    /** True when remaining time reaches 0. */
    bool isFinished() const { return remainingTime_ <= 0; }

    /** Number of context switches this process has experienced. */
    int  getContextSwitches()  const { return contextSwitches_;  }
    void incrementContextSwitches()  { ++contextSwitches_;       }

    /** Tick at which this process last entered RUNNING state (for response time). */
    int  getFirstRunTime()  const { return firstRunTime_;  }
    void setFirstRunTime(int t);       // only records the FIRST time

    /** Tick at which the process most recently started executing (for metrics). */
    int  getLastStartTime() const { return lastStartTime_; }
    void setLastStartTime(int t)  { lastStartTime_ = t;    }

    // -- Completion timestamps (written once by the scheduler on finish) --------

    int  getCompletionTime() const { return completionTime_; }
    void setCompletionTime(int t);

    // -- Derived metrics (computed lazily; valid after completion) -------------

    /**
     * Turnaround time = completion time − arrival time.
     * Represents total time from submission to completion.
     */
    int getTurnaroundTime() const;

    /**
     * Waiting time = turnaround time − burst time.
     * Time spent in the ready queue (not executing).
     */
    int getWaitingTime() const;

    /**
     * Response time = first run time − arrival time.
     * Time until the process first gets the CPU (important for interactive systems).
     */
    int getResponseTime() const;

    // -- Lifecycle state machine -----------------------------------------------

    ProcessState getState() const { return state_; }

    /**
     * Transition to a new state.
     * Enforces legal transitions:
     *   NEW        -> READY
     *   READY      -> RUNNING
     *   RUNNING    -> READY | WAITING | TERMINATED
     *   WAITING    -> READY
     *   TERMINATED -> (no transitions allowed)
     *
     * @throws std::logic_error on illegal transition (configurable via
     *         setStrictTransitions(); strict mode off by default for flexibility).
     */
    void setState(ProcessState newState);

    /** Convenience wrappers for common transitions. */
    void admit()      { setState(ProcessState::READY);      }   // NEW -> READY
    void schedule()   { setState(ProcessState::RUNNING);    }   // READY -> RUNNING
    void preempt()    { setState(ProcessState::READY);      }   // RUNNING -> READY
    void block()      { setState(ProcessState::WAITING);    }   // RUNNING -> WAITING
    void unblock()    { setState(ProcessState::READY);      }   // WAITING -> READY
    void terminate()  { setState(ProcessState::TERMINATED); }   // RUNNING -> TERMINATED

    /** Enable/disable strict state-transition checking. */
    void setStrictTransitions(bool strict) { strictTransitions_ = strict; }
    bool isStrictTransitions() const       { return strictTransitions_;   }

    // -- Comparison (for priority-queue ordering) ------------------------------

    /**
     * operator< drives std::priority_queue min-heap ordering.
     * Behaviour depends on the compareBy_ field set at construction:
     *   ARRIVAL_TIME    -> earlier arrival wins  (FCFS)
     *   BURST_TIME      -> shorter burst wins    (SJF)
     *   REMAINING_TIME  -> shorter remaining wins (SRTF)
     *   PRIORITY        -> lower priority value wins
     *
     * In all cases ties are broken by PID (lower PID wins).
     */
    bool operator<(const Process& other) const;
    bool operator==(const Process& other) const { return pid_ == other.pid_; }
    bool operator!=(const Process& other) const { return !(*this == other);  }

    /** Change the comparison field after construction (useful when switching algorithms). */
    void setCompareBy(CompareBy cb) { compareBy_ = cb; }
    CompareBy getCompareBy() const  { return compareBy_; }

    // -- Reset / clone helpers -------------------------------------------------

    /**
     * Reset all runtime state back to initial values (arrival, burst, priority
     * are preserved). Used to replay the same process set through multiple
     * scheduling algorithms.
     */
    void reset();

    /**
     * Return a deep copy with all runtime state cleared (same as calling
     * reset() on a copy). Convenient for feeding the same workload into
     * multiple schedulers without aliasing.
     */
    Process cloneReset() const;

    // -- Display / debug -------------------------------------------------------

    /** One-line compact summary: PID | Name | Arrival | Burst | Priority | State */
    std::string toString() const;

    /** Multi-line metric report (valid after process terminates). */
    std::string metricsReport() const;

    /** Stream insertion operator (delegates to toString()). */
    friend std::ostream& operator<<(std::ostream& os, const Process& p);

    // -- Static utilities ------------------------------------------------------

    /** Human-readable name for a ProcessState value. */
    static std::string stateToString(ProcessState s);

    /** Human-readable name for a CompareBy value. */
    static std::string compareByToString(CompareBy cb);
};

//  small method implementations (non-trivial ones go in Process.cpp)
inline std::ostream& operator<<(std::ostream& os, const Process& p) {
    return os << p.toString();
}

#endif // PROCESS_H