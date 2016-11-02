// Copyright (c) 2014, Facebook, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
//
#pragma once

#include <functional>
#include <memory>

namespace rocketspeed {

/**
 * Coerces a void* to T* and deletes. This is undefined behaviour if the
 * pointee's type is not T (or derived from T).
 */
template <typename T>
void Deleter(void* p) {
  delete static_cast<T*>(p);
}

/**
 * Performs type-erasure on a std::unique_ptr<T> by converting it to a
 * std::unique_ptr<void*, void(void*)> with a deleter that deletes it as a T*.
 *
 * This is useful when a uniform interface is needed with support for
 * heterogeneous types, but the exact type itself is unneeded and unknown
 * beforehand.
 *
 * @param p The object to be type-erased.
 */
template <typename T>
std::unique_ptr<void, void(*)(void*)> EraseType(std::unique_ptr<T> p) {
  return std::unique_ptr<void, void(*)(void*)>(
    static_cast<void*>(p.release()), &Deleter<T>);
}

/**
 * A functor that destroys provided object when invoked.
 */
template <typename T>
class DeferredDeleter {
 public:
  explicit DeferredDeleter(std::unique_ptr<T> object)
  : object_(std::move(object)) {}

  DeferredDeleter(const DeferredDeleter& other)
  : object_(std::move(other.object_)) {}

  DeferredDeleter(DeferredDeleter&& other)
  : object_(std::move(other.object_)) {}

  void operator()() const { object_.reset(); }

 private:
  mutable std::unique_ptr<T> object_;
};

/**
 * Creates a functor, which destroys provided object when executed.
 *
 * @param object An object wrapped in a unique_ptr.
 * @return A functor which destroys provided object on first execution.
 */
template <typename T>
DeferredDeleter<T> MakeDeferredDeleter(std::unique_ptr<T>& object) {
  return DeferredDeleter<T>(std::move(object));
}

}  // namespace rocketspeed
