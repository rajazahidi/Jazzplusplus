#ifndef JAZZ_AUDIO_GRAPH_SCHEDULER_H
#define JAZZ_AUDIO_GRAPH_SCHEDULER_H

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <cstdint>

namespace Jazz {

enum class AudioNodeType {
    SOURCE_GENERATOR,
    TRACK_CHANNEL,
    AUX_BUS,
    MASTER_BUS
};

struct AudioGraphNode {
    uint32_t id;
    std::string name;
    AudioNodeType type;
    int processingLatencySamples;
    int executionLevel; // Calculated DAG depth level (0 = source/leaves, higher = buses)
    std::vector<uint32_t> dependencies; // Nodes that MUST execute before this node
    std::vector<uint32_t> dependents;   // Nodes that depend on this node's output
    bool isMuted;
};

class JZAudioGraphScheduler {
public:
    JZAudioGraphScheduler(int maxThreads = 8);
    ~JZAudioGraphScheduler() = default;

    // Graph Construction
    void clearGraph();
    uint32_t addNode(const std::string& name, AudioNodeType type, int latencySamples = 0);
    bool removeNode(uint32_t nodeId);
    bool addEdge(uint32_t sourceNodeId, uint32_t destinationNodeId); // Destination depends on Source
    bool removeEdge(uint32_t sourceNodeId, uint32_t destinationNodeId);

    // Threading Configuration
    void setThreadCount(int threadCount);
    int getThreadCount() const { return m_threadCount; }

    // Topological Sort & Parallel Level Partitioning
    bool compileGraph(); // Detects cycles, assigns DAG levels, prepares schedule
    bool hasCycle() const { return m_hasCycle; }
    int getMaxLevel() const { return m_maxLevel; }
    size_t getNodeCount() const { return m_nodes.size(); }

    const std::vector<uint32_t>& getExecutionOrder() const { return m_executionOrder; }
    const std::vector<std::vector<uint32_t>>& getParallelLevels() const { return m_parallelLevels; }

    // Node Accessors
    const AudioGraphNode* getNode(uint32_t nodeId) const;
    void setNodeMute(uint32_t nodeId, bool mute);
    int calculateTotalGraphLatency() const;

private:
    int m_threadCount;
    bool m_hasCycle;
    int m_maxLevel;
    uint32_t m_nextId;

    std::unordered_map<uint32_t, AudioGraphNode> m_nodes;
    std::vector<uint32_t> m_executionOrder;
    std::vector<std::vector<uint32_t>> m_parallelLevels; // Grouped by DAG level for parallel execution

    bool detectCyclesAndAssignLevels();
};

} // namespace Jazz

#endif // JAZZ_AUDIO_GRAPH_SCHEDULER_H
