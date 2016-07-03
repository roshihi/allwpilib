/*----------------------------------------------------------------------------*/
/* Copyright (c) FIRST 2016. All Rights Reserved.                             */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/

#pragma once

#include <stdint.h>

#include <memory>
#include <vector>

#include "HAL/Handles.h"
#include "HAL/cpp/priority_mutex.h"
#include "HandlesInternal.h"

namespace hal {

/**
 * The LimitedClassedHandleResource class is a way to track handles. This
 * version
 * allows a limited number of handles that are allocated sequentially.
 *
 * @tparam THandle The Handle Type (Must be typedefed from HalHandle)
 * @tparam TStruct The struct type held by this resource
 * @tparam size The number of resources allowed to be allocated
 * @tparam enumValue The type value stored in the handle
 *
 */
template <typename THandle, typename TStruct, int16_t size,
          HalHandleEnum enumValue>
class LimitedClassedHandleResource {
  friend class LimitedClassedHandleResourceTest;

 public:
  LimitedClassedHandleResource(const LimitedClassedHandleResource&) = delete;
  LimitedClassedHandleResource operator=(const LimitedClassedHandleResource&) =
      delete;
  LimitedClassedHandleResource() = default;
  THandle Allocate(std::shared_ptr<TStruct> toSet);
  std::shared_ptr<TStruct> Get(THandle handle);
  void Free(THandle handle);

 private:
  std::shared_ptr<TStruct> m_structures[size];
  priority_mutex m_handleMutexes[size];
  priority_mutex m_allocateMutex;
};

template <typename THandle, typename TStruct, int16_t size,
          HalHandleEnum enumValue>
THandle
LimitedClassedHandleResource<THandle, TStruct, size, enumValue>::Allocate(
    std::shared_ptr<TStruct> toSet) {
  // globally lock to loop through indices
  std::lock_guard<priority_mutex> sync(m_allocateMutex);
  int16_t i;
  for (i = 0; i < size; i++) {
    if (m_structures[i] == nullptr) {
      // if a false index is found, grab its specific mutex
      // and allocate it.
      std::lock_guard<priority_mutex> sync(m_handleMutexes[i]);
      m_structures[i] = toSet;
      return (THandle)createHandle(i, enumValue);
    }
  }
  return HAL_INVALID_HANDLE;
}

template <typename THandle, typename TStruct, int16_t size,
          HalHandleEnum enumValue>
std::shared_ptr<TStruct> LimitedClassedHandleResource<
    THandle, TStruct, size, enumValue>::Get(THandle handle) {
  // get handle index, and fail early if index out of range or wrong handle
  int16_t index = getHandleTypedIndex(handle, enumValue);
  if (index < 0 || index >= size) {
    return nullptr;
  }
  std::lock_guard<priority_mutex> sync(m_handleMutexes[index]);
  // return structure. Null will propogate correctly, so no need to manually
  // check.
  return m_structures[index];
}

template <typename THandle, typename TStruct, int16_t size,
          HalHandleEnum enumValue>
void LimitedClassedHandleResource<THandle, TStruct, size, enumValue>::Free(
    THandle handle) {
  // get handle index, and fail early if index out of range or wrong handle
  int16_t index = getHandleTypedIndex(handle, enumValue);
  if (index < 0 || index >= size) return;
  // lock and deallocated handle
  std::lock_guard<priority_mutex> sync(m_allocateMutex);
  std::lock_guard<priority_mutex> lock(m_handleMutexes[index]);
  m_structures[index].reset();
}
}
