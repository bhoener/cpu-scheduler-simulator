/**
 * @file Process.cpp
 * @brief Implementation of the Process ADT for the CPU scheduling simulation.
 */

#include "process.h"
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <algorithm>

// -----------------------------------------------------------------------------
//  Construction
// -----------------------------------------------------------------------------

Process::Process()
    : pid_(-1),
      name_("(none)"),
      arrivalTime_(0),
      burstTime_(0),
      priority_(0),
      remainingTime_(0),
      contextSwitches_(0),
      firstRunTime_(-1),
      lastStartTime_(-1),
      completionTime_(-1),
      state_(ProcessState::NEW),
      strictTransitions_(false),
      compareBy_(CompareBy::ARRIVAL_TIME)
{}

Process::Process(int         pid,
                 std::string name,
                 int         arrivalTime,
                 int         burstTime,
                 int         priority,
                 CompareBy   compareBy)
    : pid_(pid),
      name_(std::move(name)),
      arrivalTime_(arrivalTime),
      burstTime_(burstTime),
      priority_(priority),
      remainingTime_(burstTime),   // remaining starts equal to total burst
      contextSwitches_(0),
      firstRunTime_(-1),
      lastStartTime_(-1),
      completionTime_(-1),
      state_(ProcessState::NEW),
      strictTransitions_(false),
      compareBy_(compareBy)
{
    if (pid <= 0)
        throw std::invalid_argument("Process PID must be positive (got "
                                    + std::to_string(pid) + ")");
    if (arrivalTime < 0)
        throw std::invalid_argument("Arrival time cannot be negative");
    if (burstTime <= 0)
        throw std::invalid_argument("Burst time must be positive (got "
                                    + std::to_string(burstTime) + ")");
}

// -----------------------------------------------------------------------------
//  Mutating setters (validated)
// -----------------------------------------------------------------------------

void Process::setArrivalTime(int t) {
    if (t < 0) throw std::invalid_argument("Arrival time cannot be negative");
    arrivalTime_ = t;
}

void Process::setBurstTime(int t) {
    if (t <= 0) throw std::invalid_argument("Burst time must be positive");
    burstTime_     = t;
    remainingTime_ = t;   // keep remaining in sync if burst is reset
}

void Process::setPriority(int p) {
    priority_ = p;
}

void Process::setRemainingTime(int t) {
    if (t < 0) throw std::invalid_argument("Remaining time cannot be negative");
    remainingTime_ = t;
}

void Process::setCompletionTime(int t) {
    if (t < 0) throw std::invalid_argument("Completion time cannot be negative");
    completionTime_ = t;
}

void Process::setFirstRunTime(int t) {
    if (firstRunTime_ == -1)   // only record the FIRST time
        firstRunTime_ = t;
}

// -----------------------------------------------------------------------------
//  Execution helpers
// -----------------------------------------------------------------------------

void Process::tick() {
    if (remainingTime_ > 0)
        --remainingTime_;
}

// -----------------------------------------------------------------------------
//  Derived metrics
// -----------------------------------------------------------------------------

int Process::getTurnaroundTime() const {
    if (completionTime_ < 0)
        throw std::logic_error("Turnaround time unavailable: process '"
                               + name_ + "' has not yet completed");
    return completionTime_ - arrivalTime_;
}

int Process::getWaitingTime() const {
    // waiting = turnaround - burst (time spent ready but not executing)
    return getTurnaroundTime() - burstTime_;
}

int Process::getResponseTime() const {
    if (firstRunTime_ < 0)
        throw std::logic_error("Response time unavailable: process '"
                               + name_ + "' has never been scheduled");
    return firstRunTime_ - arrivalTime_;
}

// -----------------------------------------------------------------------------
//  State machine
// -----------------------------------------------------------------------------

bool Process::isLegalTransition(ProcessState from, ProcessState to) const {
    switch (from) {
        case ProcessState::NEW:
            return to == ProcessState::READY;
        case ProcessState::READY:
            return to == ProcessState::RUNNING;
        case ProcessState::RUNNING:
            return to == ProcessState::READY
                || to == ProcessState::WAITING
                || to == ProcessState::TERMINATED;
        case ProcessState::WAITING:
            return to == ProcessState::READY;
        case ProcessState::TERMINATED:
            return false;   // no transitions out of terminal state
    }
    return false;
}

