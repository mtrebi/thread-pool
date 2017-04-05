#pragma once

#include <thread>
#include <mutex>
#include <vector>

#include "Task.h"
#include "TasksQueue.h"


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
      Task * task;
      while (!m_pool->m_shutdown) {
        {
          std::unique_lock<std::mutex> lock(m_pool->m_mutex_queue);
          if (m_pool->m_queue.is_empty()) {
            m_pool->m_conditional_lock.wait(lock);
          }
          task = m_pool->m_queue.dequeue();
        } // Release of m_mutex_queue
        if (task) {
          task->execute();
        }
      }
    }

  };

  bool m_shutdown;
  TasksQueue m_queue;
  std::vector<std::thread> m_threads;
  std::mutex m_mutex_queue;
  std::condition_variable m_conditional_lock;
public:
  ThreadPool(const int n_threads)
    : m_threads(std::vector<std::thread>(n_threads)), m_shutdown(false) {
  }

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

  //TODO: return future/promise
  // Subit tasks to be performed by the pool
  void submit(Task * task) {
    {
      std::unique_lock<std::mutex> lock(m_mutex_queue);
      m_queue.enqueue(*task);
    } // Release of m_mutex_queue
    m_conditional_lock.notify_one();
  } 
};
