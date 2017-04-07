#pragma once

#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>
#include <queue>

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
      /*
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
      }*/
      
      std::function<void()> f;
      while (!m_pool->m_shutdown) {
        {
          std::unique_lock<std::mutex> lock(m_pool->m_mutex_queue);
          if (m_pool->queue.empty()) {
            m_pool->m_conditional_lock.wait(lock);
          }
          f = m_pool->queue.front();
          m_pool->queue.pop();
        } // Release of m_mutex_queue
        if (f) {
          f();
          int a = 2;
        }
      }


    }

  };

  bool m_shutdown;
  TasksQueue m_queue;
  std::vector<std::thread> m_threads;
  std::queue<std::function<void()>> queue;
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
    m_queue.enqueue(*task);
    m_conditional_lock.notify_one();
  } 


  template<typename F, typename...Args>
  void submit(F&& f, Args&&... args) {
    //TODO std::queue of functions
    std::function<void()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    queue.push(func);
    int a = 2;
  }


  /*
  template<typename F, typename ... Args>
  //auto
   void
  submit(F && f, Args && ... args) -> std::future<decltype(f)> {
    //packaged_task
    using return_type = typename std::result_of<F(Args...)>::type;
    
    std::packaged_task<decltype(f)> task(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );


    auto _f = new std::function<void()>()
      // Bind function with parameters -> Only call bind
      ;

    //auto promise = std::packaged_task<decltype(f)
    //auto a = std::forward<FunctorType>(f);
    //auto function = std::function <void(int, int)>(a);
    //auto future = task->get_future();
    //return future;
  }
  */
  /*
  template<typename FunctorType>
  auto submit(FunctorType && f) -> std::future<decltype(f)> {
  //std::forward<FunctorType>(f)
  return nullptr;
  }
  */
};
