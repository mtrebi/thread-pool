# Introduction

A [thread pool](https://en.wikipedia.org/wiki/Thread_pool) is a technique that allows developers to exploit the concurrency of modern processors in an **easy** and **efficient** manner. It's easy because you send "work" to the pool and somehow this work gets done without blocking the main thread. It's efficient because threads are not initialized each time we want work to be done. Threads are initialized once and remain inactive until some work has to be done. This way we minimize the overhead.

There are many many Thread pool implementations in C++, many of them are probably better (safer, faster...) than mine. However,  I belive my implementation is **very straightforward and easy to understand**. 

# Thread pool

The way that I understand things better is with images. So, lets take a look at the image of Thread Pool given by wikipedia:

<p align="center">  <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/0/0c/Thread_pool.svg/580px-Thread_pool.svg.png"> </p>

As you can see, we have three important elements here:
* *Tasks Queue*. This is where the work that has to be done is stored.
* *Thread Pool*. This is set of threads (or workers) that continuously take work from the queue and do it.
* *Completed Tasks*. When the Thread has finished the work we return "something" to notify that the work has finished.

## Queue

We use a queue to store the work because it's the more sensible data structure. We want the work to be **started** in the same order that we sent it. However, this queue is a little bit **special**. As I said in the previous section, threads are continously (well, not really, but let's assume that they are) querying the queue to ask for work. When there's work available, threads take the work from the queue and do it. What would happen if two threads try to take the same work at the same time? Well, the program would crash.

To avoid this kind of problems, I implemented a wrapper over the standard C++ Queue that uses mutex to restrict the concurrent access. Let's see a small sample of the SafeQueue class:

```c
void enqueue(T& t) {
	std::unique_lock<std::mutex> lock(m_mutex);
	m_queue.push(t);
}

``` 
To enqueue the first thing we do is lock the mutex to make sure that no one else is accessing the resource. Then, we push the element to the queue. When the lock goes out of scopes it gets automatically released. Easy, huh? This way, we make the Queue thread-safe and thus we don't have to worry many threads accessing and/or modifying it at the same "time".

## Thread worker

As I told you before, the behavior of the worker should look like this:

1. Loop
	1. If Queue is not empty
		1. Unqueue work
		2. Do it

This looks quite simple but it's not very efficient. Doyou see why? What would happen if there is no work in the Queue? The threads would keep looping and asking **all the time: Is the queue empty? ** 

Imagine that we have some kind of signaling system that allowed us to to sleep a thread until new work is added to the Queue, that would allow us to change the implementation:

1. Loop
	1. If Queue is empty
		1. Wait signal
	2. Unqueue work
	3. Do it

This is way more efficient! But, who is going the send this signal? Well, the most sensible thing is to send a signal every time we add a new element to the Queue. This way, if there is a thread waiting is going to be woken up. Otherwise it will be ignored.

This signal system is implemented in C++ with **conditional variables**. Conditional variables are always bound to a mutex, so I added a mutex to the Thread Pool class just to manage this. The final code of a worker looks like this: 

```c
void operator()() {
  std::shared_ptr<std::packaged_task<void()>> task_ptr;
  bool empty;
  while (!m_pool->m_shutdown) {
    {
      std::unique_lock<std::mutex> lock(m_pool->m_conditional_mutex);
      if (m_pool->m_queue.empty()) {
        m_pool->m_conditional_lock.wait(lock);
      }
      empty = m_pool->m_queue.dequeue(task_ptr);
    }
    if (!empty) {
      std::packaged_task<void()>* task = task_ptr.get();
      (*task)();
    }
  }
}

```

## Thread pool

The most important method of the Thread Pool is the one responsible of adding work to the queue. I called this method **submit**. It's not difficult to understand how it works but its implementation can seem scary at first. Let's think about what it should do:
1. It should add some kind of "work" to the queue
2. It should send a signal to wake a thread in case someone is waiting
3. It should return "something" the result of the work

### Add work to the queue

In order to make something useful, we want the Thread Pool to accept any unit of work. I think the smallest unit of work in a program is a **function**. Our work will be exaclty one function execution. Thus, we want our submit method to get a function with it's parameters and add it to the queue. But, how the hell can we do that? Well, I've discovered many C++ features in order to achieve this effect. Let's start:

```c
template<typename F, typename...Args>
``` 
This means that the next statemnt is templated. In this case, we're saying that the first parameter is a generic function that takes a variable number of arguments.

