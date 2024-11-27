/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.
*******************************************************************************/
/*
#include "common/EventQueue.h"
#include "common/NetworkParser.h"
#include "congestion_aware/Chunk.h"
#include "congestion_aware/Helper.h"
#include <iostream>

using namespace NetworkAnalytical;
using namespace NetworkAnalyticalCongestionAware;

void chunk_arrived_callback(void* const event_queue_ptr) {
    // typecast event_queue_ptr
    auto* const event_queue = static_cast<EventQueue*>(event_queue_ptr);

    // print chunk arrival time
    const auto current_time = event_queue->get_current_time();
    std::cout << "A chunk arrived at destination at time: " << current_time << " ns" << std::endl;
}

int main() {
    // Instantiate shared resources
    const auto event_queue = std::make_shared<EventQueue>();
    Topology::set_event_queue(event_queue);

    // Parse network config and create topology
    const auto network_parser = NetworkParser("../input/Ring.yml");
    const auto topology = construct_topology(network_parser);
    const auto npus_count = topology->get_npus_count();
    const auto devices_count = topology->get_devices_count();

    // message settings
    const auto chunk_size = 1'048'576;  // 1 MB

    // Run All-Gather
    for (int i = 0; i < npus_count; i++) {
        for (int j = 0; j < npus_count; j++) {
            if (i == j) {
                continue;
            }

            // crate a chunk
            auto route = topology->route(i, j);
            auto* event_queue_ptr = static_cast<void*>(event_queue.get());
            auto chunk = std::make_unique<Chunk>(chunk_size, route, chunk_arrived_callback, event_queue_ptr);

            // send a chunk
            topology->send(std::move(chunk));
        }
    }

    // Run simulation
    while (!event_queue->finished()) {
        event_queue->proceed();
    }

    // Print simulation result
    const auto finish_time = event_queue->get_current_time();
    std::cout << "Total NPUs Count: " << npus_count << std::endl;
    std::cout << "Total devices Count: " << devices_count << std::endl;
    std::cout << "Simulation finished at time: " << finish_time << " ns" << std::endl;

    return 0;
}
*/

#include "common/EventQueue.h"
#include "common/NetworkParser.h"
#include "congestion_aware/Chunk.h"
#include "congestion_aware/Helper.h"
#include <iostream>
#include <unordered_map>
#include <vector>
#include <cassert>

using namespace NetworkAnalytical;
using namespace NetworkAnalyticalCongestionAware;

// Global variables
std::unordered_map<int, std::vector<int>> node_buffers;
std::unordered_map<int, int> reduction_progress;
std::shared_ptr<Topology> topology; // Make topology global

const int chunks_per_packet = 4;
const int chunk_size = 256 * 1024;

// Forward declarations
void trigger_all_gather(int node_id, EventQueue* event_queue);
void process_reduction(int node_id, EventQueue* event_queue);

// Callback for logging Reduce-Scatter chunk arrivals
void chunk_arrived_callback(void* const event_queue_ptr) {
    auto* const event_queue = static_cast<EventQueue*>(event_queue_ptr);
    assert(event_queue != nullptr);

    auto* chunk = static_cast<Chunk*>(event_queue_ptr);
    assert(chunk != nullptr);

    // Get the current device and next device IDs
    auto current_device = chunk->current_device();
    auto next_device = chunk->next_device();

    const auto current_time = event_queue->get_current_time();

    // Log details with protection against nullptr
    std::cout << "[Reduce-Scatter] Chunk arrived at Node " << current_device->get_id();
    if (next_device) {
        std::cout << " and will move to Node " << next_device->get_id();
    } else {
        std::cout << " and has no next destination (end of route).";
    }
    std::cout << " at time: " << current_time << " ns." << std::endl;
}

// Callback for logging All-Gather chunk arrivals
void all_gather_chunk_arrived_callback(void* const event_queue_ptr) {
    auto* const event_queue = static_cast<EventQueue*>(event_queue_ptr);
    assert(event_queue != nullptr);

    auto* chunk = static_cast<Chunk*>(event_queue_ptr);
    assert(chunk != nullptr);

    // Get the current device and next device IDs
    auto current_device = chunk->current_device();
    auto next_device = chunk->next_device();

    const auto current_time = event_queue->get_current_time();

    // Log details with protection against nullptr
    std::cout << "[All-Gather] Chunk arrived at Node " << current_device->get_id();
    if (next_device) {
        std::cout << " and will move to Node " << next_device->get_id();
    } else {
        std::cout << " and has no next destination (end of route).";
    }
    std::cout << " at time: " << current_time << " ns." << std::endl;
}

// Trigger All-Gather for a node
void trigger_all_gather(int node_id, EventQueue* event_queue) {
    for (int dest = 0; dest < node_buffers.size(); dest++) {
        if (dest == node_id) continue;
    
        auto route = topology->route(node_id, dest);
        auto* event_queue_ptr = static_cast<void*>(event_queue);
    
        auto chunk = std::make_unique<Chunk>(
            chunk_size, route, all_gather_chunk_arrived_callback, event_queue_ptr);
        chunk->data = node_buffers[node_id][0]; // Assign reduced value to chunk
    
        std::cout << "[All-Gather] Sending reduced result from Node " << node_id
                  << " to Node " << dest << " with value " << chunk->data
                  << " at time: " << event_queue->get_current_time() << " ns." << std::endl;
    
        topology->send(std::move(chunk));
    }
}

// Process reduction progress
void process_reduction(int node_id, EventQueue* event_queue) {
    node_buffers[node_id][0] += 1;

    reduction_progress[node_id]++;
    if (reduction_progress[node_id] == chunks_per_packet) {
        if (node_buffers[node_id][0] == chunks_per_packet) {
            const auto current_time = event_queue->get_current_time();
            std::cout << "Node " << node_id << " completed reduction. Fully reduced value: "
                      << node_buffers[node_id][0] << " at time: " << current_time << " ns" << std::endl;

            trigger_all_gather(node_id, event_queue);
        }
    }
}

int main() {
    const auto event_queue = std::make_shared<EventQueue>();
    Topology::set_event_queue(event_queue);

    const auto network_parser = NetworkParser("../input/FullyConnected.yml");
    topology = construct_topology(network_parser);
    const auto npus_count = topology->get_npus_count();
    const auto devices_count = topology->get_devices_count();

    for (int i = 0; i < npus_count; i++) {
        node_buffers[i] = std::vector<int>(1, 0);
        reduction_progress[i] = 0;
    }

    for (int i = 0; i < npus_count; i++) {
        for (int j = 0; j < npus_count; j++) {
            if (i == j) continue;
    
            for (int chunk_id = 0; chunk_id < chunks_per_packet; chunk_id++) {
                auto route = topology->route(i, j);
                auto* event_queue_ptr = static_cast<void*>(event_queue.get());
    
                auto chunk = std::make_unique<Chunk>(chunk_size, route, chunk_arrived_callback, event_queue_ptr);
    
                std::cout << "[Reduce-Scatter] Sending chunk from Node " << i
                          << " to Node " << j << " with size " << chunk_size << " bytes." << std::endl;
    
                topology->send(std::move(chunk));
                process_reduction(j, event_queue.get());
            }
        }
    }

    while (!event_queue->finished()) {
        event_queue->proceed();
    }

    const auto finish_time = event_queue->get_current_time();
    std::cout << "Total NPUs Count: " << npus_count << std::endl;
    std::cout << "Total devices Count: " << devices_count << std::endl;
    std::cout << "Simulation finished at time: " << finish_time << " ns" << std::endl;

    return 0;
}

