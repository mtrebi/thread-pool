#include <iostream>
#include <random>

#include "ThreadPool.h"

std::random_device rd;
std::mt19937 mt(rd());
std::uniform_real_distribution<double> dist(0, 1000);
auto random = std::bind(dist, mt);


// Simple function that adds multiplies two numbers and prints the result
void multiply(const int a, const int b) {
  // Simulate hard computation
  _sleep(2000 + random());
  const int res = a * b;
  std::cout << a << " * " << b << " = " << res << std::endl;
}


// Same as before but now we have an output parameter
void multiply_output(int & out, const int a, const int b) {
  // Simulate hard computation
  _sleep(2000 + random());
  out = a * b;
  std::cout << a << " * " << b << " = " << out << std::endl;
}

// Same as before but now we have an output parameter
int multiply_return(const int a, const int b) {
  // Simulate hard computation
  _sleep(2000 + random());
  const int res = a * b;
  std::cout << a << " * " << b << " = " << res << std::endl;
  return res;
}


int main() {
  // Create pool with 3 threads
  ThreadPool pool(3);

  // Initialize pool
  pool.init();

  // Submit (partial) multiplication table
  for (int i = 1; i < 3; ++i) {
    for (int j = 1; j < 10; ++j) {
      pool.submit(multiply, i, j);
    }
  }

  // Submit function with output parameter passed by ref
  int output_ref;
  auto future1 = pool.submit(multiply_output, std::ref(output_ref), 5, 6);

  // Wait for multiplication output to finish
  future1.get();
  std::cout << "Last operation result is equals to " << output_ref << std::endl;

  // Submit function with return parameter 
  auto future2 = pool.submit(multiply_return, 5, 3);

  // Wait for multiplication output to finish
  int res = future2.get();
  std::cout << "Last operation result is equals to " << res << std::endl;
  
  pool.shutdown();
}