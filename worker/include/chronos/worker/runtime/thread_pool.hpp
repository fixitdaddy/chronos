#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <stdexcept>
#include <vector>

namespace chronos::worker::runtime {

class ThreadPool {
 public:
  explicit ThreadPool(std::size_t size);
  ~ThreadPool();

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  template <typename Func>
  auto Submit(Func&& fn) -> std::future<decltype(fn())> {
    using ReturnType = decltype(fn());

    auto task = std::make_shared<std::packaged_task<ReturnType()>>(std::forward<Func>(fn));
    std::future<ReturnType> result = task->get_future();

    {
      std::lock_guard<std::mutex> lock(mu_);
      if (stopping_) {
        throw std::runtime_error("thread pool is stopping");
      }
      queue_.emplace([task]() { (*task)(); });
      queued_tasks_.fetch_add(1);
    }

    cv_.notify_one();
    return result;
  }

  void Shutdown(bool drain);

  [[nodiscard]] std::size_t QueueDepth() const;
  [[nodiscard]] std::size_t ActiveWorkers() const;

 private:
  void WorkerLoop();

  mutable std::mutex mu_;
  std::condition_variable cv_;
  std::queue<std::function<void()>> queue_;
  std::vector<std::thread> threads_;
  bool stopping_{false};
  bool drain_{true};
  std::atomic<std::size_t> active_workers_{0};
  std::atomic<std::size_t> queued_tasks_{0};
};

}  // namespace chronos::worker::runtime
