/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.
*******************************************************************************/

#pragma once

#include "common/EventList.h"
#include "common/Type.h"
#include <queue> // Include for std::priority_queue

namespace NetworkAnalytical {

/**
 * EventQueue manages scheduled EventLists.
 */
class EventQueue {
  public:
    /**
     * Constructor.
     */
    EventQueue() noexcept;

    /**
     * Get current event time of the event queue.
     *
     * @return current event time
     */
    [[nodiscard]] EventTime get_current_time() const noexcept;

    /**
     * Check all registered events are invoked.
     * i.e., check if the event queue is empty.
     *
     * @return true if the event queue is empty, false otherwise
     */
    [[nodiscard]] bool finished() const noexcept;

    /**
     * Proceed the event queue.
     * i.e., first update the current event time to the next registered event
     * time, and then invoke all events registered at the current updated event
     * time.
     */
    void proceed() noexcept;

    /**
     * Schedule an event with a given event time.
     *
     * @param event_time time of event
     * @param callback callback function pointer
     * @param callback_arg argument of the callback function
     */
    void schedule_event(EventTime event_time, Callback callback, CallbackArg callback_arg) noexcept;

  private:
    /// current time of the event queue
    EventTime current_time;

    /// priority queue of EventLists
    struct EventComparator {
        bool operator()(const EventList& a, const EventList& b) {
            return a.get_event_time() > b.get_event_time();
        }
    };

    std::priority_queue<EventList, std::vector<EventList>, EventComparator> event_queue;
};

}  // namespace NetworkAnalytical
