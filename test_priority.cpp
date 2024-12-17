#include <chrono>
#include <iostream>
#include <thread>

#include <tbb/parallel_for.h>
#include <tbb/task_arena.h>
#include <tbb/this_task_arena.h>

int main() {
  // High-priority arena (for letters)
  tbb::task_arena highPriArena(
    tbb::task_arena::automatic,
    tbb::task_arena::automatic,
    tbb::priority_high
  );

  // Low-priority arena (for numbers)
  tbb::task_arena lowPriArena(
    tbb::task_arena::automatic,
    tbb::task_arena::automatic,
    tbb::priority_low
  );

  highPriArena.execute([&]() {


    // Now nest a low-priority arena execution
    lowPriArena.execute([&]() {
      tbb::this_task_arena::isolate([&]() {
        // Print numbers 0-9 in parallel
        tbb::parallel_for(size_t(0), size_t(10), [&](size_t i) {
          std::cout << "Number: " << i << "\n";
        });
      });
    });

    // Run a set of tasks that print letters A-J, each with a delay
    tbb::parallel_for(size_t(0), size_t(10), [&](size_t i) {
      char letter = static_cast<char>('A' + i);
      // Simulate work
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      std::cout << "Letter: " << letter << "\n";
    });
  });

  return 0;
}
