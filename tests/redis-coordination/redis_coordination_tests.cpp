#include <chrono>
#include <iostream>
#include <thread>

#include "chronos/coordination/redis_coordination.hpp"

namespace {

bool TestLockExpiryAndRecovery() {
  chronos::coordination::InMemoryRedisCoordination redis;

  const auto a = redis.TryLock("lock:test", "owner-a", std::chrono::seconds(1));
  const auto b_while_held = redis.TryLock("lock:test", "owner-b", std::chrono::seconds(1));

  if (!a || b_while_held) {
    return false;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(1200));
  const auto b_after_expiry = redis.TryLock("lock:test", "owner-b", std::chrono::seconds(1));
  return b_after_expiry;
}

bool TestNetworkPartitionFailOpenBehavior() {
  chronos::coordination::InMemoryRedisCoordination redis;
  redis.SetAvailable(false);

  // Rate limiting and dedupe should fail-open to avoid correctness breakage.
  const auto rate_allowed = redis.TryConsumeRate("api", 1, std::chrono::seconds(10));
  const auto dedupe_allowed = redis.TryRegisterDedupe("dispatch:abc", std::chrono::seconds(10));

  // Locks fail closed by returning false; system should rely on DB correctness.
  const auto lock_ok = redis.TryLock("lock:test", "owner", std::chrono::seconds(10));

  return rate_allowed && dedupe_allowed && !lock_ok;
}

}  // namespace

int main() {
  const bool lock_expiry_ok = TestLockExpiryAndRecovery();
  const bool partition_ok = TestNetworkPartitionFailOpenBehavior();

  std::cout << "lock_expiry_recovery=" << lock_expiry_ok << "\n";
  std::cout << "partition_fail_open=" << partition_ok << "\n";

  return (lock_expiry_ok && partition_ok) ? 0 : 1;
}
