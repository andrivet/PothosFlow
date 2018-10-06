// Copyright (c) 2013-2018 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#pragma once
#include <Pothos/Config.hpp>
#include "GraphObjects/GraphObject.hpp"
#include "GraphObjects/GraphEndpoint.hpp"
#include <QObject>
#include <QString>
#include <QPointF>
#include <memory>
#include <utility> //pair

typedef std::pair<QString, QString> SigSlotPair;

class GraphConnection : public GraphObject
{
    Q_OBJECT
public:
    GraphConnection(QObject *parent);
    ~GraphConnection(void);

    void setupEndpoint(const GraphConnectionEndpoint &ep);

    const GraphConnectionEndpoint &getOutputEndpoint(void) const;
    const GraphConnectionEndpoint &getInputEndpoint(void) const;

    //! true if an endpoint is a signal or slot
    bool isSignalOrSlot(void) const;

    //! Get all the connected endpoints inside the signal/slots connection
    const std::vector<SigSlotPair> &getSigSlotPairs(void) const;

    //! Set the connected endpoints to a new list of signal/slots connections
    void setSigSlotPairs(const std::vector<SigSlotPair> &);

    //! Add a new signal slot connection
    void addSigSlotPair(const SigSlotPair &sigSlot);

    //! Remove an existing signal slot connection
    void removeSigSlotPair(const SigSlotPair &sigSlot);

    /*!
     * Get a list of endpoint pairs.
     * For a regular connection, this is just the output, input endpoint.
     * For a signal/slot style connection, this is a list representing
     * all connections involving signals and slots inside the connection.
     */
    std::vector<std::pair<GraphConnectionEndpoint, GraphConnectionEndpoint>> getEndpointPairs(void) const;

    //! utility to get a displayable name for a port key knowing its endpoint
    QString getKeyName(const QString &portKey, const GraphConnectionEndpoint &ep);

    QPainterPath shape(void) const;

    void render(QPainter &painter);

    QJsonObject serialize(void) const;

    virtual void deserialize(const QJsonObject &obj);

private slots:

    //! The block or breaker was deleted, delete this connection
    void handleEndPointDestroyed(QObject *obj);

    //! After some possible changing event, recheck the endpoints
    void handleEndPointEventRecheck(void);

private:

    //only called by the destructor
    void unregisterEndpoint(const GraphConnectionEndpoint &ep);

    struct Impl;
    std::unique_ptr<Impl> _impl;
};
