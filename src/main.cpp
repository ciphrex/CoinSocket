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
#include <WebSocketServer/WebSocketServer.h>
#include <logger/logger.h>

#include "config.h"

#include <iostream>
#include <signal.h>

#include <thread>
#include <chrono>

using namespace CoinDB;
using namespace std;

const string WS_PORT = "12345";

bool g_bShutdown = false;

void finish(int sig)
{
    LOGGER(debug) << "Stopping..." << endl;
    g_bShutdown = true;
}

void openCallback(WebSocket::Server& server, websocketpp::connection_hdl hdl)
{
    cout << "Client " << hdl.lock().get() << " connected." << endl;

    JsonRpc::Response res;
    res.setResult("connected");
    server.send(hdl, res);
}

void closeCallback(WebSocket::Server& server, websocketpp::connection_hdl hdl)
{
    cout << "Client " << hdl.lock().get() << " disconnected." << endl;
}

void requestCallback(WebSocket::Server& server, const WebSocket::Server::client_request_t& req)
{
    JsonRpc::Response response;

    const string& method = req.second.getMethod();
//    const json_spirit::Array& params = req.second.getParams();

    try
    {
        if (method == "subscribe")
        {
        }
        else
        {
            throw std::runtime_error("Invalid method.");
        }
    }
    catch (const exception& e)
    {
        response.setError(e.what(), req.second.getId());
    }

    server.send(req.first, response);
}

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
    
    INIT_LOGGER("coinsocket.log");

    signal(SIGINT, &finish);

    WebSocket::Server wsServer(WS_PORT);
    wsServer.setOpenCallback(&openCallback);
    wsServer.setCloseCallback(&closeCallback);
    wsServer.setRequestCallback(&requestCallback);
    try
    {
        cout << "Starting websocket server on port " << WS_PORT << "..." << flush;
        wsServer.start();
        cout << "done." << endl;
    }
    catch (const exception& e)
    {
        cout << endl;
        cerr << "Error starting websocket server: " << e.what() << endl;
        return 1;
    }
    
    SynchedVault synchedVault;
    synchedVault.subscribeTxInserted([](std::shared_ptr<Tx> tx)
    {
        cout << "Transaction inserted: " << uchar_vector(tx->hash()).getHex() << endl;
    });
    synchedVault.subscribeTxStatusChanged([](std::shared_ptr<Tx> tx)
    {
        cout << "Transaction status changed: " << uchar_vector(tx->hash()).getHex() << " New status: " << Tx::getStatusString(tx->status()) << endl;
    });
    synchedVault.subscribeMerkleBlockInserted([](std::shared_ptr<MerkleBlock> merkleblock)
    {
        cout << "Merkle block inserted: " << uchar_vector(merkleblock->blockheader()->hash()).getHex() << " Height: " << merkleblock->blockheader()->height() << endl;
    });

    try
    {
        cout << "Opening vault " << config.getDatabaseFile() << endl;
        synchedVault.openVault(config.getDatabaseFile());
        cout << "Attempting to sync with " << config.getPeerHost() << ":" << config.getPeerPort() << endl;
        synchedVault.startSync(config.getPeerHost(), config.getPeerPort());
    }
    catch (const std::exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    while (!g_bShutdown) { std::this_thread::sleep_for(std::chrono::microseconds(200)); }

    cout << "Stopping websocket server..." << flush;
    wsServer.stop();
    cout << "done." << endl;

    return 0;
}
