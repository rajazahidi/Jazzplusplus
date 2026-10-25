//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// ComplexSidechainAdcGraphResolver.cpp - Complex Sidechain Directed Acyclic Graph
// (DAG) Automatic Delay Compensation (ADC) Solver
//*****************************************************************************

#include "ComplexSidechainAdcGraphResolver.h"
#include <algorithm>
#include <queue>

namespace Jazz {

JZComplexSidechainAdcGraphResolver::JZComplexSidechainAdcGraphResolver()
{
    clear();
}

void JZComplexSidechainAdcGraphResolver::clear()
{
    m_nodes.clear();
    m_masterNodeId = "Master";
    m_lastResult = ResolutionResult{};
}

void JZComplexSidechainAdcGraphResolver::addNode(const std::string& nodeId, const std::string& name, uint32_t pluginLatencySamples)
{
    AdcNode node;
    node.nodeId = nodeId;
    node.name = name.empty() ? nodeId : name;
    node.internalPluginLatencySamples = pluginLatencySamples;
    m_nodes[nodeId] = node;
}

void JZComplexSidechainAdcGraphResolver::addConnection(
    const std::string& sourceNodeId, const std::string& targetNodeId, AdcConnectionType type)
{
    if (m_nodes.find(sourceNodeId) != m_nodes.end() && m_nodes.find(targetNodeId) != m_nodes.end()) {
        AdcEdge edge;
        edge.targetNodeId = targetNodeId;
        edge.connectionType = type;
        m_nodes[sourceNodeId].outgoingEdges.push_back(edge);
    }
}

void JZComplexSidechainAdcGraphResolver::setMasterOutputNode(const std::string& masterNodeId)
{
    m_masterNodeId = masterNodeId;
    if (m_nodes.find(masterNodeId) == m_nodes.end()) {
        addNode(masterNodeId, "Master Output", 0);
    }
}

bool JZComplexSidechainAdcGraphResolver::detectCycleAndTopologicalSort(
    std::vector<std::string>& sortedOrder, std::string& cycleError)
{
    // In-degree calculation
    std::map<std::string, int> inDegree;
    for (const auto& pair : m_nodes) {
        inDegree[pair.first] = 0;
    }
    for (const auto& pair : m_nodes) {
        for (const auto& edge : pair.second.outgoingEdges) {
            inDegree[edge.targetNodeId]++;
        }
    }

    // Queue nodes with in-degree 0 (sources / roots)
    std::queue<std::string> q;
    for (const auto& pair : inDegree) {
        if (pair.second == 0) {
            q.push(pair.first);
        }
    }

    sortedOrder.clear();
    while (!q.empty()) {
        std::string u = q.front();
        q.pop();
        sortedOrder.push_back(u);

        for (const auto& edge : m_nodes[u].outgoingEdges) {
            inDegree[edge.targetNodeId]--;
            if (inDegree[edge.targetNodeId] == 0) {
                q.push(edge.targetNodeId);
            }
        }
    }

    if (sortedOrder.size() != m_nodes.size()) {
        cycleError = "Feedback routing cycle detected in audio graph";
        return false;
    }
    return true;
}

JZComplexSidechainAdcGraphResolver::ResolutionResult JZComplexSidechainAdcGraphResolver::resolveDelayCompensation()
{
    m_lastResult = ResolutionResult{};

    if (m_nodes.empty()) {
        m_lastResult.isResolved = true;
        return m_lastResult;
    }

    // 1. Cycle Detection & Topological Sort
    std::vector<std::string> topoOrder;
    std::string cycleErr;
    if (!detectCycleAndTopologicalSort(topoOrder, cycleErr)) {
        m_lastResult.isAcyclic = false;
        m_lastResult.isResolved = false;
        m_lastResult.cycleErrorMessage = cycleErr;
        return m_lastResult;
    }

    m_lastResult.isAcyclic = true;
    m_lastResult.topologicalOrder = topoOrder;

    // 2. Compute Longest Path to Master (Reverse Topological traversal)
    for (auto& pair : m_nodes) {
        pair.second.cumulativeLatencyToMaster = pair.second.internalPluginLatencySamples;
    }

    for (auto it = topoOrder.rbegin(); it != topoOrder.rend(); ++it) {
        const std::string& nodeId = *it;
        auto& node = m_nodes[nodeId];

        uint32_t maxChildPath = 0;
        for (const auto& edge : node.outgoingEdges) {
            const auto& childNode = m_nodes[edge.targetNodeId];
            if (childNode.cumulativeLatencyToMaster > maxChildPath) {
                maxChildPath = childNode.cumulativeLatencyToMaster;
            }
        }
        node.cumulativeLatencyToMaster = node.internalPluginLatencySamples + maxChildPath;
    }

    // 3. Determine Global Critical Path Latency
    uint32_t criticalPath = 0;
    for (const auto& pair : m_nodes) {
        if (pair.second.cumulativeLatencyToMaster > criticalPath) {
            criticalPath = pair.second.cumulativeLatencyToMaster;
        }
    }
    m_lastResult.masterCriticalPathLatencySamples = criticalPath;

    // 4. Calculate Required Delay Compensation per Node
    for (auto& pair : m_nodes) {
        uint32_t needed = (criticalPath >= pair.second.cumulativeLatencyToMaster)
                          ? (criticalPath - pair.second.cumulativeLatencyToMaster) : 0;
        pair.second.requiredDelayCompensationSamples = needed;
        m_lastResult.nodeDelayCompensations[pair.first] = needed;
    }

    m_lastResult.isResolved = true;
    return m_lastResult;
}

uint32_t JZComplexSidechainAdcGraphResolver::getCompensationForNode(const std::string& nodeId) const
{
    auto it = m_lastResult.nodeDelayCompensations.find(nodeId);
    if (it != m_lastResult.nodeDelayCompensations.end()) {
        return it->second;
    }
    return 0;
}

} // namespace Jazz
