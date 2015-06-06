///////////////////////////////////////////////////////////////////////////////
//
// CoinSocket
//
// events.cpp
//
// Copyright (c) 2015 Eric Lombrozo
//
// All Rights Reserved.
//

#include "events.h"

#include <CoinDB/SynchedVault.h>
#include <WebSocketAPI/Server.h>

#include <logger/logger.h>

#include "config.h"
#include "jsonobjects.h"
#include "txproposal.h"

#include <string>
#include <set>
#include <vector>
#include <map>

#include <thread>
#include <chrono>


using namespace CoinSocket;
using namespace WebSocket;
using namespace CoinDB;
using namespace std;

static multimap<uint32_t, shared_ptr<Tx>> g_pendingTxs;
static set<bytes_t> g_pendingTxHashes;

multimap<uint32_t, shared_ptr<Tx>>& CoinSocket::getPendingTxs() { return g_pendingTxs; }
set<bytes_t>& CoinSocket::getPendingTxHashes() { return g_pendingTxHashes; } 

// TODO: Clean up the txapprovedjson and txrejectedjson mess

void CoinSocket::sendTxJsonEvent(TxEventType type, WebSocket::Server& wsServer, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, std::shared_ptr<CoinDB::Tx>& tx, bool fakeFinal)
{
    using namespace json_spirit;

    try
    {
        bytes_t unsigned_hash = tx->unsigned_hash();
        Tx::status_t status = tx->status();
        string hash = uchar_vector(tx->hash()).getHex();
        string statusstr = Tx::getStatusString(status);
        //uint32_t confirmations = synchedVault.getVault()->getTxConfirmations(tx);
        uint32_t height = tx->blockheader() ? tx->blockheader()->height() : 0;
        bool bFinal = fakeFinal || ((height > 0) && (synchedVault.getSyncHeight() >= height + getConfig().getMinConf() - 1));

        switch (type)
        {
        case INSERTED:
            LOGGER(debug) << "Transaction inserted: " << hash << " Status: " << statusstr << " Height: " << height << endl;
            break;
        case UPDATED:
            LOGGER(debug) << "Transaction updated: " << hash << " Status: " << statusstr << " Height: " << height << endl;
            break;
        case APPROVED:
            LOGGER(debug) << "Transaction approved: " << hash << " Status: " << statusstr << " Height: " << height << endl;
            break;
        case CANCELED:
            LOGGER(debug) << "Transaction canceled: " << hash << " Status: " << statusstr << " Height: " << height << endl;
            break;
        case REJECTED:
            LOGGER(debug) << "Transaction rejected: " << hash << " Status: " << statusstr << " Height: " << height << endl;
            break;
        case DELETED:
            LOGGER(debug) << "Transaction deleted: " << hash << " Status: " << statusstr << " Height: " << height << endl;
            break;
        default:
            break;
        }

        {
            Value txVal;
            if (!read_string(tx->toJson(false), txVal) || txVal.type() != obj_type) throw InternalTxJsonInvalidException();
            Object txObj = txVal.get_obj();
            txObj.push_back(Pair("assettype", getConfig().getCoinParams().currency_symbol()));
            txObj.push_back(Pair("final", bFinal));

            //txObj.push_back(Pair("confirmations", (uint64_t)confirmations));
            stringstream msg;
            switch (type)
            {
            case INSERTED:
                msg << "{\"event\":\"txinsertedjson\", \"data\":" << write_string<Value>(txObj) << "}";
                wsServer.send(hdl, msg.str());
                break;
            case UPDATED:
                msg << "{\"event\":\"txupdatedjson\", \"data\":" << write_string<Value>(txObj) << "}";
                wsServer.send(hdl, msg.str());
                break;
            case APPROVED:
                msg << "{\"event\":\"txapprovedjson\", \"data\":" << write_string<Value>(txObj) << "}";
                wsServer.send(hdl, msg.str());
                break;
            case CANCELED:
                msg << "{\"event\":\"txcanceledjson\", \"data\":" << write_string<Value>(txObj) << "}";
                wsServer.send(hdl, msg.str());
                break;
            case REJECTED:
                msg << "{\"event\":\"txrejectedjson\", \"data\":" << write_string<Value>(txObj) << "}";
                wsServer.send(hdl, msg.str());
                break;
            case DELETED:
                msg << "{\"event\":\"txdeletedjson\", \"data\":" << write_string<Value>(txObj) << "}";
                wsServer.send(hdl, msg.str());
                break;
            default:
                break;
            }

            if (type == INSERTED || type == UPDATED || type == DELETED)
            {
                shared_ptr<TxProposal> txProposal = getProcessedTxProposal(tx->unsigned_hash());
                if (txProposal)
                {
                    txObj.push_back(Pair("proposal", getTxProposalObject(*txProposal)));
                    stringstream msg;
                    switch (type)
                    {
                    case INSERTED:
                    case UPDATED:
                        if (status == Tx::PROPAGATED || status == Tx::CONFIRMED)
                        {
                            msg << "{\"event\":\"txapprovedjson\", \"data\":" << write_string<Value>(txObj) << "}";
                            wsServer.send(hdl, msg.str());
                        }
                        break;
                    case DELETED:
                        msg << "{\"event\":\"txrejectedjson\", \"data\":" << write_string<Value>(txObj) << "}";
                        wsServer.send(hdl, msg.str());
                        break;
                    default:
                        break;
                    }
                }
            }
        }
    }
    catch (const exception& e)
    {
        LOGGER(error) << "sendTxJsonEvent() error: " << e.what() << endl;
    }
}