```c
std::future<void> submit(F&& f, Args&&... args)
``` 
When the type of a parameter is declared as **T&&** for some deducted type T that parameter is a **universal reference**. This term was coined by [Scott Meyers](https://isocpp.org/blog/2012/11/universal-references-in-c11-scott-meyers) because **T&&** can also mean r-value reference. However, in the context of type deduction, it means that it can be bound to both l-values and r-values, unlike l-value references that can only be bound to to non-const objects (they bind only to modifiable lvalues) and r-value references (they bind only to rvalues).

```c
// Create a function with bounded parameters ready to execute
std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
``` 
There are many many things happening here. The std::function<T> is a C++ object that encapsulates a function. It allows you to execute the function as if it were a normal function calling the method () with the required parameters BUT, because it is an object, you can store it, move it...

In order to know the signature of this function,we use **decltype**. This function inspects the type of an expression. In our case, we give decltype the funcion f with the all the generic and variable parameters args.




The std::bind function takes a function F and a set of parameters to create an object of type std::function that can be executed 



```c
template<typename F, typename...Args>
std::future<void> submit(F&& f, Args&&... args) {
	// Create a function with bounded parameters ready to execute
	std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
	// Encapsulate it into a shared ptr in order to be able to copy construct / assign 
	std::shared_ptr<std::packaged_task<void()>> task_ptr = std::make_shared<std::packaged_task<void()>>(func);

	m_queue.enqueue((task_ptr));
	m_conditional_lock.notify_one();

	// Return future from promise
	return task_ptr->get_future();
}
```



# Usage example

Creating the Thread Pool is as easy as:

```c
// Create pool with 3 threads
ThreadPool pool(3);

// Initialize pool
pool.init();
```

Then, if we want to send some work to the pool we just have to call the submit function:

```c
pool.submit(work);
```



Deending on the type of work, I've distinguished different use-cases. Suppose that the work that we have to do is multiply two numbers. We can do it in many different ways. I've implemented the three most common ways to do it that I can imagine:
* Use-Case #1. Function returns the result
* Use-Case #2. Function updates by ref parameter with the result
* Use-Case #3. Function prints the result

__Note: This is just to show how the submit function works. Options are not exclusive __

## Use-Case #1
The multiply function with a return looks like this:

```c
// Simple function that adds multiplies two numbers and returns the result
int multiply(const int a, const int b) {
  const int res = a * b;
  return res;
}
```

Then, the submit:


```c
// The type of future is given by the return type of the function
std::future<int> future = pool.submit(multiply, 2, 3);
```

We can also use the **auto** keyword for convenience:

```c
auto future = pool.submit(multiply, 2, 3);
```

Nice, finally, when the pool finishes the work it will automatically update the future. Then, we can retrieve the result:

So, when the work is done we know that the future will get updated and we can retrieve the result calling:
```c
const int result = future.get();
std::cout << result << std::endl;
```

The get() function of std::future<T> always return the type T of the future.

## Case #2
The multiply function has a parameter passed by ref:

```c
// Simple function that adds multiplies two numbers and updates the out_res variable passed by ref
void  multiply(int& out_res, const int a, const int b) {
	out_res = a * b;
}
```

Now, we have to call the submit function with a subtle difference. Because we are using templates and type deduction, the parameter passed by ref needs to be called using **std::ref(param)** to make sure that we are passing it by ref and not by value.

```c
int result = 0;
auto future = pool.submit(multiply, std::ref(result), 2, 3);
// result is 0
future.get();
// result is 6
std::cout << result << std::endl;
```

In this case, what's the type of future? Well, it is **std::future<void>** because this multiply function is of type void. Calling future.get() returns void but it's still useful to make sure that the work has been done.


## Case #3
The last case is the easiest one. Our multiply function simply prints the result:

We have a simple function without output parameters. For this example I implemented the following multiplication function:

```c
// Simple function that adds multiplies two numbers and prints the result
void multiply(const int a, const int b) {
  const int result = a * b;
  std::cout << result << std::endl;
}
```

Then, we can simply call:

```c
auto future = pool.submit(multiply, 2, 3);
future.get();
```

In this case, we know that as soon as the multiplication is done it will be printed. If we care when this is done, we can wait for it calling future.get(), otherwise we can just continue the execution and eventually it will be done.

Checkout the main program for a complete example.

# Future work

* Make it safer (exceptions)
* Run benchmarks
