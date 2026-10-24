#include "AudioGraphScheduler.h"
#include <queue>
#include <algorithm>

namespace Jazz {

JZAudioGraphScheduler::JZAudioGraphScheduler(int maxThreads)
    : m_threadCount(maxThreads > 0 ? maxThreads : 4)
    , m_hasCycle(false)
    , m_maxLevel(0)
    , m_nextId(1)
{
}

void JZAudioGraphScheduler::clearGraph()
{
    m_nodes.clear();
    m_executionOrder.clear();
    m_parallelLevels.clear();
    m_hasCycle = false;
    m_maxLevel = 0;
    m_nextId = 1;
}

uint32_t JZAudioGraphScheduler::addNode(const std::string& name, AudioNodeType type, int latencySamples)
{
    uint32_t id = m_nextId++;
    AudioGraphNode node;
    node.id = id;
    node.name = name;
    node.type = type;
    node.processingLatencySamples = latencySamples;
    node.executionLevel = 0;
    node.isMuted = false;

    m_nodes[id] = node;
    return id;
}

bool JZAudioGraphScheduler::removeNode(uint32_t nodeId)
{
    auto it = m_nodes.find(nodeId);
    if (it == m_nodes.end()) return false;

    // Remove from other nodes' dependencies and dependents
    for (auto& pair : m_nodes) {
        auto& deps = pair.second.dependencies;
        deps.erase(std::remove(deps.begin(), deps.end(), nodeId), deps.end());

        auto& depts = pair.second.dependents;
        depts.erase(std::remove(depts.begin(), depts.end(), nodeId), depts.end());
    }

    m_nodes.erase(it);
    return true;
}

bool JZAudioGraphScheduler::addEdge(uint32_t sourceNodeId, uint32_t destinationNodeId)
{
    if (sourceNodeId == destinationNodeId) return false;
    if (m_nodes.find(sourceNodeId) == m_nodes.end()) return false;
    if (m_nodes.find(destinationNodeId) == m_nodes.end()) return false;

    auto& srcNode = m_nodes[sourceNodeId];
    auto& destNode = m_nodes[destinationNodeId];

    if (std::find(srcNode.dependents.begin(), srcNode.dependents.end(), destinationNodeId) == srcNode.dependents.end()) {
        srcNode.dependents.push_back(destinationNodeId);
    }
    if (std::find(destNode.dependencies.begin(), destNode.dependencies.end(), sourceNodeId) == destNode.dependencies.end()) {
        destNode.dependencies.push_back(sourceNodeId);
    }

    return true;
}

bool JZAudioGraphScheduler::removeEdge(uint32_t sourceNodeId, uint32_t destinationNodeId)
{
    if (m_nodes.find(sourceNodeId) == m_nodes.end()) return false;
    if (m_nodes.find(destinationNodeId) == m_nodes.end()) return false;

    auto& srcNode = m_nodes[sourceNodeId];
    auto& destNode = m_nodes[destinationNodeId];

    srcNode.dependents.erase(
        std::remove(srcNode.dependents.begin(), srcNode.dependents.end(), destinationNodeId),
        srcNode.dependents.end());

    destNode.dependencies.erase(
        std::remove(destNode.dependencies.begin(), destNode.dependencies.end(), sourceNodeId),
        destNode.dependencies.end());

    return true;
}

void JZAudioGraphScheduler::setThreadCount(int threadCount)
{
    m_threadCount = threadCount > 0 ? threadCount : 1;
}

bool JZAudioGraphScheduler::compileGraph()
{
    m_executionOrder.clear();
    m_parallelLevels.clear();
    m_hasCycle = false;
    m_maxLevel = 0;

    if (m_nodes.empty()) return true;

    return detectCyclesAndAssignLevels();
}

bool JZAudioGraphScheduler::detectCyclesAndAssignLevels()
{
    // Kahn's algorithm for Topological Sort & Level Assignment
    std::unordered_map<uint32_t, int> inDegree;
    for (const auto& pair : m_nodes) {
        inDegree[pair.first] = static_cast<int>(pair.second.dependencies.size());
    }

    std::queue<uint32_t> zeroQueue;
    for (const auto& pair : m_nodes) {
        if (inDegree[pair.first] == 0) {
            zeroQueue.push(pair.first);
            m_nodes[pair.first].executionLevel = 0;
        }
    }

    std::vector<uint32_t> ordered;
    m_maxLevel = 0;

    while (!zeroQueue.empty()) {
        uint32_t u = zeroQueue.front();
        zeroQueue.pop();
        ordered.push_back(u);

        int currentLevel = m_nodes[u].executionLevel;
        if (currentLevel > m_maxLevel) {
            m_maxLevel = currentLevel;
        }

        for (uint32_t v : m_nodes[u].dependents) {
            inDegree[v]--;
            // Update child's level to be at least currentLevel + 1
            if (m_nodes[v].executionLevel <= currentLevel) {
                m_nodes[v].executionLevel = currentLevel + 1;
            }

            if (inDegree[v] == 0) {
                zeroQueue.push(v);
            }
        }
    }

    // If ordered doesn't contain all nodes, there is a cycle!
    if (ordered.size() != m_nodes.size()) {
        m_hasCycle = true;
        m_executionOrder.clear();
        m_parallelLevels.clear();
        return false;
    }

    m_hasCycle = false;
    m_executionOrder = ordered;

    // Partition into parallel execution levels
    m_parallelLevels.resize(m_maxLevel + 1);
    for (uint32_t nodeId : m_executionOrder) {
        int level = m_nodes[nodeId].executionLevel;
        m_parallelLevels[level].push_back(nodeId);
    }

    return true;
}

const AudioGraphNode* JZAudioGraphScheduler::getNode(uint32_t nodeId) const
{
    auto it = m_nodes.find(nodeId);
    if (it == m_nodes.end()) return nullptr;
    return &(it->second);
}

void JZAudioGraphScheduler::setNodeMute(uint32_t nodeId, bool mute)
{
    auto it = m_nodes.find(nodeId);
    if (it != m_nodes.end()) {
        it->second.isMuted = mute;
    }
}

int JZAudioGraphScheduler::calculateTotalGraphLatency() const
{
    int maxLatency = 0;
    for (const auto& pair : m_nodes) {
        if (pair.second.processingLatencySamples > maxLatency) {
            maxLatency = pair.second.processingLatencySamples;
        }
    }
    return maxLatency;
}

} // namespace Jazz