void CoinSocket::sendTxChannelEvent(TxEventType type, Server& wsServer, SynchedVault& synchedVault, shared_ptr<Tx>& tx, bool fakeFinal)
{
    using namespace json_spirit;

    try
    {
        bytes_t unsigned_hash = tx->unsigned_hash();
        Tx::status_t status = tx->status();
        string hash = uchar_vector(tx->hash()).getHex();
        string statusstr = Tx::getStatusString(status);
        //uint32_t confirmations = synchedVault.getVault()->getTxConfirmations(tx);
        uint32_t height = tx->blockheader() ? tx->blockheader()->height() : 0;
        bool bFinal = fakeFinal || ((height > 0) && (synchedVault.getSyncHeight() >= height + getConfig().getMinConf() - 1));
        if (status == Tx::CONFIRMED && !bFinal && g_pendingTxHashes.find(unsigned_hash) == g_pendingTxHashes.end())
        {
            g_pendingTxHashes.insert(unsigned_hash);
            g_pendingTxs.insert(pair<uint32_t, shared_ptr<Tx>>(height, tx));
        }

        shared_ptr<TxProposal> txProposal = getProcessedTxProposal(tx->unsigned_hash());

        switch (type)
        {
        case INSERTED:
            LOGGER(debug) << "Transaction inserted: " << hash << " Status: " << statusstr << " Height: " << height << endl;
            break;
        case UPDATED:
            LOGGER(debug) << "Transaction updated: " << hash << " Status: " << statusstr << " Height: " << height << endl;
            break;
        case DELETED:
            LOGGER(debug) << "Transaction deleted: " << hash << " Status: " << statusstr << " Height: " << height << endl;
            break;
        default:
            break;
        }

        Object txData;
        txData.push_back(Pair("hash", hash));
        txData.push_back(Pair("status", statusstr));
        //txData.push_back(Pair("confirmations", (uint64_t)confirmations));
        txData.push_back(Pair("height", (uint64_t)height));

        {
            stringstream msg;
            switch (type)
            {
            case INSERTED:
                msg << "{\"event\":\"txinserted\", \"data\":" << write_string<Value>(txData) << "}";
                wsServer.sendChannel("txinserted", msg.str());
                break;
            case UPDATED:
                msg << "{\"event\":\"txupdated\", \"data\":" << write_string<Value>(txData) << "}";
                wsServer.sendChannel("txupdated", msg.str());
                break;
            case DELETED:
                msg << "{\"event\":\"txdeleted\", \"data\":" << write_string<Value>(txData) << "}";
                wsServer.sendChannel("txdeleted", msg.str());
                break;
            default:
                break;
            }
        }
        {
            Value txVal;
            if (!read_string(tx->toJson(false), txVal) || txVal.type() != obj_type) throw InternalTxJsonInvalidException();
            Object txObj = txVal.get_obj();
            txObj.push_back(Pair("assettype", getConfig().getCoinParams().currency_symbol()));
            txObj.push_back(Pair("final", bFinal));

            //txObj.push_back(Pair("confirmations", (uint64_t)confirmations));
            stringstream msg;
            switch (type)
            {
            case INSERTED:
                msg << "{\"event\":\"txinsertedjson\", \"data\":" << write_string<Value>(txObj) << "}";
                wsServer.sendChannel("txinsertedjson", msg.str());
                if (txProposal && (status == Tx::PROPAGATED || status == Tx::CONFIRMED))
                {
                    txObj.push_back(Pair("proposal", getTxProposalObject(*txProposal)));
                    stringstream msg;
                    msg << "{\"event\":\"txapprovedjson\", \"data\":" << write_string<Value>(txObj) << "}";
                    wsServer.sendChannel("txapprovedjson", msg.str());
                }
                break;
            case UPDATED:
                msg << "{\"event\":\"txupdatedjson\", \"data\":" << write_string<Value>(txObj) << "}";
                wsServer.sendChannel("txupdatedjson", msg.str());
                if (txProposal && (status == Tx::PROPAGATED || status == Tx::CONFIRMED))
                {
                    txObj.push_back(Pair("proposal", getTxProposalObject(*txProposal)));
                    stringstream msg;
                    msg << "{\"event\":\"txapprovedjson\", \"data\":" << write_string<Value>(txObj) << "}";
                    wsServer.sendChannel("txapprovedjson", msg.str());
                }
                break;
            case DELETED:
                msg << "{\"event\":\"txdeletedjson\", \"data\":" << write_string<Value>(txObj) << "}";
                wsServer.sendChannel("txdeletedjson", msg.str());
                if (txProposal)
                {
                    txObj.push_back(Pair("proposal", getTxProposalObject(*txProposal)));
                    stringstream msg;
                    msg << "{\"event\":\"txrejectedjson\", \"data\":" << write_string<Value>(txObj) << "}";
                    wsServer.sendChannel("txrejectedjson", msg.str());
                }
                break;
            default:
                break;
            }
        }
        {
            Object rawTxData(txData);
            rawTxData.push_back(Pair("rawtx", uchar_vector(tx->raw()).getHex()));
            stringstream msg;
            switch (type)
            {
            case INSERTED:
                msg << "{\"event\":\"txinsertedraw\", \"data\":" << write_string<Value>(rawTxData) << "}";
                wsServer.sendChannel("txinsertedraw", msg.str());
                break;
            case UPDATED:
                msg << "{\"event\":\"txupdatedraw\", \"data\":" << write_string<Value>(rawTxData) << "}";
                wsServer.sendChannel("txupdatedraw", msg.str());
                break;
            case DELETED:
                msg << "{\"event\":\"txdeletedraw\", \"data\":" << write_string<Value>(rawTxData) << "}";
                wsServer.sendChannel("txdeletedraw", msg.str());
                break;
            default:
                break;
            }
        }
        {
            Object serializedTxData(txData);
            serializedTxData.push_back(Pair("serializedtx", tx->toSerialized()));
            stringstream msg;
            switch (type)
            {
            case INSERTED:
                msg << "{\"event\":\"txinsertedserialized\", \"data\":" << write_string<Value>(serializedTxData) << "}";
                wsServer.sendChannel("txinsertedserialized", msg.str());
                break;
            case UPDATED:
                msg << "{\"event\":\"txupdatedserialized\", \"data\":" << write_string<Value>(serializedTxData) << "}";
                wsServer.sendChannel("txupdatedserialized", msg.str());
                break;
            case DELETED:
                msg << "{\"event\":\"txdeletedserialized\", \"data\":" << write_string<Value>(serializedTxData) << "}";
                wsServer.sendChannel("txdeletedserialized", msg.str());
                break;
            default:
                break;
            }
        }
    }
    catch (const exception& e)
    {
        LOGGER(error) << "sendTxChannelEvent() error: " << e.what() << endl;
    }
}

void CoinSocket::sendStatusEvent(Server& wsServer, SynchedVault& synchedVault)
{
    uint32_t finalHeight = synchedVault.getSyncHeight() + 1 - getConfig().getMinConf();
    auto ret = getPendingTxs().equal_range(finalHeight);
    for (auto it = ret.first; it != ret.second; ++it)
    {
        sendTxChannelEvent(UPDATED, wsServer, synchedVault, it->second);
        getPendingTxHashes().erase(it->second->unsigned_hash());
    }
    getPendingTxs().erase(finalHeight);

    string syncStatusJson = json_spirit::write_string<json_spirit::Value>(getSyncStatusObject(synchedVault));
    LOGGER(debug) << "Status: " << syncStatusJson << endl;
    stringstream msg;
    msg << "{\"event\":\"status\", \"data\":" << syncStatusJson << "}";
    wsServer.sendChannel("status", msg.str());
}
