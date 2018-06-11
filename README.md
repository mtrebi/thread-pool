# Table of Contents
&nbsp;[Introduction](https://github.com/mtrebi/thread-pool/blob/master/README.md#introduction)  <br/> 
&nbsp;[Build instructions](https://github.com/mtrebi/thread-pool/blob/master/README.md#build-instructions)  <br/> 
&nbsp;[Thread pool](https://github.com/mtrebi/thread-pool/blob/master/README.md#thread-pool)<br/> 
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Queue](https://github.com/mtrebi/thread-pool/blob/master/README.md#queue)<br/> 
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Submit function](https://github.com/mtrebi/thread-pool/blob/master/README.md#submit-function)  <br/> 
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Thread worker](https://github.com/mtrebi/thread-pool/blob/master/README.md#thread-worker)  <br/> 
&nbsp;[Usage example](https://github.com/mtrebi/thread-pool/blob/master/README.md#usage-example)  <br/> 
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Use case#1](https://github.com/mtrebi/thread-pool#use-case-1)  <br/> 
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Use case#2](https://github.com/mtrebi/thread-pool#use-case-2)  <br/> 
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Use case#3](https://github.com/mtrebi/thread-pool#use-case-3)  <br/> 
&nbsp;[Future work](https://github.com/mtrebi/thread-pool/blob/master/README.md#future-work)  <br/> 
&nbsp;[References](https://github.com/mtrebi/thread-pool/blob/master/README.md#references)  <br/> 

# Introduction

A [thread pool](https://en.wikipedia.org/wiki/Thread_pool) is a technique that allows developers to exploit the concurrency of modern processors in an **easy** and **efficient** manner. It's easy because you send "work" to the pool and somehow this work gets done without blocking the main thread. It's efficient because threads are not initialized each time we want work to be done. Threads are initialized once and remain inactive until some work has to be done. This way we minimize the overhead.

There are many many Thread pool implementations in C++, many of them are probably better (safer, faster...) than mine. However,  I belive my implementation is **very straightforward and easy to understand**. 

# Build instructions

This project has been developed using Netbeans and Linux but it should work on Windows, MAC OS and Linux. It can be easily build using CMake and different generators. The following code can be used to generate the VS 2017 project files:

```c
// VS 2017
cd <project-folder>
mkdir build
cd build/
cmake .. "Visual Studio 15 2017 Win64"
```

Then, from VS you can edit and execute the project. Make sure that __main project is set up as the startup project__

If you are using Linux, you need to change the generator (use the default) and execute an extra operation to actually make the executable:

```c
// Linux
cd <project-folder>
mkdir build
cd build/
cmake ..
make
```

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

## Submit function

The most important method of the Thread Pool is the one responsible of adding work to the queue. I called this method **submit**. It's not difficult to understand how it works but its implementation can seem scary at first. Let's think about **what** should do and after that we will worry about **how** to do it. What:
* Accept any function with any parameters.
* Return "something" immediately to avoid blocking main thread. This returned object should **eventually** contain the result of the operation.

Cool, let's see **how** we can implement it.

### Submit implementation 

The complete submit functions looks like this:

```c
// Submit a function to be executed asynchronously by the pool
template<typename F, typename...Args>
auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
	// Create a function with bounded parameters ready to execute
	std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
	// Encapsulate it into a shared ptr in order to be able to copy construct / assign 
	auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);

	// Wrap packaged task into void function
	std::function<void()> wrapper_func = [task_ptr]() {
	  (*task_ptr)(); 
	};

	// Enqueue generic wrapper function
	m_queue.enqueue(wrapperfunc);

	// Wake up one thread if its waiting
	m_conditional_lock.notify_one();

	// Return future from promise
	return task_ptr->get_future();
}
```

Nevertheless, we're going to inspect line by line what's going on in order to fully understand how it works. 

#### Variadic template function

```c
template<typename F, typename...Args>
``` 

This means that the next statemnet is templated. The first template parameter is called F (our function) and second one is a parameter pack. A parameter pack is a special template parameter that can accept zero or more template arguments. It is, in fact, a way to express a variable number of arguments in a template. A template with at least one parameter pack is called **variadic template**

Summarazing, we are telling the compiler that our submit function is going to take one generic parameter of type F (our function) and a parameter pack Args (the parameters of the function F).

#### Function declaration

```c
auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
``` 

This may seem weird but, it's not. A function, in fact, can be declared using two different syntaxes. The following is the most well known:

```c
return-type identifier ( argument-declarations... )
``` 

But, we can also declare the function like this:

```c
auto identifier ( argument-declarations... ) -> return_type
 ```

Why to sintaxes? Well, imagine that you have a function that has a return type that depends on the input parameters of the function. Using the first syntax you can't declare that function without getting a compiler error since you  would be using a variable in the return type that has not been declared yet (because the return type declaration goes before the parameters type declaration). 

Using the second syntax you can declare the function to have return type **auto** then, using the -> you can declare the return type depending on the arguments of the functions that have been delcared previously. 

Now, let's inspect the parameters of the submit function. When the type of a parameter is declared as **T&&** for some deducted type T that parameter is a **universal reference**. This term was coined by [Scott Meyers](https://isocpp.org/blog/2012/11/universal-references-in-c11-scott-meyers) because **T&&** can also mean r-value reference. However, in the context of type deduction, it means that it can be bound to both l-values and r-values, unlike l-value references that can only be bound to non-const objects (they bind only to modifiable lvalues) and r-value references (they bind only to rvalues).


The return type of the function is of type **std::future<T>**. A std::future is a special type that provides a mechanism to access the result of asynchronous operations, in our case, the result of executing a specific function. This makes sense with what we said earlier.

Finally, the template type of std::future is **decltype(f(args...))**. Decltype is a special C++ keywoard that inspects the declared type of an entity or the type and value category of an expression. In our case, we want to know the return type of the function _f_, so we give decltype our generic function _f_ and the parameter pack _args_.

#### Function body

```c
// Create a function with bounded parameters ready to execute
std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
``` 

There are many many things happening here. First of all, the **std::bind(F, Args)** is a function that creates a wrapper for F with the given Args. Caling this wrapper is the same as calling F with the Args that it has been bound. Here, we are simply calling bind with our generic function _f_ and the parameterc pack _args_ but using another wrapper **std::forward<T>(t)** for each parameter. This second wrapper is needed to achieve perfect forwarding of universal references.
The result of this bind call is a **std::function<T>**. The std::function<T> is a C++ object that encapsulates a function. It allows you to execute the function as if it were a normal function calling the operator() with the required parameters BUT, because it is an object, you can store it, copy it and move it around. The template type of any std::function is the signature of that function: std::function< return-type (arguments)>. In this case, we already know how to get the return type of this function using decltype. But, what about the arguments? Well, because we bound all arguments _args_ to the function _f_ we just have to add an empty pair of parenthersis that represents an empty list of arguments: **decltype(f(args...))()**.


```c
// Encapsulate it into a shared ptr in order to be able to copy construct / assign 
auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
```

The next thing we do is we create a **std::packaged_task<T>(t)**.  A packaged_task is a wrapper around a function that can be executed asynchronously. It's result is stored in a shared state inside a std::future<T> object. The templated type T of a std::packaged_task<T>(t) is the type of the function _t_ that is wrapping. Because we said it before, the signature of the function _f_ is **decltype(f(args...))()** that is the same type of the packaged_task. Then, we just wrap again this packaged task inside a **std::shared_ptr** using the initialize function **std::make_shared**.

```c
// Wrap packaged task into void function
std::function<void()> wrapperfunc = [task_ptr]() {
  (*task_ptr)(); 
};

```

Again, we create a std:.function, but, note that this time its template type is **void()**. Independently of the function _f_ and its parameters _args_ this _wrapperfun_ the return type will always be **void**. Since all functions _f_ may have different return types, the only way to store them in a container (our Queue) is wrapping them with a generic void function. Here, we are just declaring this _wrapperfunc_ to execute the actual task _taskptr_ that will execute the bound function _func_.


```c
// Enqueue generic wrapper function
m_queue.enqueue(wrapperfunc);
```

We enqueue this _wrapperfunc_. 

```c
// Wake up one thread if its waiting
m_conditional_lock.notify_one();
```

Before finishing, we wake up one thread in case it is waiting.

```c
// Return future from promise
return task_ptr->get_future();
```

And finally, we return the future of the packaged_task. Because we are returning the future that is bound to the packaged_task _taskptr_ that, at the same time, is bound with the function _func_, executing this _taskptr_ will automatically update the future. Because we wrapped the execution of the _taskptr_ with a generic wrapper function, is the execution of _wrapperfunc_ that, in fact, updates the future. Aaaaand. since we enqueued this wrapper function, it will be executed by a thread after being dequeued calling the operator().


## Thread worker

Now that we understand how the submit method works, we're going to focus on how the work gets done. Probably, the simplest implementation of a thread worker could be using polling:

	 Loop
		If Queue is not empty
			Unqueue work
			Do it

This looks alright but it's **not very efficient**. Doyou see why? What would happen if there is no work in the Queue? The threads would keep looping and asking all the time: Is the queue empty? 

The more sensible implementation is done by "sleeping" the threads until some work is added to the queue. As we saw before, as soon as we enqueue work, a signal **notify_one()** is sent. This allows us to implement a more efficient algorithm:

	Loop
		If Queue is empty
			Wait signal
		Unqueue work
		Do it

This signal system is implemented in C++ with **conditional variables**. Conditional variables are always bound to a mutex, so I added a mutex to the Thread Pool class just to manage this. The final code of a worker looks like this: 

```c
void operator()() {
	std::function<void()> func;
	bool dequeued;
	while (!m_pool->m_shutdown) {
	{
		std::unique_lock<std::mutex> lock(m_pool->m_conditional_mutex);
		if (m_pool->m_queue.empty()) {
			m_pool->m_conditional_lock.wait(lock);
		}
		dequeued = m_pool->m_queue.dequeue(func);
	}
		if (dequeued) {
	  		func();
		}
	}	
}

```

The code is really easy to understand so I am not going to explain anything. The only thing to note here is that, _func_ is our wrapper function declared as:

```c
std::function<void()> wrapperfunc = [task_ptr]() {
  (*task_ptr)(); 
};

```

So, executing this function will automatically update the future.

# Usage example

Creating the Thread Pool is as easy as:

```c
// Create pool with 3 threads
ThreadPool pool(3);

// Initialize pool
pool.init();
```

When we want to shutdown the pool just call:

```c
// Shutdown the pool, releasing all threads
pool.shutdown()
```

Ff we want to send some work to the pool, after we have initialized it, we just have to call the submit function:

```c
pool.submit(work);
```

Depending on the type of work, I've distinguished different use-cases. Suppose that the work that we have to do is multiply two numbers. We can do it in many different ways. I've implemented the three most common ways to do it that I can imagine:
* Use-Case #1. Function returns the result
* Use-Case #2. Function updates by ref parameter with the result
* Use-Case #3. Function prints the result

_Note: This is just to show how the submit function works. Options are not exclusive_

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

Nice, when the work is finished by the Thread POol we know that the future will get updated and we can retrieve the result calling:
```c
const int result = future.get();
std::cout << result << std::endl;
```

The get() function of std::future<T> always return the type T of the future. **This type will always be equal to the return type of the function passed to the submit method**. In this case, int.

## Use-Case #2
The multiply function has a parameter passed by ref:

```c
// Simple function that adds multiplies two numbers and updates the out_res variable passed by ref
void  multiply(int& out_res, const int a, const int b) {
	out_res = a * b;
}
```

Now, we have to call the submit function with a subtle difference. Because we are using templates and type deduction (universal references), the parameter passed by ref needs to be called using **std::ref(param)** to make sure that we are passing it by ref and not by value.

```c
int result = 0;
auto future = pool.submit(multiply, std::ref(result), 2, 3);
// result is 0
future.get();
// result is 6
std::cout << result << std::endl;
```

In this case, what's the type of future? Well, as I said before, the return type will always be equal to the return type of the function passed to the submit method. Because this function is of type void, the future  is **std::future<void>**. Calling future.get() returns void. That's not very useful, but we still need to call .get() to make sure that the work has been done.

## Use-Case #3
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

In this case, we know that as soon as the multiplication is done it will be printed. If we care when this is done, we can wait for it calling future.get().

Checkout the [main](https://github.com/mtrebi/thread-pool/blob/master/src/main.cpp) program for a complete example.

# Future work

* Make it more reliable and safer (exceptions)
* Find a better way to use it with member functions (thanks to @rajenk)
* Run benchmarks and improve performance if needed
 * Evaluate performance and impact of std::function in the heap and try alternatives if necessary. (thanks to @JensMunkHansen) 

# References

* [MULTI-THREADED PROGRAMMING TERMINOLOGY - 2017](http://www.bogotobogo.com/cplusplus/multithreaded.php): Fast analysis of how a multi-thread system works

* [Universal References in C++11—Scott Meyers](https://isocpp.org/blog/2012/11/universal-references-in-c11-scott-meyers): Universal references in C++11 by Scott Meyers

* [Perfect forwarding and universal references in C++](http://eli.thegreenplace.net/2014/perfect-forwarding-and-universal-references-in-c/): Article about how and when to use perfect forwarding and universal references

* [C++ documentation](http://www.cplusplus.com/reference/): Thread, conditional variables, mutex and many others...
