#pragma once

#include "common/EventList.h"
#include "common/Type.h"
#include <queue>
#include <memory> // For std::shared_ptr

namespace NetworkAnalytical {

class EventQueue {
  public:
    EventQueue() noexcept;

    [[nodiscard]] EventTime get_current_time() const noexcept;
    [[nodiscard]] bool finished() const noexcept;

    void proceed() noexcept;
    void schedule_event(EventTime event_time, Callback callback, CallbackArg callback_arg) noexcept;

  private:
    EventTime current_time;

    struct EventComparator {
        bool operator()(const std::shared_ptr<EventList>& a, const std::shared_ptr<EventList>& b) {
            return a->get_event_time() > b->get_event_time();
        }
    };

    std::priority_queue<std::shared_ptr<EventList>, std::vector<std::shared_ptr<EventList>>, EventComparator> event_queue;
};

}  // namespace NetworkAnalytical
