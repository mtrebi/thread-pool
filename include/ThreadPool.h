#pragma once

#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>
#include <chrono>

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
			std::function<void()> func;
			bool dequeued;
			
			while (!m_pool->m_shutdown) {
				{
					std::unique_lock<std::mutex> lock(m_pool->m_conditional_mutex);

					while (m_pool->m_queue.empty() && !m_pool->m_shutdown)
					{
						
						if (m_pool->wait_exit_thr_num > 0)
						{
							m_pool->wait_exit_thr_num--;

							if (m_pool->live_thr_num > m_pool->min_thr_num)
							{
								m_pool->live_thr_num--;
								return;
							}
						}
						else
						{
							m_pool->m_conditional_lock.wait(lock);
						}
					}		
				}

				dequeued = m_pool->m_queue.dequeue(func);

				if (dequeued) {
					m_pool->busy_thr_num++;
					func();
					m_pool->busy_thr_num--;
				}
			}
			
		}
	};

	//
	void adjust_thread(void)
	{
		while (!m_shutdown)
		{
			std::this_thread::sleep_for(period);
			std::unique_lock<std::mutex> lock(m_conditional_mutex);
			int queue_size = m_queue.size();
			int live_thr_num = this->live_thr_num;
			lock.unlock();
			int busy_thr_num = this->busy_thr_num;

			if (queue_size >= min_wait_task_num && live_thr_num < max_thr_num) {
				lock.lock();
				int add = 0;
				for (int i = 0; i < max_thr_num && add < default_thread_vary
					&& live_thr_num < max_thr_num; i++) {
					if (m_threads[i].get_id() == std::thread::id()) {
						m_threads[i] = std::thread(ThreadWorker(this, i));
						add++;
						this->live_thr_num++;
					}
				}
				lock.unlock();
			}

			if ((busy_thr_num * 2) < live_thr_num && live_thr_num > min_thr_num) {
				lock.lock();
				wait_exit_thr_num = default_thread_vary;
				lock.unlock();
				m_conditional_lock.notify_all();
			}

		}
	}

	const int min_wait_task_num = 10;
	const int default_thread_vary = 10;
	const int min_thr_num;
	//Maximum number of threads
	const int max_thr_num;	
	int live_thr_num;
	int wait_exit_thr_num;
	//Number of busy threads
	std::atomic<int> busy_thr_num;		
	bool m_shutdown;
	SafeQueue<std::function<void()>> m_queue;
	std::vector<std::thread> m_threads;
	std::mutex m_conditional_mutex;
	std::condition_variable m_conditional_lock;
	std::chrono::seconds period{5};
	std::thread adjustthread;
public:
	ThreadPool(const int min_thr_num,const int max_thr_num)
		: m_threads(std::vector<std::thread>(max_thr_num)),  min_thr_num(min_thr_num),
		max_thr_num(max_thr_num), live_thr_num(min_thr_num), m_shutdown(false), wait_exit_thr_num(0){
	}

	ThreadPool(const ThreadPool &) = delete;
	ThreadPool(ThreadPool &&) = delete;

	ThreadPool & operator=(const ThreadPool &) = delete;
	ThreadPool & operator=(ThreadPool &&) = delete;

	// Inits thread pool
	void init() {
		for (int i = 0; i < min_thr_num; ++i) {
			m_threads[i] = std::thread(ThreadWorker(this, i));
		}
		adjustthread = std::move(std::thread(&ThreadPool::adjust_thread, this));
	}

	// Waits until threads finish their current task and shutdowns the pool
	void shutdown() {
		m_shutdown = true;
		if (adjustthread.joinable()) {
			adjustthread.join();
		}

		m_conditional_lock.notify_all();

		for (size_t i = 0; i < m_threads.size(); ++i) {
			if (m_threads[i].joinable()) {
				m_threads[i].join();
			}
		}
	}

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
		m_queue.enqueue(wrapper_func);

		// Wake up one thread if its waiting
		m_conditional_lock.notify_one();

		// Return future from promise
		return task_ptr->get_future();
	}

};

