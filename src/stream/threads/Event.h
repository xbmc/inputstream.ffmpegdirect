/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "Condition.h"
#include "SingleLock.h"

#include <initializer_list>
#include <memory>
#include <vector>

// forward declare the CEventGroup
namespace FFmpegDirectThreads
{
  class CEventGroup;
}


/**
 * This is an Event class built from a ConditionVariable. The Event adds the state
 * that the condition is gating as well as the mutex/lock.
 *
 * This Event can be 'interruptible' (even though there is only a single place
 * in the code that uses this behavior).
 *
 * This class manages 'spurious returns' from the condition variable.
 */
class CEvent
{
  bool manualReset;
  volatile bool signaled;
  unsigned int numWaits = 0;

  CCriticalSection groupListMutex; // lock for the groups list
  std::unique_ptr<std::vector<FFmpegDirectThreads::CEventGroup*>> groups;

  /**
   * To satisfy the TightConditionVariable requirements and allow the
   *  predicate being monitored to include both the signaled and interrupted
   *  states.
   */
  FFmpegDirectThreads::ConditionVariable actualCv;
  FFmpegDirectThreads::TightConditionVariable<volatile bool&> condVar;
  CCriticalSection mutex;

  friend class FFmpegDirectThreads::CEventGroup;

  void addGroup(FFmpegDirectThreads::CEventGroup* group);
  void removeGroup(FFmpegDirectThreads::CEventGroup* group);

  // helper for the two wait methods
  inline bool prepReturn() { bool ret = signaled; if (!manualReset && numWaits == 0) signaled = false; return ret; }

  CEvent(const CEvent&) = delete;
  CEvent& operator=(const CEvent&) = delete;

public:
  inline CEvent(bool manual = false, bool signaled_ = false) :
    manualReset(manual), signaled(signaled_), condVar(actualCv,signaled) {}

  inline void Reset() { CSingleLock lock(mutex); signaled = false; }
  void Set();

  /** Returns true if Event has been triggered and not reset, false otherwise. */
  inline bool Signaled() { CSingleLock lock(mutex); return signaled; }

  /**
   * This will wait up to 'milliSeconds' milliseconds for the Event
   *  to be triggered. The method will return 'true' if the Event
   *  was triggered. Otherwise it will return false.
   */
  inline bool WaitMSec(unsigned int milliSeconds)
  { CSingleLock lock(mutex); numWaits++; condVar.wait(mutex,milliSeconds); numWaits--; return prepReturn(); }

  /**
   * This will wait for the Event to be triggered. The method will return
   * 'true' if the Event was triggered. If it was either interrupted
   * it will return false. Otherwise it will return false.
   */
  inline bool Wait()
  { CSingleLock lock(mutex); numWaits++; condVar.wait(mutex); numWaits--; return prepReturn(); }

  /**
   * This is mostly for testing. It allows a thread to make sure there are
   *  the right amount of other threads waiting.
   */
  inline int getNumWaits() { CSingleLock lock(mutex); return numWaits; }

};

namespace FFmpegDirectThreads
{
  /**
   * CEventGroup is a means of grouping CEvents to wait on them together.
   * It is equivalent to WaitOnMultipleObject that returns when "any" Event
   * in the group signaled.
   */
  class CEventGroup
  {
    std::vector<CEvent*> events;
    CEvent* signaled{};
    FFmpegDirectThreads::ConditionVariable actualCv;
    FFmpegDirectThreads::TightConditionVariable<CEvent*&> condVar{actualCv, signaled};
    CCriticalSection mutex;

    unsigned int numWaits{0};

    // This is ONLY called from CEvent::Set.
    inline void Set(CEvent* child) { CSingleLock l(mutex); signaled = child; condVar.notifyAll(); }

    friend class ::CEvent;

    CEventGroup(const CEventGroup&) = delete;
    CEventGroup& operator=(const CEventGroup&) = delete;

  public:
    /**
     * Create a CEventGroup from a number of CEvents.
     */
    CEventGroup(std::initializer_list<CEvent*> events);

    ~CEventGroup();

    /**
     * This will block until any one of the CEvents in the group are
     * signaled at which point a pointer to that CEvents will be
     * returned.
     */
    CEvent* wait();

    /**
     * This will block until any one of the CEvents in the group are
     * signaled or the timeout is reached. If an event is signaled then
     * it will return a pointer to that CEvent, otherwise it will return
     * NULL.
     */
    CEvent* wait(unsigned int milliseconds);

    /**
     * This is mostly for testing. It allows a thread to make sure there are
     *  the right amount of other threads waiting.
     */
    inline int getNumWaits() { CSingleLock lock(mutex); return numWaits; }

  };
}
