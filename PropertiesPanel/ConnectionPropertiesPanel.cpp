// Copyright (c) 2014-2018 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "MainWindow/IconUtils.hpp"
#include "ConnectionPropertiesPanel.hpp"
#include "GraphObjects/GraphConnection.hpp"
#include "GraphObjects/GraphBlock.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QPushButton>
#include <QLabel>
#include <algorithm> //std::set_difference

ConnectionPropertiesPanel::ConnectionPropertiesPanel(GraphConnection *conn, QWidget *parent):
    QWidget(parent),
    _conn(conn),
    _isSlot(_conn->getInputEndpoint().getConnectableAttrs().direction == GRAPH_CONN_SLOT),
    _isSignal(_conn->getOutputEndpoint().getConnectableAttrs().direction == GRAPH_CONN_SIGNAL),
    _removeButton(nullptr),
    _inputListWidget(nullptr),
    _outputListWidget(nullptr),
    _connectionsListWidget(nullptr)
{
    //master layout for this widget
    auto layout = new QVBoxLayout(this);

    auto inputEp = _conn->getInputEndpoint();
    auto outputEp = _conn->getOutputEndpoint();

    //title
    {
        auto label = new QLabel(tr("<h1>%1</h1>").arg(tr("Connection")), this);
        label->setAlignment(Qt::AlignCenter);
        layout->addWidget(label);
    }

    //signal/slots selection boxes
    {
        auto listWidgetLayout = new QHBoxLayout();
        layout->addLayout(listWidgetLayout);

        //output list
        _outputListWidget = this->makePortListWidget(this, outputEp, _outputItemToKey);
        connect(_outputListWidget, &QTreeWidget::itemSelectionChanged, this, &ConnectionPropertiesPanel::handleItemSelectionChanged);
        listWidgetLayout->addWidget(_outputListWidget);

        //input list
        _inputListWidget = this->makePortListWidget(this, inputEp, _inputItemToKey);
        connect(_inputListWidget, &QTreeWidget::itemSelectionChanged, this, &ConnectionPropertiesPanel::handleItemSelectionChanged);
        listWidgetLayout->addWidget(_inputListWidget);
    }

    //existing connections
    {
        _connectionsListWidget = new QTreeWidget(this);
        layout->addWidget(_connectionsListWidget);
        _connectionsListWidget->setColumnCount(1);
        _connectionsListWidget->setHeaderLabels(QStringList(tr("Signal slot connections")));
        connect(_connectionsListWidget, &QTreeWidget::itemSelectionChanged, this, &ConnectionPropertiesPanel::handleItemSelectionChanged);

        //save pre-edit settings
        _originalKeyPairs = _conn->getSigSlotPairs();
    }

    //remove button
    {
        _removeButton = new QPushButton(makeIconFromTheme("list-remove"), tr("Remove connection"), this);
        connect(_removeButton, &QPushButton::pressed, this, &ConnectionPropertiesPanel::handleRemoveConnection);
        layout->addWidget(_removeButton);
    }

    connect(_conn, &GraphConnection::destroyed, this, &ConnectionPropertiesPanel::handleConnectionDestroyed);
    this->handleItemSelectionChanged(); //init state
    this->populateConnectionsList(); //init state
}

void ConnectionPropertiesPanel::handleConnectionDestroyed(QObject *)
{
    this->deleteLater();
}

void ConnectionPropertiesPanel::handleCancel(void)
{
    //restore original settings
    _conn->setSigSlotPairs(_originalKeyPairs);
}

template <typename T>
T mySetDifference(const T &a, const T &b)
{
    T aSort = a; std::sort(aSort.begin(), aSort.end());
    T bSort = b; std::sort(bSort.begin(), bSort.end());
    T v(std::max(a.size(), b.size()));
    auto it = std::set_difference(aSort.begin(), aSort.end(), bSort.begin(), bSort.end(), v.begin());
    v.resize(it-v.begin());
    return v;
}

