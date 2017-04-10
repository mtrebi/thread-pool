# Introduction

A [thread pool](https://en.wikipedia.org/wiki/Thread_pool) is a technique that allows developers to exploit the concurrency of modern processors in an **easy** and **efficient** manner. It's easy because you send "work" to the pool and somehow this work gets done without blocking the main thread. It's efficient because threads are not initialized each time we want work to be done. Threads are initialized once and remain inactive until some work has to be done. This way we minimize the overhead.

There are many many Thread pool implementations in C++, many of them are probably better (safer, faster...) than mine. However,  I belive my implementation is **very straightforward and easy to understand**. 

# Thread pool

The way that I understand things better is with images. So, lets take a look at the image of Thread Pool given by wikipedia:

<p align="center">  <img src="https://en.wikipedia.org/wiki/Thread_pool#/media/File:Thread_pool.svg"> </p>

As you can see, we have three important elements here:
* *Tasks Queue*. This is where the work that has to be done is stored.
* *Thread Pool*. This is set of threads (or workers) that continuously take work from the queue and do it.
* *Completed Tasks*. When the Thread has finished the work we return "something" to notify that the work has finished.

## Queue

We use a queue to store the work because it's the more sensible data structure. We want the work to be **started** in the same order that we sent it. However, this queue is a little bit **special**

## Thread worker

## Thread pool

# Sample code

# Future work

* Make it safer (exceptions)
* Research about submiting functions with return types
* Execute some benchmarks

# References
