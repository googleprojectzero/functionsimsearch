// A simple class for a C++11 thread pool. I know this should not be rewritten
// from scratch, but I did anyways - partially as a learning experience.
//
// Mostly a slightly amended rewrite of the techniques in Vitaliy Vitsentiy's
// thread pool code.
//
// Copyright September 2016, Thomas Dullien. Licensed under Apache License.

#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <atomic>
#include <condition_variable>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace threadpool {

// Simple synchronized queue.
template <typename T> class SynchronizedQueue {
  public:
    bool Push(T const& value) {
      std::lock_guard<std::mutex> lock(mutex_);
      queue_.push(value);
      return true;
    }
    bool Pop(T &value) {
      std::lock_guard<std::mutex> lock(mutex_);
      if (queue_.empty())
        return false;
      value = queue_.front();
      queue_.pop();
      return true;
    }
    bool Empty() {
      std::lock_guard<std::mutex> lock(mutex_);
      return queue_.empty();
    }
  private:
    std::queue<T> queue_;
    std::mutex mutex_;
};

class ThreadPool {
  public:
    ThreadPool() { Init(); };
    ThreadPool(uint32_t threads) { Init(); Resize(threads); };
    ~ThreadPool() {Stop(true);};
    size_t GetNumberOfThreads() { return threads_.size(); };
    size_t GetNumberOfIdleThreads() { return waiting_; };
    bool AllIdle() { return waiting_ == threads_.size(); };

    std::thread& GetThread(uint32_t index) { return *threads_[index]; };

    void Resize(uint32_t new_threads) {
      if (!is_stopped_ && !is_done_) {
        size_t old_threads = threads_.size();

        // Number of threads is to be increased.
        if (old_threads <= new_threads) {
          threads_.resize(new_threads);
          flags_.resize(new_threads);
          for (uint32_t index = old_threads; index < new_threads; ++index) {
            flags_[index] = std::make_shared<std::atomic<bool>>(false);
            SetThread(index);
          }
        } else { // Number of threads is to be decreased.
          for (uint32_t index = old_threads-1; index >= new_threads; --index) {
            // Tell the thread to finish itself.
            *flags_[index] = true;
            // Detach the thread. This is a little bit dangerous - it would be
            // better to get the thread into a joinable state.
            threads_[index]->detach();
          }
          {
            std::lock_guard<std::mutex> lock(mutex_);
            this->conditions_.notify_all();
          }
          this->threads_.resize(new_threads);
          // This seems dangerous in relation to the std::atomic resizing above?
          //
          this->flags_.resize(new_threads);
        }
      }
    }

    void ClearQueue() {
      std::function<void(int id)>* function;
      while (this->queue_.Pop(function)) {
        delete function;
      }
    }

    std::function<void(int)> Pop() {
      std::function<void(int id)>* function = nullptr;
      // Get the function pointer.
      queue_.Pop(function);
      // Wrap it in a unique_ptr to make sure it will be destroyed.
      std::unique_ptr<std::function<void(int id)>> function2(function);
      // Make a copy of the function and return it.
      std::function<void(int)> f;
      if (function2) {
        f = *function2;
      }
      return f;
    }

    void Stop(bool run_queues_until_empty = false) {
      if (!run_queues_until_empty) {
        if(is_stopped_) {
          return;
        }
        is_stopped_ = true;
        for (int index = 0, n = GetNumberOfThreads(); index < n; ++index) {
          *flags_[index] = true;  // Tell each thread to stop.
        }
        ClearQueue();
      } else {
        if (is_done_ || is_stopped_) {
          return;
        }
        is_done_ = true;
      }
      {
        std::lock_guard<std::mutex> lock(mutex_);
        conditions_.notify_all(); // Stop all waiting threads.
      }
      // Join all threads if they are in joinable state.
      for (size_t index = 0; index < threads_.size(); ++index) {
        if (threads_[index]->joinable()) {
          threads_[index]->join();
        }
      }
      ClearQueue();
      threads_.clear();
      flags_.clear();
    }

    // A template function that specializes for a given function with variable
    // arguments. Returns a future for the return type of that function.
    template<typename F, typename... Rest>
      auto Push(F&& function, Rest&&... rest)
        ->std::future<decltype(function(0, rest...))> {
      // Build a packaged task by binding the function arguments to the
      // function in question and making a shared pointer around it.
      auto package = std::make_shared<
        std::packaged_task<decltype(function(0, rest...))(int)>>(
            std::bind(std::forward<F>(function), std::placeholders::_1,
              std::forward<Rest>(rest)...));
      // Construct a new std::function from this package; use a lambda to
      // wrap it.
      auto _function = new std::function<void(int id)>([package](int id) {
          (*package)(id);});
      // Push it into the queue.
      queue_.Push(_function);
      {
        // Notify at least one thread to wake up.
        std::unique_lock<std::mutex> lock(mutex_);
        conditions_.notify_one();
        // Return the future.
        return package->get_future();
      }
    }

    template<typename F>
      auto Push(F&& function) -> std::future<decltype(function(0))> {
      auto package = std::make_shared<std::packaged_task<
        decltype(function(0))(int)>>(std::forward<F>(function));
      auto _function = new std::function<void(int id)>([package](int id) {
        (*package)(id);});
      queue_.Push(_function);
      {
        std::unique_lock<std::mutex> lock(mutex_);
        conditions_.notify_one();
        return package->get_future();
      }
    }

  private:
    // Delete copy constructors.
    ThreadPool(const ThreadPool&);
    ThreadPool(ThreadPool&&);
    ThreadPool& operator=(const ThreadPool&);
    ThreadPool& operator=(ThreadPool&&);

    // Creates a new thread at index i in the internal tracking array. The
    // thread itself runs a lambda which keeps on polling jobs.
    void SetThread(int i) {
      std::shared_ptr<std::atomic<bool>> flag(flags_[i]);
      // The lambda takes a copy of the shared pointer.
      auto function = [this, i, flag]() {
        std::atomic<bool>& _flag = *flag;
        std::function<void(int id)>* _function;
        bool is_pop = queue_.Pop(_function);
        while (true) {
          while (is_pop) { // There is still something in the queue.
            std::unique_ptr<std::function<void(int id)>> func(_function);
            (*_function)(i);
            if (_flag) {
              return; // The thread is supposed to stop.
            } else {
              is_pop = queue_.Pop(_function);
            }
          }
          {
            std::unique_lock<std::mutex> lock(mutex_);
            ++waiting_;
            // Wait until awoken. When woken, pop a new job from the queue,
            conditions_.wait(lock, [this, &_function, &is_pop, &_flag]() {
                is_pop = queue_.Pop(_function);
                return is_pop || is_done_ || _flag;
              });
            --waiting_;
            if (!is_pop) {
              return;
            }
          }
        }
        // This will never be reached, but is needed to make the compiler deduce
        // a bool return type for the lambda.
      };
      threads_[i].reset(new std::thread(function));
    }

    void Init(){ waiting_ = 0; is_stopped_ = false; is_done_ = false; }

    std::vector<std::unique_ptr<std::thread>> threads_;
    std::vector<std::shared_ptr<std::atomic<bool>>> flags_;
    SynchronizedQueue<std::function<void(int id)> *> queue_;
    std::atomic<bool> is_done_;
    std::atomic<bool> is_stopped_;
    std::atomic<int> waiting_;
    std::mutex mutex_;
    std::condition_variable conditions_;
};

} // namespace threadpool

#endif // THREADPOOL_HPP
