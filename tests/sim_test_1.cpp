#include "core/scheduler/event-id.hpp"
#include "core/simulator/simulator.hpp"

#include <iostream>
#include <vector>

using namespace core;

static std::vector<int> g_order;
static std::vector<EventId> g_pending;

static void PushValue(int x) {
  std::cout << "Executing event " << x << " at sim time " << Simulator::Now()
            << "\n";
  g_order.push_back(x);
}

int main() {
  std::cout << "Scheduling event 10 at T=1\n";
  g_pending.push_back(Simulator::Schedule(1, &PushValue, 10));

  std::cout << "Scheduling event 5 at T=5\n";
  g_pending.push_back(Simulator::Schedule(5, &PushValue, 5));

  std::cout << "Scheduling event 7 at T=7\n";
  g_pending.push_back(Simulator::Schedule(7, &PushValue, 7));

  std::cout << "Starting run at sim time " << Simulator::Now() << "\n";
  Simulator::Run();

  std::cout << "Run finished at sim time " << Simulator::Now() << "\n";

  std::cout << "Execution order: ";
  for (int x : g_order) {
    std::cout << x << " ";
  }
  std::cout << "\n";

  return 0;
}
