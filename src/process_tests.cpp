/**
 * @file test_process.cpp
 * @brief Unit test / demonstration driver for the Process ADT.
 */

#include "process.h"
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <cassert>

// ── helpers ───────────────────────────────────────────────────────────────────
void section(const std::string& title) {
    std::cout << "\n-- " << title << " --\n";
}
void ok(const std::string& msg) {
    std::cout << "  OK  " << msg << "\n";
}

int main() {
    std::cout << "=== Process ADT Test Suite ===\n";

    // ─────────────────────────────────────────────────────────────────────────
    section("1. Construction & identity");
    // ─────────────────────────────────────────────────────────────────────────
    Process p1(1, "P1", 0, 5, 2);
    assert(p1.getPID()         == 1);
    assert(p1.getName()        == "P1");
    assert(p1.getArrivalTime() == 0);
    assert(p1.getBurstTime()   == 5);
    assert(p1.getPriority()    == 2);
    assert(p1.getRemainingTime() == 5);    // remaining starts == burst
    assert(p1.getState()       == ProcessState::NEW);
    ok("PID, name, arrival, burst, priority, remainingTime, state all correct");

    // Invalid construction
    bool threw = false;
    try { Process bad(-1, "X", 0, 5); }
    catch (const std::invalid_argument&) { threw = true; }
    assert(threw);
    ok("Negative PID throws invalid_argument");

    threw = false;
    try { Process bad(2, "X", 0, 0); }
    catch (const std::invalid_argument&) { threw = true; }
    assert(threw);
    ok("Zero burst time throws invalid_argument");

    // ─────────────────────────────────────────────────────────────────────────
    section("2. tick() and isFinished()");
    // ─────────────────────────────────────────────────────────────────────────
    Process p2(2, "P2", 0, 3);
    assert(!p2.isFinished());
    p2.tick(); assert(p2.getRemainingTime() == 2);
    p2.tick(); assert(p2.getRemainingTime() == 1);
    p2.tick(); assert(p2.isFinished());
    p2.tick(); assert(p2.getRemainingTime() == 0);  // clamps at 0
    ok("tick() decrements correctly; isFinished() triggers at 0; no underflow");

    // ─────────────────────────────────────────────────────────────────────────
    section("3. State machine && normal lifecycle");
    // ─────────────────────────────────────────────────────────────────────────
    Process p3(3, "P3", 0, 4);
    p3.setStrictTransitions(true);

    p3.admit();     assert(p3.getState() == ProcessState::READY);
    p3.schedule();  assert(p3.getState() == ProcessState::RUNNING);
    p3.preempt();   assert(p3.getState() == ProcessState::READY);
    p3.schedule();  assert(p3.getState() == ProcessState::RUNNING);
    p3.block();     assert(p3.getState() == ProcessState::WAITING);
    p3.unblock();   assert(p3.getState() == ProcessState::READY);
    p3.schedule();  assert(p3.getState() == ProcessState::RUNNING);
    p3.terminate(); assert(p3.getState() == ProcessState::TERMINATED);
    ok("Full lifecycle: NEW -> READY -> RUNNING -> READY -> RUNNING -> WAITING -> READY -> RUNNING -> TERMINATED");

    // ─────────────────────────────────────────────────────────────────────────
    section("4. State machine && illegal transition enforcement");
    // ─────────────────────────────────────────────────────────────────────────
    threw = false;
    try { p3.admit(); }   // TERMINATED -> READY is illegal
    catch (const std::logic_error&) { threw = true; }
    assert(threw);
    ok("TERMINATED -> READY throws logic_error in strict mode");

    threw = false;
    Process p4(4, "P4", 0, 5);
    p4.setStrictTransitions(true);
    try { p4.schedule(); }   // NEW -> RUNNING is illegal (must go through READY)
    catch (const std::logic_error&) { threw = true; }
    assert(threw);
    ok("NEW -> RUNNING skipping READY throws logic_error");

    // ─────────────────────────────────────────────────────────────────────────
    section("5. firstRunTime and context switches");
    // ─────────────────────────────────────────────────────────────────────────
    Process p5(5, "P5", 2, 6);
    p5.setFirstRunTime(5);
    p5.setFirstRunTime(9);   // second call should be ignored
    assert(p5.getFirstRunTime() == 5);
    ok("setFirstRunTime() records only the first call");

    p5.incrementContextSwitches();
    p5.incrementContextSwitches();
    p5.incrementContextSwitches();
    assert(p5.getContextSwitches() == 3);
    ok("incrementContextSwitches() accumulates correctly");

    // ─────────────────────────────────────────────────────────────────────────
    section("6. Derived metrics");
    // ─────────────────────────────────────────────────────────────────────────
    // Simulate: P6 arrives at tick 2, runs for 4 ticks, finishes at tick 10
    Process p6(6, "P6", 2, 4);
    p6.setFirstRunTime(6);
    p6.setCompletionTime(10);

    assert(p6.getTurnaroundTime() == 8);   // 10 - 2
    assert(p6.getWaitingTime()    == 4);   // 8  - 4
    assert(p6.getResponseTime()   == 4);   // 6  - 2
    ok("Turnaround = 8, Waiting = 4, Response = 4  (all correct)");

    // Metrics unavailable before completion
    Process p7(7, "P7", 0, 3);
    threw = false;
    try { p7.getTurnaroundTime(); }
    catch (const std::logic_error&) { threw = true; }
    assert(threw);
    ok("getTurnaroundTime() throws before process completes");

    // ─────────────────────────────────────────────────────────────────────────
    section("7. reset() and cloneReset()");
    // ─────────────────────────────────────────────────────────────────────────
    Process p8(8, "P8", 1, 5, 3);
    p8.setFirstRunTime(3);
    p8.setCompletionTime(8);
    p8.incrementContextSwitches();
    p8.setState(ProcessState::TERMINATED);

    p8.reset();
    assert(p8.getRemainingTime()   == 5);
    assert(p8.getContextSwitches() == 0);
    assert(p8.getState()           == ProcessState::NEW);
    assert(p8.getCompletionTime()  == -1);
    // arrival, burst, priority preserved
    assert(p8.getArrivalTime() == 1);
    assert(p8.getBurstTime()   == 5);
    assert(p8.getPriority()    == 3);
    ok("reset() clears runtime state but preserves arrival/burst/priority");

    Process orig(9, "P9", 0, 6, 1);
    orig.setFirstRunTime(0);
    orig.setCompletionTime(6);
    Process clone = orig.cloneReset();
    assert(clone.getPID()           == 9);
    assert(clone.getBurstTime()     == 6);
    assert(clone.getRemainingTime() == 6);
    assert(clone.getState()         == ProcessState::NEW);
    ok("cloneReset() produces an identical but cleared copy");

    // ─────────────────────────────────────────────────────────────────────────
    section("8. operator< for priority queues (SJF ordering)");
    // ─────────────────────────────────────────────────────────────────────────
    Process short1(10, "S1", 0, 2, 0, CompareBy::BURST_TIME);
    Process short2(11, "S2", 0, 5, 0, CompareBy::BURST_TIME);
    Process short3(12, "S3", 0, 2, 0, CompareBy::BURST_TIME); // same burst as S1, higher PID

    assert(short1 < short2);   // shorter burst wins
    assert(!(short2 < short1));
    assert(short1 < short3);   // tie-break: lower PID wins
    ok("SJF comparison: shorter burst < longer burst; PID tie-break");

    // Min-heap using Process operator< with a comparator adaptor
    // (std::priority_queue is a max-heap; we need a min-heap for scheduling)
    auto cmp = [](const Process& a, const Process& b){ return !(a < b); };
    std::priority_queue<Process, std::vector<Process>, decltype(cmp)> pq(cmp);
    pq.push(short2);
    pq.push(short3);
    pq.push(short1);
    assert(pq.top().getPID() == 10);  // S1 (burst=2, PID=10) should come out first
    ok("Min-heap via operator< extracts shortest-burst process first");

    // ─────────────────────────────────────────────────────────────────────────
    section("9. toString() and metricsReport()");
    // ─────────────────────────────────────────────────────────────────────────
    Process p10(10, "P10", 3, 7, 1);
    p10.setFirstRunTime(5);
    p10.setCompletionTime(12);
    std::cout << "  toString: " << p10.toString() << "\n";
    std::cout << p10.metricsReport();
    ok("Output methods produce non-empty strings");


    // ─────────────────────────────────────────────────────────────────────────
    std::cout << "\n=== All tests passed ===\n";
    return 0;
}