#include <tbb/flow_graph.h>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/tbb.h>

#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include <vector>
#include <map>

static void sleep_ms(int ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

int main() {
  using namespace tbb::flow;
  tbb::global_control global_limit(tbb::global_control::max_allowed_parallelism, 1);
  graph g;
  std::map<int, int> priority_map;
  priority_map[1] = 1;
  priority_map[2] = 2;
  priority_map[3] = 2;
  priority_map[4] = 2;
  priority_map[5] = 1;
  priority_map[6] = 0;
  priority_map[7] = 2;

  std::map<int, std::string> priority_str;
  priority_str[0] = "low";
  priority_str[1] = "medium";
  priority_str[2] = "high";


  // Helper lambdas for printing start/end with priority
  auto print_begin = [](int task_number, const char* priority) {
    std::cout << "Begin Task " << task_number << " (priority: " << priority << ")\n";
  };
  auto print_end = [](int task_number, const char* priority) {
    std::cout << "End Task " << task_number << " (priority: " << priority << ")\n";
  };

  // Task 1: print A,B,C and sleep 2s, priority medium
  continue_node<continue_msg> t1(g, [&](const continue_msg&) {
    int task_number = 1;
    print_begin(task_number, priority_str[priority_map[task_number]].c_str());
    std::cout << "A\nB\nC\n";
    sleep_ms(2000);
    print_end(task_number, priority_str[priority_map[task_number]].c_str());
    return continue_msg();
  }, node_priority_t(priority_map[1]));

  // Task 2: print D,E,F,G and sleep 3s, priority high
  continue_node<continue_msg> t2(g, [&](const continue_msg&) {
    int task_number = 2;
    print_begin(task_number, priority_str[priority_map[task_number]].c_str());
    std::cout << "D\nE\nF\nG\n";
    sleep_ms(3000);
    print_end(task_number, priority_str[priority_map[task_number]].c_str());
    return continue_msg();
  }, node_priority_t(priority_map[2]));

  // Task 3: print H to L and sleep 1s, priority high
  continue_node<continue_msg> t3(g, [&](const continue_msg&) {
    int task_number = 3;
    print_begin(task_number, priority_str[priority_map[task_number]].c_str());
    for (char c = 'H'; c <= 'L'; ++c) {
      std::cout << c << "\n";
    }
    sleep_ms(1000);
    print_end(task_number, priority_str[priority_map[task_number]].c_str());
    return continue_msg();
  }, node_priority_t(priority_map[3]));

  // Task 4: print M to Z and sleep 2s, priority high
  continue_node<continue_msg> t4(g, [&](const continue_msg&) {
    int task_number = 4;
    print_begin(task_number, priority_str[priority_map[task_number]].c_str());
    for (char c = 'M'; c <= 'Z'; ++c) {
      std::cout << c << "\n";
    }
    sleep_ms(2000);
    print_end(task_number, priority_str[priority_map[task_number]].c_str());
    return continue_msg();
  }, node_priority_t(priority_map[4]));

  // Task 5: produce a vector<int> of 200 random ints, priority low
  function_node<continue_msg, std::vector<int>> t5(g, serial, [&](const continue_msg&) {
    int task_number = 5;
    print_begin(task_number, priority_str[priority_map[task_number]].c_str());
    std::vector<int> v(200);
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 1000);
    for (auto &val : v) {
      val = dist(gen);
    }
    sleep_ms(100); // Just a small sleep to simulate work
    print_end(task_number, priority_str[priority_map[task_number]].c_str());
    return v;
  }, node_priority_t(priority_map[5]));

// Task 6: should print the content of the vector produced by task 5
// We'll use a function_node to take std::vector<int> as input and produce continue_msg as output.
function_node<std::vector<int>, continue_msg> t6(
    g, 
    serial, 
    [&](const std::vector<int>& vec) {
      int task_number = 6;
      print_begin(task_number, priority_str[priority_map[task_number]].c_str()); 
      tbb::parallel_for(
        tbb::blocked_range<size_t>(0, vec.size()),
        [&](const tbb::blocked_range<size_t>& r) {
          for (size_t i = r.begin(); i < r.end(); ++i) {
            std::cout << "Task 6 printing index " << i << ": " << vec[i] << "\n";
          }
        }
      );
      print_end(task_number, priority_str[priority_map[task_number]].c_str());
      return continue_msg();
    },
    node_priority_t(priority_map[6]) // If you want to set a priority
);


  // Task 7: print "ciao" and sleep for 5 seconds. priority high
  // Task 7 depends on task 5 and 4.
  // We have a join node to combine outputs from 5 (vector<int>) and 4 (continue_msg).
  join_node<std::tuple<std::vector<int>, continue_msg>, queueing> j(g);

  function_node<std::tuple<std::vector<int>, continue_msg>, continue_msg> t7(
      g, serial, [&](const std::tuple<std::vector<int>, continue_msg>&) {
        int task_number = 7;
        print_begin(task_number, priority_str[priority_map[task_number]].c_str());
        std::cout << "ciao\n";
        sleep_ms(5000);
        print_end(task_number, priority_str[priority_map[task_number]].c_str());
        return continue_msg();
      }, node_priority_t(priority_map[7]));

  // Now we set up dependencies:
  // The chain: 1 -> 2 -> 3 -> 4
  make_edge(t1, t2);
  make_edge(t2, t3);
  make_edge(t3, t4);

  // Task 6 depends on 5
  make_edge(t5, t6);

  // Task 7 depends on 5 and 4: connect them to join node
  make_edge(t5, input_port<0>(j));
  make_edge(t4, input_port<1>(j));

  // connect join node to t7
  make_edge(j, t7);

  // Start the flow by activating task 1 and task 5
  t1.try_put(continue_msg());
  t5.try_put(continue_msg());

  g.wait_for_all();

  return 0;
}
