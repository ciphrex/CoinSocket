///////////////////////////////////////////////////////////////////////////////
//
// CoinSocket
//
// CoinSocket.cpp
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
//

#include "CoinSocketExceptions.h"
#include "jsonobjects.h"
#include "channels.h"

#include <CoinDB/SynchedVault.h>
#include <CoinQ/CoinQ_script.h>
#include <CoinCore/Base58Check.h>
#include <CoinCore/random.h>

#include <WebSocketAPI/Server.h>

#include <logger/logger.h>

#include "config.h"
#include "commands.h"

#include <iostream>
#include <signal.h>

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

// Globals
command_map_t g_command_map;

bool g_bShutdown = false;

std::string g_connectKey;

// Callbacks
void finish(int sig)
{
    LOGGER(info) << "Stopping..." << endl;
    g_bShutdown = true;
}

bool validateCallback(Server& server, websocketpp::connection_hdl hdl)
{
    string endpoint = server.getRemoteEndpoint(hdl);
    string key = server.getResource(hdl);
    LOGGER(info) << "Validating client " << endpoint << "..." << endl;

    if (key == g_connectKey)
    {
        LOGGER(info) << "Client " << endpoint << " validation successful." << endl;
        return true;
    }
    else
    {
        LOGGER(info) << "Client " << endpoint << " validation failed." << endl;
        return false;
    }
}

void openCallback(Server& server, websocketpp::connection_hdl hdl)
{
    LOGGER(info) << "Client " << server.getRemoteEndpoint(hdl) << " connected as " << hdl.lock().get() << "." << endl;

    JsonRpc::Response res;
    res.setResult("connected");
    server.send(hdl, res);
}

void closeCallback(Server& server, websocketpp::connection_hdl hdl)
{
    LOGGER(info) << "Client " << server.getRemoteEndpoint(hdl) << " disconnected as " << hdl.lock().get() << "." << endl;
}

#ifdef USE_TLS
Server::context_ptr tlsInit(const std::string& tlsCertificateFile, Server& server, websocketpp::connection_hdl hdl)
{
    LOGGER(info) << "tlsInit called with hdl " << hdl.lock().get() << "." << endl;
 
    Server::context_ptr ctx(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv1));
 
    try
    {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::single_dh_use);
        ctx->set_password_callback(bind([]() { return "test"; }));
        ctx->use_certificate_chain_file(tlsCertificateFile);
        ctx->use_private_key_file(tlsCertificateFile, boost::asio::ssl::context::pem);
    }
    catch (std::exception& e)
    {
        LOGGER(error) << e.what() << std::endl;
    }
 
    return ctx;
}
#endif


void requestCallback(Server& server, SynchedVault& synchedVault, const Server::client_request_t& req)
{
    using namespace json_spirit;

    std::string remoteEndpoint = server.getRemoteEndpoint(req.first);
    LOGGER(info) << "Client " << remoteEndpoint << " sent command " << req.second.getJson() << "." << std::endl;

    const string& method = req.second.getMethod();
    const Array& params = req.second.getParams();
    const Value& id = req.second.getId();

    JsonRpc::Response response;

    try
    {
        auto it = g_command_map.find(method);
        if (it == g_command_map.end())
            throw CommandInvalidMethodException();

        Value result = it->second(server, req.first, synchedVault, params);
        response.setResult(result, id);
    }
    catch (const stdutils::custom_error& e)
    {
        response.setError(e, id);
    }
    catch (const exception& e)
    {
        response.setError(e, id);
    }

    server.send(req.first, response);
}

