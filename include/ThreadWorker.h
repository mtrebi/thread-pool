#pragma once

#include "Task.h"
#include "ThreadPool.h"

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
    while (true) {
      {
        std::unique_lock<std::mutex> lock(m_pool->m_mutex_queue);
        if (m_pool->m_queue.is_empty()) {
          m_pool->m_cv.wait(lock);
        }
        task = m_pool->get_task();
      } // Release of m_mutex_queue
      task->execute();
    }
  }

};