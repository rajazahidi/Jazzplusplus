//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// ComplexSidechainAdcGraphResolver.h - Complex Sidechain Directed Acyclic Graph
// (DAG) Automatic Delay Compensation (ADC) Solver
//
// Resolves multi-hop nested subgroup routing, parallel audio busses, and cross-track
// sidechain dependencies. Uses topological graph sorting and critical-path latency
// analysis to calculate sample-accurate delay compensation buffers across all
// audio paths, eliminating phase smearing and comb filtering.
//*****************************************************************************

#ifndef JAZZ_COMPLEX_SIDECHAIN_ADC_GRAPH_RESOLVER_H
#define JAZZ_COMPLEX_SIDECHAIN_ADC_GRAPH_RESOLVER_H

#include <vector>
#include <string>
#include <map>
#include <set>
#include <cstdint>
#include <cstddef>

namespace Jazz {

enum class AdcConnectionType {
    AUDIO_BUS = 0,         // Main or auxiliary audio summing bus
    SIDECHAIN_CONTROL      // Dynamic sidechain control detector signal
};

struct AdcEdge {
    std::string targetNodeId;
    AdcConnectionType connectionType = AdcConnectionType::AUDIO_BUS;
};

struct AdcNode {
    std::string nodeId;
    std::string name;
    uint32_t internalPluginLatencySamples = 0; // Lookahead latency of insert FX
    std::vector<AdcEdge> outgoingEdges;

    // Computed compensation metrics
    uint32_t cumulativeLatencyToMaster = 0;
    uint32_t requiredDelayCompensationSamples = 0;
};

class JZComplexSidechainAdcGraphResolver {
public:
    struct ResolutionResult {
        bool isAcyclic = true;                      // True if graph is free of illegal feedback loops
        bool isResolved = false;
        std::string cycleErrorMessage;
        uint32_t masterCriticalPathLatencySamples = 0;
        std::vector<std::string> topologicalOrder;
        std::map<std::string, uint32_t> nodeDelayCompensations;
    };

    JZComplexSidechainAdcGraphResolver();
    ~JZComplexSidechainAdcGraphResolver() = default;

    void clear();

    // Graph Construction
    void addNode(const std::string& nodeId, const std::string& name, uint32_t pluginLatencySamples = 0);
    void addConnection(const std::string& sourceNodeId, const std::string& targetNodeId, AdcConnectionType type = AdcConnectionType::AUDIO_BUS);
    void setMasterOutputNode(const std::string& masterNodeId);

    // Resolves ADC graph
    ResolutionResult resolveDelayCompensation();

    // Inspection
    uint32_t getCompensationForNode(const std::string& nodeId) const;
    uint32_t getCriticalPathLatency() const { return m_lastResult.masterCriticalPathLatencySamples; }
    bool isGraphValid() const { return m_lastResult.isAcyclic && m_lastResult.isResolved; }
    size_t getNodeCount() const { return m_nodes.size(); }

private:
    std::map<std::string, AdcNode> m_nodes;
    std::string m_masterNodeId = "Master";
    ResolutionResult m_lastResult;

    bool detectCycleAndTopologicalSort(std::vector<std::string>& sortedOrder, std::string& cycleError);
};

} // namespace Jazz

#endif // JAZZ_COMPLEX_SIDECHAIN_ADC_GRAPH_RESOLVER_H
