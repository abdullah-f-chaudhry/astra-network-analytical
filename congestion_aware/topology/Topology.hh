/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.
*******************************************************************************/

#pragma once

#include "common/event-queue/EventQueue.hh"
#include "congestion_aware/network/Chunk.hh"
#include "congestion_aware/network/Link.hh"
#include "congestion_aware/network/Node.hh"

using namespace NetworkAnalytical;

namespace NetworkAnalyticalCongestionAware {

class Topology {
 public:
  /**
   * Link the event queue to the Link class.
   *
   * @param event_queue The event queue to be linked.
   */
  static void set_event_queue(std::shared_ptr<EventQueue> event_queue) noexcept;

  /**
   * Construct a topology with the given number of npus.
   *
   * @param npus_count number of npus in the topology
   */
  explicit Topology(int npus_count) noexcept;

  /**
   * Construct the route from src to dest.
   * Route is a list of std::shared_ptr<Node>, including both src and dest.
   *
   * e.g., route(0, 3) = [0, 1, 2, 3]
   *
   * @param src src npu node_id
   * @param dest dest npu node_id
   *
   * @return route from src to dest
   */
  [[nodiscard]] virtual Route route(NodeId src, NodeId dest) const noexcept = 0;

  /**
   * Initiate the transmission of a chunk.
   *
   * @param chunk chunk to be transmitted
   */
  void send(std::unique_ptr<Chunk> chunk) noexcept;

 protected:
  /// number of npus in the topology
  int npus_count;

  /// vector of Node instances in the topology
  std::vector<std::shared_ptr<Node>> npus;

  /**
   * Connect src -> dest with the given bandwidth and latency.
   * (i.e., a `Link` gets constructed between the two npus)
   *
   * if bidirectional=true, dest -> src connection is also established.
   *
   * @param src src npu node_id
   * @param dest dest npu node_id
   * @param bandwidth bandwidth of link
   * @param latency latency of link
   * @param bidirectional true if connection is bidirectional, false otherwise
   */
  void connect(
      NodeId src,
      NodeId dest,
      Bandwidth bandwidth,
      Latency latency,
      bool bidirectional = true) noexcept;
};

} // namespace NetworkAnalyticalCongestionAware
