///////////////////////////////////////////////////////////////////////////////
//
// CoinSocket
//
// CoinSocket.cpp
//
// Copyright (c) 2014-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
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
#include "coinparams.h"
#include "alerts.h"
#include "commands.h"
#include "events.h"

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
bool g_bDisconnected = false;

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

    stringstream msg;
#ifdef OLD_OPEN_CALLBACK
    msg << "{\"result\":\"connected\", \"error\":null, \"id\":null}";
#else
    msg << "{\"event\":\"connected\", \"data\":{}}";
#endif
    server.send(hdl, msg.str());
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
                         boost::asio::ssl::context::no_sslv3 |
                         boost::asio::ssl::context::single_dh_use);
        //ctx->set_password_callback(bind([]() { return "test"; }));
        //ctx->use_certificate_chain_file(tlsCertificateFile);
        ctx->use_certificate_file(tlsCertificateFile, boost::asio::ssl::context::pem);
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

void trySendStartedAlert()
{
    CoinSocketConfig& config = getConfig();
    SmtpTls& smtpTls = getSmtpTls();
    if (smtpTls.isSet())
    {
        cout << "Sending instance started email alert..." << endl;
        LOGGER(info) << "Sending instance started email alert..." << endl;
        try
        {
            stringstream subject;
            subject << "CoinSocket: " << config.getInstanceName() << " started";
            smtpTls.setSubject(subject.str());
            stringstream body;
            body << "Instance name: " << config.getInstanceName();
            LOGGER(debug) << "Body: " << body.str() << endl;
            smtpTls.setBody(body.str());
            smtpTls.send();
        }
        catch (const exception& e)
        {
            cerr << "Error sending email: " << e.what() << endl;
            LOGGER(error) << "Error sending email: " << e.what() << endl;
        }
    }
}

void trySendStoppedAlert(const std::string& errmsg = std::string())
{
    CoinSocketConfig& config = getConfig();
    SmtpTls& smtpTls = getSmtpTls();
    if (smtpTls.isSet())
    {
        cout << "Sending instance shutdown email alert..." << endl;
        LOGGER(info) << "Sending instance shutdown email alert..." << endl;
        try
        {
            stringstream subject;
            subject << "CoinSocket: " << config.getInstanceName() << " stopped";
            smtpTls.setSubject(subject.str());
            stringstream body;
            body << "Instance name: " << config.getInstanceName() << "\r\n";
            if (!errmsg.empty())
            {
                body << "Error: " << errmsg << "\r\n";
            }
            smtpTls.setBody(body.str());
            smtpTls.send();
        }
        catch (const exception& e)
        {
            cerr << "Error sending email: " << e.what() << endl;
            LOGGER(error) << "Error sending email: " << e.what() << endl;
        }
    }
}


