#include <iostream>


#include "SafeQueue.h"
#include "ThreadPool.h"
#include <utility>


// Simple function that adds multiplies two numbers and prints the result
void multiply(const int a, const int b) {
  // Simulate hard computation
  _sleep(2000);
  const int res = a * b;
  std::cout << a << " * " << b << " = " << res << std::endl;
}


// Same as before but now we have an output parameter
void multiply_output(const int a, const int b, int & res) {
  // Simulate hard computation
  _sleep(2000);
  res = a * b;
  std::cout << a << " * " << b << " = " << res << std::endl;
}


int main() {
  // Create pool with 3 threads
  ThreadPool pool(3);
  // Initialize pool
  pool.init();

  // Initialize local variables
  int res;
  const int a = 2, b = 3, c = 4, d = 5;

  // Submit multiplication table
  for (int i = 1; i < 10; ++i) {
    for (int j = 1; j < 10; ++j) {
      pool.submit(multiply, i, j);
    }
  }

  // Submit some functions to execute
  auto multiply_future = pool.submit(multiply_output, a, b, std::ref(res));

  // Wait for multiplication output to finish
  multiply_future.get();
  std::cout << "Last operation result is equals to " << res << std::endl;

  _sleep(10000);
  pool.shutdown();
}