// Main program
int main(int argc, char* argv[])
{
    CoinSocketConfig config;

    try
    {
        config.init(argc, argv);
    }
    catch (const std::exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    if (config.help())
    {
        cout << config.getHelpOptions() << endl;
        return 0;
    }

    setDocumentDir(config.getDocumentDir());
    g_connectKey = string("/") + config.getConnectKey();
    std::string logFile = config.getDataDir() + "/coinsocket.log"; 
    INIT_LOGGER(logFile.c_str());

    signal(SIGINT, &finish);
    signal(SIGTERM, &finish);

    try
    {
        SynchedVault synchedVault;

        cout << "Opening vault " << config.getDatabaseName() << endl;
        LOGGER(info) << "Opening vault " << config.getDatabaseName() << endl;
        synchedVault.openVault(config.getDatabaseUser(), config.getDatabasePassword(), config.getDatabaseName());

        initCommandMap(g_command_map);
        Server wsServer(config.getWebSocketPort(), config.getAllowedIps());
        wsServer.setValidateCallback(&validateCallback);
        wsServer.setOpenCallback(&openCallback);
        wsServer.setCloseCallback(&closeCallback);
#ifdef USE_TLS
        wsServer.setTlsInitCallback([&](Server& server, websocketpp::connection_hdl hdl)
        {
            return tlsInit(config.getTlsCertificateFile(), server, hdl);
        });
#endif
        wsServer.setRequestCallback([&](Server& server, const Server::client_request_t& req)
        {
            requestCallback(server, synchedVault, req);
        });

        try
        {
            cout << "Starting websocket server on port " << config.getWebSocketPort() << "..." << flush;
            LOGGER(info) << "Starting websocket server on port " << config.getWebSocketPort() << "..." << endl;
            wsServer.start();
            cout << "done." << endl;
            LOGGER(info) << "done." << endl;
        }
        catch (const exception& e)
        {
            LOGGER(error) << "Error starting websocket server: " << e.what() << endl;
            cout << endl;
            cerr << "Error starting websocket server: " << e.what() << endl;
            return 1;
        }

        // SYNC STATUS CHANGE
        synchedVault.subscribeStatusChanged([&](SynchedVault::status_t status)
        {
            string syncStatusJson = json_spirit::write_string<json_spirit::Value>(getSyncStatusObject(synchedVault));
            LOGGER(debug) << "Sync status changed: " << syncStatusJson << endl;
            stringstream msg;
            msg << "{\"event\":\"syncstatuschanged\", \"data\":" << syncStatusJson << "}";
            wsServer.sendChannel("syncstatuschanged", msg.str());
        });
        addChannel("syncstatuschanged");
        addChannelToSet("all", "syncstatuschanged");

        // TX INSERTED 
        synchedVault.subscribeTxInserted([&](shared_ptr<Tx> tx)
        {
            using namespace json_spirit;

            try
            {
                string hash = uchar_vector(tx->hash()).getHex();
                string status = Tx::getStatusString(tx->status());
                uint32_t confirmations = synchedVault.getVault()->getTxConfirmations(tx);
                uint32_t height = tx->blockheader() ? tx->blockheader()->height() : 0;

                LOGGER(debug) << "Transaction inserted: " << hash << " Status: " << status << " Confirmations: " << confirmations << " Height: " << height << endl;

                Object txData;
                txData.push_back(Pair("hash", hash));
                txData.push_back(Pair("status", status));
                txData.push_back(Pair("confirmations", (uint64_t)confirmations));
                txData.push_back(Pair("height", (uint64_t)height));

                {
                    stringstream msg;
                    msg << "{\"event\":\"txinserted\", \"data\":" << write_string<Value>(txData) << "}";
                    wsServer.sendChannel("txinserted", msg.str());
                }
                {
                    Value txVal;
                    if (!read_string(tx->toJson(false), txVal) || txVal.type() != obj_type) throw InternalTxJsonInvalidException();
                    Object txObj = txVal.get_obj();

                    txObj.push_back(Pair("confirmations", (uint64_t)confirmations));
                    stringstream msg;
                    msg << "{\"event\":\"txinsertedjson\", \"data\":" << write_string<Value>(txObj) << "}";
                    wsServer.sendChannel("txinsertedjson", msg.str());
                }
                {
                    Object rawTxData(txData);
                    rawTxData.push_back(Pair("rawtx", uchar_vector(tx->raw()).getHex()));
                    stringstream msg;
                    msg << "{\"event\":\"txinsertedraw\", \"data\":" << write_string<Value>(rawTxData) << "}";
                    wsServer.sendChannel("txinsertedraw", msg.str());
                }
                {
                    Object serializedTxData(txData);
                    serializedTxData.push_back(Pair("serializedtx", tx->toSerialized()));
                    stringstream msg;
                    msg << "{\"event\":\"txinsertedserialized\", \"data\":" << write_string<Value>(serializedTxData) << "}";
                    wsServer.sendChannel("txinsertedserialized", msg.str());
                }
            }
            catch (const exception& e)
            {
                LOGGER(error) << "txinserted handler error: " << e.what() << endl;
            }
        });
        addChannel("txinserted");
        addChannel("txinsertedjson");
        addChannel("txinsertedraw");
        addChannel("txinsertedserialized");

        addChannelToSet("tx",           "txinserted");
        addChannelToSet("txjson",       "txinsertedjson");
        addChannelToSet("txraw",        "txinsertedraw");
        addChannelToSet("txserialized", "txinsertedserialized");

        addChannelToSet("all",          "txinserted");
        addChannelToSet("all",          "txinsertedjson");
        addChannelToSet("all",          "txinsertedraw");
        addChannelToSet("all",          "txinsertedserialized");

        // TX UPDATED
        synchedVault.subscribeTxUpdated([&](std::shared_ptr<Tx> tx)
        {
            using namespace json_spirit;

            try
            {
                string hash = uchar_vector(tx->hash()).getHex();
                string status = Tx::getStatusString(tx->status());
                uint32_t confirmations = synchedVault.getVault()->getTxConfirmations(tx);
                uint32_t height = tx->blockheader() ? tx->blockheader()->height() : 0;

                LOGGER(debug) << "Transaction updated: " << hash << " Status: " << status << " Confirmations: " << confirmations << " Height: " << height << endl;

                Object txData;
                txData.push_back(Pair("hash", hash));
                txData.push_back(Pair("status", status));
                txData.push_back(Pair("confirmations", (uint64_t)confirmations));
                txData.push_back(Pair("height", (uint64_t)height));

                {
                    stringstream msg;
                    msg << "{\"event\":\"txupdated\", \"data\":" << write_string<Value>(txData) << "}";
                    wsServer.sendChannel("txupdated", msg.str());
                }
                {
                    Value txVal;
                    if (!read_string(tx->toJson(false), txVal) || txVal.type() != obj_type) throw InternalTxJsonInvalidException();
                    Object txObj = txVal.get_obj();

                    txObj.push_back(Pair("confirmations", (uint64_t)confirmations));
                    stringstream msg;
                    msg << "{\"event\":\"txupdatedjson\", \"data\":" << write_string<Value>(txObj) << "}";
                    wsServer.sendChannel("txupdatedjson", msg.str());
                }
                {
                    Object rawTxData(txData);
                    rawTxData.push_back(Pair("rawtx", uchar_vector(tx->raw()).getHex()));
                    stringstream msg;
                    msg << "{\"event\":\"txupdatedraw\", \"data\":" << write_string<Value>(rawTxData) << "}";
                    wsServer.sendChannel("txupdatedraw", msg.str());
                }
                {
                    Object serializedTxData(txData);
                    serializedTxData.push_back(Pair("serializedtx", tx->toSerialized()));
                    stringstream msg;
                    msg << "{\"event\":\"txupdatedserialized\", \"data\":" << write_string<Value>(serializedTxData) << "}";
                    wsServer.sendChannel("txupdatedserialized", msg.str());
                }
            }
            catch (const exception& e)
            {
                LOGGER(error) << "txupdated handler error: " << e.what() << endl;
            }
        });
        addChannel("txupdated");
        addChannel("txupdatedjson");
        addChannel("txupdatedraw");
        addChannel("txupdatedserialized");

        addChannelToSet("tx",           "txupdated");
        addChannelToSet("txjson",       "txupdatedjson");
        addChannelToSet("txraw",        "txupdatedraw");
        addChannelToSet("txserialized", "txupdatedserialized");

        addChannelToSet("all",          "txupdated");
        addChannelToSet("all",          "txupdatedjson");
        addChannelToSet("all",          "txupdatedraw");
        addChannelToSet("all",          "txupdatedserialized");

        // MERKLE BLOCK INSERTED
        synchedVault.subscribeMerkleBlockInserted([&](std::shared_ptr<MerkleBlock> merkleblock)
        {
            LOGGER(debug) << "Merkle block inserted: " << uchar_vector(merkleblock->blockheader()->hash()).getHex() << " Height: " << merkleblock->blockheader()->height() << endl;

            //if (synchedVault.getStatus() != SynchedVault::SYNCHED) return;

            std::stringstream msg;
            msg << "{\"event\":\"merkleblockinserted\", \"data\":" << merkleblock->toJson() << "}";
            wsServer.sendChannel("merkleblockinserted", msg.str());
        });
        addChannel("merkleblockinserted");

        addChannelToSet("all",          "merkleblockinserted");




        if (config.getSync())
        {
            std::string blockTreeFile = config.getDataDir() + "/blocktree.dat"; 
            cout << "Loading headers from " << blockTreeFile << endl;
            LOGGER(info) << "Loading headers from " << blockTreeFile << endl;
            synchedVault.loadHeaders(blockTreeFile, false,
                [](const CoinQBlockTreeMem& blockTree) {
                    std::stringstream progress;
                    progress << "Height: " << blockTree.getBestHeight() << " / " << "Total Work: " << blockTree.getTotalWork().getDec();
                    cout << "Loaded headers - " << progress.str() << endl;
                    return !g_bShutdown;
                });

            if (g_bShutdown)
            {
                cout << "Interrupted." << endl;
                LOGGER(info) << "Interrupted." << endl;
                wsServer.stop();
                return 0;
            }

            cout << "Attempting to sync with " << config.getPeerHost() << ":" << config.getPeerPort() << endl;
            LOGGER(info) << "Attempting to sync with " << config.getPeerHost() << ":" << config.getPeerPort() << endl;
            synchedVault.startSync(config.getPeerHost(), config.getPeerPort());
        }

        while (!g_bShutdown) { std::this_thread::sleep_for(std::chrono::microseconds(200)); }

        cout << "Stopping vault sync..." << flush;
        LOGGER(info) << "Stopping vault sync..." << endl;
        synchedVault.stopSync();
        cout << "done." << endl;
        LOGGER(info) << "done." << endl;

        cout << "Stopping websocket server..." << flush;
        LOGGER(info) << "Stopping websocket server..." << endl;
        wsServer.stop();
        cout << "done." << endl;
        LOGGER(info) << "done." << endl;
    }
    catch (const stdutils::custom_error& e)
    {
        cerr << "Error: " << e.what() << endl;
        LOGGER(error) << e.what() << endl;

        cout << "exiting." << endl << endl;
        LOGGER(info) << "exiting." << endl << endl;

        return e.has_code() ? e.code() : -1;
    }
    catch (const std::exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        LOGGER(error) << e.what() << endl;

        cout << "exiting." << endl << endl;
        LOGGER(info) << "exiting." << endl << endl;

        return -1;
    }

    cout << "exiting." << endl << endl;
    LOGGER(info) << "exiting." << endl << endl;
    return 0;
}
