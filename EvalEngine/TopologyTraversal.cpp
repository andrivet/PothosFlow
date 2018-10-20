// Copyright (c) 2014-2018 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "TopologyEval.hpp"
#include "GraphObjects/GraphBlock.hpp"
#include "GraphObjects/GraphBreaker.hpp"
#include "GraphObjects/GraphConnection.hpp"

/*!
 * Given an input endpoint, discover all of the "resolved" input endpoints by traversing breakers of the same node name.
 */
static std::vector<GraphConnectionEndpoint> traverseInputEps(
    const GraphConnectionEndpoint &inputEp,
    const GraphObjectList &graphObjects,
    std::vector<GraphConnectionEndpoint> traversed = std::vector<GraphConnectionEndpoint>()
)
{
    std::vector<GraphConnectionEndpoint> inputEndpoints;
    if (not inputEp.getObj()->isEnabled()) return inputEndpoints;

    //avoid recursive loops by keeping track of traversed endpoints
    auto it = std::find(traversed.begin(), traversed.end(), inputEp);
    if (it != traversed.end()) return inputEndpoints;
    traversed.push_back(inputEp);

    auto inputBlock = qobject_cast<GraphBlock *>(inputEp.getObj().data());
    auto inputBreaker = qobject_cast<GraphBreaker *>(inputEp.getObj().data());

    if (inputBlock != nullptr)
    {
        inputEndpoints.push_back(inputEp);
    }

    if (inputBreaker != nullptr)
    {
        auto nodeName = inputBreaker->getNodeName();
        for (auto graphObject : graphObjects)
        {
            auto breaker = qobject_cast<GraphBreaker *>(graphObject);
            if (breaker == nullptr) continue;
            if (not breaker->isEnabled()) continue;
            if (breaker->getNodeName() != nodeName) continue;
            if (breaker == inputBreaker) continue;
            //follow all connections from this breaker to an input
            //this is the recursive part
            for (auto graphSubObject : graphObjects)
            {
                auto connection = qobject_cast<GraphConnection *>(graphSubObject);
                if (connection == nullptr) continue;
                if (not connection->isEnabled()) continue;
                if (connection->getOutputEndpoint().getObj() != breaker) continue;
                for (const auto &epPair : connection->getEndpointPairs())
                {
                    const auto &inputEp = epPair.second;
                    for (const auto &subEp : traverseInputEps(inputEp, graphObjects, traversed))
                    {
                        inputEndpoints.push_back(subEp);
                    }
                }
            }
        }
    }

    return inputEndpoints;
}

ConnectionInfos TopologyEval::getConnectionInfo(const GraphObjectList &graphObjects)
{
    ConnectionInfos connections;
    for (auto graphObject : graphObjects)
    {
        auto connection = qobject_cast<GraphConnection *>(graphObject);
        if (connection == nullptr) continue;
        if (not connection->isEnabled()) continue;
        if (not connection->getInputEndpoint().isValid()) continue;
        if (not connection->getOutputEndpoint().isValid()) continue;
        for (const auto &epPair : connection->getEndpointPairs())
        {
            const auto &outputEp = epPair.first;
            const auto &inputEp = epPair.second;

            //ignore connections from output breakers
            //we will come back to them from the block to breaker to block path
            auto outputBreaker = qobject_cast<GraphBreaker *>(outputEp.getObj().data());
            if (outputBreaker != nullptr) continue;

            for (const auto &subEp : traverseInputEps(inputEp, graphObjects))
            {
                ConnectionInfo info;
                info.srcBlockUID = outputEp.getObj()->uid();
                info.srcPort = outputEp.getKey().id;
                info.dstBlockUID = subEp.getObj()->uid();
                info.dstPort = subEp.getKey().id;
                connections.push_back(info);
            }
        }
    }
    return connections;
}
