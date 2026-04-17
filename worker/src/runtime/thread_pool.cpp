#include "chronos/worker/runtime/thread_pool.hpp"

#include <stdexcept>
#include <utility>

namespace chronos::worker::runtime {

ThreadPool::ThreadPool(std::size_t size) {
  threads_.reserve(size);
  for (std::size_t i = 0; i < size; ++i) {
    threads_.emplace_back([this]() { WorkerLoop(); });
  }
}

ThreadPool::~ThreadPool() {
  Shutdown(true);
}

void ThreadPool::Shutdown(bool drain) {
  {
    std::lock_guard<std::mutex> lock(mu_);
    if (stopping_) {
      return;
    }
    stopping_ = true;
    drain_ = drain;
    if (!drain_) {
      std::queue<std::function<void()>> empty;
      std::swap(queue_, empty);
      queued_tasks_.store(0);
    }
  }

  cv_.notify_all();

  for (auto& t : threads_) {
    if (t.joinable()) {
      t.join();
    }
  }
}

std::size_t ThreadPool::QueueDepth() const {
  return queued_tasks_.load();
}

std::size_t ThreadPool::ActiveWorkers() const {
  return active_workers_.load();
}

void ThreadPool::WorkerLoop() {
  for (;;) {
    std::function<void()> task;

    {
      std::unique_lock<std::mutex> lock(mu_);
      cv_.wait(lock, [this]() { return stopping_ || !queue_.empty(); });

      if (stopping_ && (!drain_ || queue_.empty())) {
        return;
      }

      if (queue_.empty()) {
        continue;
      }

      task = std::move(queue_.front());
      queue_.pop();
      queued_tasks_.fetch_sub(1);
    }

    active_workers_.fetch_add(1);
    task();
    active_workers_.fetch_sub(1);
  }
}

}  // namespace chronos::worker::runtime
