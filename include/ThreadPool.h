#pragma once

#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

#include "SafeQueue.h"

class ThreadPool {
private:
  class ThreadWorker {
  private:
    int m_id;
    ThreadPool * m_pool;
  public:
    ThreadWorker(ThreadPool * pool, const int id)
      : m_pool(pool), m_id(id) {
    }

    void operator()() {
      std::packaged_task<void()> task;
      bool empty;
      while (!m_pool->m_shutdown) {
        {
          std::unique_lock<std::mutex> lock(m_pool->m_mutex_queue);
          if (m_pool->m_queue.empty()) {
            m_pool->m_conditional_lock.wait(lock);
          }
          empty = m_pool->m_queue.dequeue(task);
        }
        if (!empty) {
          task();
        }
      }
    }
  };

  bool m_shutdown;
  SafeQueue<std::packaged_task<void()>> m_queue;
  std::vector<std::thread> m_threads;
  std::mutex m_mutex_queue;
  std::condition_variable m_conditional_lock;
public:
  ThreadPool(const int n_threads)
    : m_threads(std::vector<std::thread>(n_threads)), m_shutdown(false) {
  }

  ThreadPool(const ThreadPool &) = delete;
  ThreadPool(ThreadPool &&) = delete;

  ThreadPool & operator=(const ThreadPool &) = delete;
  ThreadPool & operator=(ThreadPool &&) = delete;

  // Inits thread pool
  void init() {
    for (int i = 0; i < m_threads.size(); ++i) {
      m_threads[i] = std::thread(ThreadWorker(this, i));
    }
  }

  // Waits until threads finish their current task and shutdowns the pool
  void shutdown() {
    m_shutdown = true;
    m_conditional_lock.notify_all();
    
    for (int i = 0; i < m_threads.size(); ++i) {
      m_threads[i].join();
    }
  }

  // Submit a function to be executed asynchronously by the pool
  template<typename F, typename...Args>
  auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
    // Create a function with bounded parameters ready to execute
    std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    // Encapsulate function inside task and promise to return a future
    std::packaged_task<decltype(f(args...))()> task(func);

    m_queue.enqueue(std::move(task));
    m_conditional_lock.notify_one();

    return task.get_future();
  }
};
