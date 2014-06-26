///////////////////////////////////////////////////////////////////////////////
//
// CoinSocket
//
// main.cpp
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
//

#include <CoinDB/SynchedVault.h>
#include <CoinQ/CoinQ_script.h>
#include <CoinCore/Base58Check.h>
#include <CoinCore/random.h>

#ifdef USE_TLS
    #include <WebSocketServer/WebSocketServerTls.h>
    typedef WebSocket::ServerTls WebSocketServer;
#else
    #include <WebSocketServer/WebSocketServer.h>
    typedef WebSocket::Server WebSocketServer;
#endif

#include <logger/logger.h>

#include "config.h"
#include "commands.h"

#include <iostream>
#include <signal.h>

#include <thread>
#include <chrono>

using namespace CoinDB;
using namespace std;

// Globals
command_map_t g_command_map;
bool g_bShutdown = false;

// Callbacks
void finish(int sig)
{
    LOGGER(info) << "Stopping..." << endl;
    g_bShutdown = true;
}

void openCallback(WebSocketServer& server, websocketpp::connection_hdl hdl)
{
    LOGGER(info) << "Client " << server.getRemoteEndpoint(hdl) << " connected as " << hdl.lock().get() << "." << endl;

    JsonRpc::Response res;
    res.setResult("connected");
    server.send(hdl, res);
}

void closeCallback(WebSocketServer& server, websocketpp::connection_hdl hdl)
{
    LOGGER(info) << "Client " << server.getRemoteEndpoint(hdl) << " disconnected as " << hdl.lock().get() << "." << endl;
}

#ifdef USE_TLS
WebSocketServer::context_ptr tlsInit(const std::string& tlsCertificateFile, WebSocketServer& server, websocketpp::connection_hdl hdl)
{
    LOGGER(info) << "tlsInit called with hdl " << hdl.lock().get() << "." << endl;
 
    WebSocketServer::context_ptr ctx(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv1));
 
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

void requestCallback(SynchedVault& synchedVault, WebSocketServer& server, const WebSocketServer::client_request_t& req)
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
            throw std::runtime_error("Invalid method.");

        Value result = it->second(synchedVault, params);
        response.setResult(result, id);
    }
    catch (const exception& e)
    {
        response.setError(e.what(), id);
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

    std::string logFile = config.getDataDir() + "/coinsocket.log"; 
    INIT_LOGGER(logFile.c_str());

    signal(SIGINT, &finish);
    signal(SIGTERM, &finish);

    {
        SynchedVault synchedVault(config.getDataDir() + "/blocktree.dat");

        initCommandMap(g_command_map);
        WebSocketServer wsServer(config.getWebSocketPort(), config.getAllowedIps());
        wsServer.setOpenCallback(&openCallback);
        wsServer.setCloseCallback(&closeCallback);
#ifdef USE_TLS
        wsServer.setTlsInitCallback([&](WebSocketServer& server, websocketpp::connection_hdl hdl)
        {
            return tlsInit(config.getTlsCertificateFile(), server, hdl);
        });
#endif
        wsServer.setRequestCallback([&](WebSocketServer& server, const WebSocketServer::client_request_t& req)
        {
            requestCallback(synchedVault, server, req);
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
        
        synchedVault.subscribeTxInserted([&](std::shared_ptr<Tx> tx)
        {
            std::string hash = uchar_vector(tx->hash()).getHex();
            LOGGER(debug) << "Transaction inserted: " << hash << endl;
            std::stringstream msg;
            msg << "{\"type\":\"txinserted\", \"tx\":" << tx->toJson() << "}";
            wsServer.sendAll(msg.str());
        });

        synchedVault.subscribeTxStatusChanged([&](std::shared_ptr<Tx> tx)
        {
            LOGGER(debug) << "Transaction status changed: " << uchar_vector(tx->hash()).getHex() << " New status: " << Tx::getStatusString(tx->status()) << endl;

            std::stringstream msg;
            msg << "{\"type\":\"txstatuschanged\", \"tx\":" << tx->toJson() << "}";
            wsServer.sendAll(msg.str());
        });

        synchedVault.subscribeMerkleBlockInserted([&](std::shared_ptr<MerkleBlock> merkleblock)
        {
            LOGGER(debug) << "Merkle block inserted: " << uchar_vector(merkleblock->blockheader()->hash()).getHex() << " Height: " << merkleblock->blockheader()->height() << endl;

            std::stringstream msg;
            msg << "{\"type\":\"merkleblockinserted\", \"merkleblock\":" << merkleblock->toJson() << "}";
            wsServer.sendAll(msg.str());
        });

        try
        {
            cout << "Opening vault " << config.getDatabaseName() << endl;
            LOGGER(info) << "Opening vault " << config.getDatabaseName() << endl;
            synchedVault.openVault(config.getDatabaseUser(), config.getDatabasePassword(), config.getDatabaseName());
            cout << "Attempting to sync with " << config.getPeerHost() << ":" << config.getPeerPort() << endl;
            LOGGER(info) << "Attempting to sync with " << config.getPeerHost() << ":" << config.getPeerPort() << endl;
            synchedVault.startSync(config.getPeerHost(), config.getPeerPort());
        }
        catch (const std::exception& e)
        {
            cerr << "Error: " << e.what() << endl;
            return 1;
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

    cout << "exiting." << endl << endl;
    LOGGER(info) << "exiting." << endl << endl;
    return 0;
}