// Main program
int main(int argc, char* argv[])
{

    try
    {
        initConfig(argc, argv);
    }
    catch (const std::exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    CoinSocketConfig& config = getConfig();
    if (config.help())
    {
        cout << config.getHelpOptions() << endl;
        return 0;
    }

    setDocumentDir(config.getDocumentDir());
    setCoinParams(config.getCoinParams());
    if (config.getSendEmailAlerts())
    {
        setSmtpTls(config.getSmtpUser(), config.getSmtpPassword(), config.getSmtpUrl());
        getSmtpTls().setFrom(config.getSmtpFrom());
        const vector<string>& emailAlerts = config.getEmailAlerts();
        for (auto& to: emailAlerts) { getSmtpTls().addTo(to); }
    }

    g_connectKey = string("/") + config.getConnectKey();
    std::string logFile = config.getDataDir() + "/coinsocket.log"; 
    INIT_LOGGER(logFile.c_str());

    signal(SIGINT, &finish);
    signal(SIGTERM, &finish);

    try
    {
        SynchedVault synchedVault(config.getCoinParams());

        cout << "Opening vault " << config.getDatabaseName() << endl;
        LOGGER(info) << endl << endl << endl << endl;
        LOGGER(info) << "Opening vault " << config.getDatabaseName() << endl;
        synchedVault.openVault(config.getDatabaseUser(), config.getDatabasePassword(), config.getDatabaseName(), false, SCHEMA_VERSION, string(), config.getMigrate());

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
        synchedVault.subscribeStatusChanged([&](SynchedVault::status_t status) { sendStatusEvent(wsServer, synchedVault); });

        addChannel("status");
        addChannelToSet("all", "status");

        // TX INSERTED 
        synchedVault.subscribeTxInserted([&](shared_ptr<Tx> tx) { sendTxChannelEvent(INSERTED, wsServer, synchedVault, tx); });

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
        synchedVault.subscribeTxUpdated([&](std::shared_ptr<Tx> tx) { sendTxChannelEvent(UPDATED, wsServer, synchedVault, tx); });

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

        // TX DELETED
        synchedVault.subscribeTxDeleted([&](std::shared_ptr<Tx> tx) { sendTxChannelEvent(DELETED, wsServer, synchedVault, tx); });

        addChannel("txdeleted");
        addChannel("txdeletedjson");
        addChannel("txdeletedraw");
        addChannel("txdeletedserialized");

        addChannelToSet("tx",           "txdeleted");
        addChannelToSet("txjson",       "txdeletedjson");
        addChannelToSet("txraw",        "txdeletedraw");
        addChannelToSet("txserialized", "txdeletedserialized");

        addChannelToSet("all",          "txdeleted");
        addChannelToSet("all",          "txdeletedjson");
        addChannelToSet("all",          "txdeletedraw");
        addChannelToSet("all",          "txdeletedserialized");

        addChannel("txapprovedjson");
        addChannel("txcanceledjson");
        addChannel("txrejectedjson");

        addChannelToSet("txjson",       "txapprovedjson");
        addChannelToSet("txjson",       "txcanceledjson");
        addChannelToSet("txjson",       "txrejectedjson");

        addChannelToSet("all",          "txapprovedjson");
        addChannelToSet("all",          "txcanceledjson");
        addChannelToSet("all",          "txrejectedjson");

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


        // PEER DISCONNECTED
        synchedVault.subscribePeerDisconnected([&]() {
            g_bDisconnected = true;
        });


        if (config.getSync())
        {
            std::string blockTreeFile = config.getBlockTreeFile();
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

            cout << "Attempting to sync with " << config.getPeerHost() << ":" << config.getPeerPort() << " on network " << config.getNetworkName() << endl;
            LOGGER(info) << "Attempting to sync with " << config.getPeerHost() << ":" << config.getPeerPort() << " on network " << config.getNetworkName() << endl;
            synchedVault.startSync(config.getPeerHost(), config.getPeerPort());
        }

        trySendStartedAlert();

        std::thread* reconnectPeerThread = nullptr;

        while (!g_bShutdown)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(200));
            if (g_bDisconnected)
            {
                g_bDisconnected = false;
                LOGGER(info) << "Peer disconnected." << endl;
                synchedVault.stopSync();
                if (reconnectPeerThread)
                {
                    reconnectPeerThread->join();
                    delete reconnectPeerThread;
                }

                reconnectPeerThread = new std::thread([&]() {
                    LOGGER(info) << "Attempting to reconnect in 5 seconds..." << endl;
                    std::this_thread::sleep_for(std::chrono::microseconds(5000000));
                    synchedVault.startSync(config.getPeerHost(), config.getPeerPort());
                });
            }
        }

        cout << "Stopping vault sync..." << flush;
        LOGGER(info) << "Stopping vault sync..." << endl;
        synchedVault.stopSync();
        if (reconnectPeerThread)
        {
            reconnectPeerThread->join();
            delete reconnectPeerThread;
        }
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

        trySendStoppedAlert(e.what());

        cout << "exiting." << endl << endl;
        LOGGER(info) << "exiting." << endl << endl;

        return e.has_code() ? e.code() : -1;
    }
    catch (const std::exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        LOGGER(error) << e.what() << endl;

        trySendStoppedAlert(e.what());

        cout << "exiting." << endl << endl;
        LOGGER(info) << "exiting." << endl << endl;

        return -1;
    }

    trySendStoppedAlert();

    cout << "exiting." << endl << endl;
    LOGGER(info) << "exiting." << endl << endl;
    return 0;
}
