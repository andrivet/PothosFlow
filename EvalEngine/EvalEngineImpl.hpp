// Copyright (c) 2014-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#pragma once
#include <Pothos/Config.hpp>
#include "EnvironmentEval.hpp"
#include "TopologyEval.hpp"
#include "BlockEval.hpp"
#include <QString>
#include <Poco/JSON/Object.h>
#include <memory>
#include <map>
#include <set>

class EnvironmentEval;
class ThreadPoolEval;
class TopologyEval;
class BlockEval;
class GraphBlock;
class QTimer;
class EvalEngineGuiBlockDeleter;

typedef std::map<size_t, BlockInfo> BlockInfos;
typedef std::map<QString, Poco::JSON::Object::Ptr> ZoneInfos;

/*!
 * The EvalEngineImpl hold eval state and performs the actual work
 */
class EvalEngineImpl : public QObject
{
    Q_OBJECT
public:

    EvalEngineImpl(void);

    ~EvalEngineImpl(void);

signals:

    //! Emitted by the monitor timer to signal that this thread is not-blocked
    void monitorHeartBeat(void);

    //! A failure occured, this is a notification to deactivate
    void deactivateDesign(void);

public slots:

    //! Submit trigger for de/activation of the topology
    void submitActivateTopology(const bool enable);

    //! Submit a single block info for individual re-eval
    void submitBlock(const BlockInfo &info);

    //! Submit most up to date topology information
    void submitTopology(const BlockInfos &blockInfos, const ConnectionInfos &connections);

    //! Submit a list if UIDs to re-evaluate
    void submitReeval(const std::vector<size_t> &uids);

    //! Submit most up to date zone information
    void submitZoneInfo(const ZoneInfos &info);

    //! query the dot markup for the active topology
    std::string getTopologyDotMarkup(const std::string &config);

    //! Cleanup and shutdown prior to destruction
    void submitCleanup(void);

private slots:
    void handleMonitorTimeout(void);

private:
    void evaluate(void);
    bool _requireEval;

    QTimer *_monitorTimer;

    //most recent info
    BlockInfos _blockInfo;
    ConnectionInfos _connectionInfo;
    ZoneInfos _zoneInfo;

    //current state of the evaluator
    std::map<HostProcPair, std::shared_ptr<EnvironmentEval>> _environmentEvals;
    std::map<QString, std::shared_ptr<ThreadPoolEval>> _threadPoolEvals;
    std::map<size_t, std::shared_ptr<BlockEval>> _blockEvals;
    std::shared_ptr<TopologyEval> _topologyEval;

    void handleOrphanedGuiBlocks(void);
    std::set<std::shared_ptr<void>> _guiBlocks;
    std::shared_ptr<EvalEngineGuiBlockDeleter> _guiBlockDeleter;
};