void ConnectionPropertiesPanel::handleCommit(void)
{
    const auto createdPairs = mySetDifference(_conn->getSigSlotPairs(), _originalKeyPairs);
    const auto removedPairs = mySetDifference(_originalKeyPairs, _conn->getSigSlotPairs());

    //description of the change
    QString desc;
    if (createdPairs.empty() and removedPairs.empty()) return this->handleCancel();
    else if (createdPairs.empty() and removedPairs.size() == 1)
    {
        desc = tr("Removed %1->%2")
            .arg(_conn->getKeyName(removedPairs.at(0).first, _conn->getOutputEndpoint()))
            .arg(_conn->getKeyName(removedPairs.at(0).second, _conn->getInputEndpoint()));
    }
    else if (createdPairs.size() == 1 and removedPairs.empty())
    {
        desc = tr("Created %1->%2")
            .arg(_conn->getKeyName(createdPairs.at(0).first, _conn->getOutputEndpoint()))
            .arg(_conn->getKeyName(createdPairs.at(0).second, _conn->getInputEndpoint()));
    }
    else
    {
        desc = tr("Changed signals->slots");
    }

    //emit a new graph state event
    emit this->stateChanged(GraphState("document-properties", desc));
}

void ConnectionPropertiesPanel::handleItemSelectionChanged(void)
{
    auto signalItemsSelected = _outputListWidget->selectedItems();
    auto slotItemsSelected = _inputListWidget->selectedItems();
    _removeButton->setEnabled(not _connectionsListWidget->selectedItems().isEmpty());
    if (not signalItemsSelected.isEmpty() and not slotItemsSelected.isEmpty())
    {
        this->handleCreateConnection();
    }
}

void ConnectionPropertiesPanel::handleCreateConnection(void)
{
    for (auto signalItem : _outputListWidget->selectedItems())
    {
        signalItem->setSelected(false);
        for (auto slotItem : _inputListWidget->selectedItems())
        {
            slotItem->setSelected(false);
            auto pair = std::make_pair(
                _outputItemToKey.at(signalItem),
                _inputItemToKey.at(slotItem));
            _conn->addSigSlotPair(pair);
            this->populateConnectionsList();
        }
    }
}

void ConnectionPropertiesPanel::populateConnectionsList(void)
{
    _connectionsListWidget->clear();
    _connItemToKeyPair.clear();
    for (const auto &pair : _conn->getSigSlotPairs())
    {
        auto item = new QTreeWidgetItem(_connectionsListWidget, QStringList(QString("%1 -> %2")
            .arg(_conn->getKeyName(pair.first, _conn->getOutputEndpoint()))
            .arg(_conn->getKeyName(pair.second, _conn->getInputEndpoint()))));
        _connItemToKeyPair[item] = pair;
        _connectionsListWidget->addTopLevelItem(item);
    }
}

void ConnectionPropertiesPanel::handleRemoveConnection(void)
{
    for (auto item : _connectionsListWidget->selectedItems())
    {
        _conn->removeSigSlotPair(_connItemToKeyPair.at(item));
        delete item;
        this->populateConnectionsList();
    }
}

QTreeWidget *ConnectionPropertiesPanel::makePortListWidget(QWidget *parent, const GraphConnectionEndpoint &ep, std::map<QTreeWidgetItem *, QString> &itemToKey)
{
    //create list widget
    auto listWidget = new QTreeWidget(parent);
    listWidget->setColumnCount(1);
    listWidget->setHeaderLabels(QStringList(QString("%1[%2]").arg(ep.getObj()->getId(), ep.getKey().id)));

    //query the signal/slot ports
    QStringList portKeys;
    auto block = qobject_cast<GraphBlock *>(ep.getObj().data());
    if (block != nullptr) switch(ep.getConnectableAttrs().direction)
    {
    case GRAPH_CONN_SLOT: portKeys = block->getSlotPorts(); break;
    case GRAPH_CONN_SIGNAL: portKeys = block->getSignalPorts(); break;
    default: break;
    }

    //populate
    if (portKeys.empty())
    {
        auto item = new QTreeWidgetItem(listWidget, QStringList(_conn->getKeyName(ep.getKey().id, ep)));
        itemToKey[item] = ep.getKey().id;
        listWidget->addTopLevelItem(item);
    }
    else for (const auto &portKey : portKeys)
    {
        auto item = new QTreeWidgetItem(listWidget, QStringList(_conn->getKeyName(portKey, ep)));
        itemToKey[item] = portKey;
        listWidget->addTopLevelItem(item);
    }
    listWidget->resizeColumnToContents(0);
    listWidget->setSortingEnabled(true);
    listWidget->sortByColumn(0, Qt::AscendingOrder);

    //return new widget
    return listWidget;
}