void Process::setState(ProcessState newState) {
    if (strictTransitions_ && !isLegalTransition(state_, newState)) {
        throw std::logic_error(
            "Illegal process state transition for '" + name_ + "': "
            + stateToString(state_) + " -> " + stateToString(newState));
    }
    state_ = newState;
}

// -----------------------------------------------------------------------------
//  Comparison (for priority queues)
// -----------------------------------------------------------------------------

bool Process::operator<(const Process& other) const {
    int lhs, rhs;

    switch (compareBy_) {
        case CompareBy::ARRIVAL_TIME:
            lhs = arrivalTime_;
            rhs = other.arrivalTime_;
            break;
        case CompareBy::BURST_TIME:
            lhs = burstTime_;
            rhs = other.burstTime_;
            break;
        case CompareBy::REMAINING_TIME:
            lhs = remainingTime_;
            rhs = other.remainingTime_;
            break;
        case CompareBy::PRIORITY:
            lhs = priority_;
            rhs = other.priority_;
            break;
        default:
            lhs = arrivalTime_;
            rhs = other.arrivalTime_;
    }

    if (lhs != rhs) return lhs < rhs;
    return pid_ < other.pid_;   // tie-break by PID
}

// -----------------------------------------------------------------------------
//  Reset / clone
// -----------------------------------------------------------------------------

void Process::reset() {
    remainingTime_   = burstTime_;
    contextSwitches_ = 0;
    firstRunTime_    = -1;
    lastStartTime_   = -1;
    completionTime_  = -1;
    state_           = ProcessState::NEW;
}

Process Process::cloneReset() const {
    Process p = *this;
    p.reset();
    return p;
}

// -----------------------------------------------------------------------------
//  Display
// -----------------------------------------------------------------------------

std::string Process::toString() const {
    std::ostringstream oss;
    oss << "PID:" << std::setw(3) << pid_
        << "  name: " << std::setw(3) << std::left << name_ << std::right
        << "  arr:" << std::setw(3) << arrivalTime_
        << "  burst:" << std::setw(3) << burstTime_
        << "  rem:" << std::setw(3) << remainingTime_
        << "  pri:" << std::setw(2) << priority_
        << "  [" << stateToString(state_) << "]";
    return oss.str();
}

std::string Process::metricsReport() const {
    std::ostringstream oss;
    const int W = 22;

    oss << "-- Process Metrics: " << name_ << " (PID " << pid_ << ") --\n";
    oss << ": " << std::left << std::setw(W) << "Arrival time"
                << arrivalTime_        << " ticks\n";
    oss << ": " << std::setw(W) << "Burst time"
                << burstTime_          << " ticks\n";
    oss << ": " << std::setw(W) << "Priority"
                << priority_           << "\n";
    oss << ": " << std::setw(W) << "Context switches"
                << contextSwitches_    << "\n";

    if (completionTime_ >= 0) {
        oss << ": " << std::setw(W) << "Completion time"
                    << completionTime_   << " ticks\n";
        oss << ": " << std::setw(W) << "Turnaround time"
                    << getTurnaroundTime() << " ticks\n";
        oss << ": " << std::setw(W) << "Waiting time"
                    << getWaitingTime()    << " ticks\n";
    } else {
        oss << ": (process not yet completed)\n";
    }

    if (firstRunTime_ >= 0) {
        oss << ": " << std::setw(W) << "Response time"
                    << getResponseTime()   << " ticks\n";
    }

    oss << "--" << std::string(38, '-') << "\n";
    return oss.str();
}

// -----------------------------------------------------------------------------
//  Static utilities
// -----------------------------------------------------------------------------

std::string Process::stateToString(ProcessState s) {
    switch (s) {
        case ProcessState::NEW:        return "NEW";
        case ProcessState::READY:      return "READY";
        case ProcessState::RUNNING:    return "RUNNING";
        case ProcessState::WAITING:    return "WAITING";
        case ProcessState::TERMINATED: return "TERMINATED";
    }
    return "UNKNOWN";
}

std::string Process::compareByToString(CompareBy cb) {
    switch (cb) {
        case CompareBy::ARRIVAL_TIME:   return "ARRIVAL_TIME";
        case CompareBy::BURST_TIME:     return "BURST_TIME";
        case CompareBy::REMAINING_TIME: return "REMAINING_TIME";
        case CompareBy::PRIORITY:       return "PRIORITY";
    }
    return "UNKNOWN";
}