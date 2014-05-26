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
#include <WebSocketServer/WebSocketServer.h>
#include <logger/logger.h>

#include "config.h"

#include <iostream>
#include <signal.h>

#include <thread>
#include <chrono>

using namespace CoinDB;
using namespace std;

// TODO: Get this from a config file
const unsigned char BITCOIN_BASE58_VERSIONS[] = { 0x00, 0x05 };

// Data formatting
std::string getAddressFromScript(const bytes_t& script, const unsigned char base58Versions[])
{
    using namespace CoinQ::Script;

    payee_t payee = getScriptPubKeyPayee(script);
    if (payee.first == SCRIPT_PUBKEY_PAY_TO_PUBKEY_HASH)
        return toBase58Check(payee.second, base58Versions[0]);
    else if (payee.first == SCRIPT_PUBKEY_PAY_TO_SCRIPT_HASH)
        return toBase58Check(payee.second, base58Versions[1]);
    else
        return "N/A";
}

bool g_bShutdown = false;

void finish(int sig)
{
    LOGGER(info) << "Stopping..." << endl;
    g_bShutdown = true;
}

void openCallback(WebSocket::Server& server, websocketpp::connection_hdl hdl)
{
    LOGGER(info) << "Client " << hdl.lock().get() << " connected." << endl;

    JsonRpc::Response res;
    res.setResult("connected");
    server.send(hdl, res);
}

void closeCallback(WebSocket::Server& server, websocketpp::connection_hdl hdl)
{
    LOGGER(info) << "Client " << hdl.lock().get() << " disconnected." << endl;
}

void requestCallback(SynchedVault& synchedVault, WebSocket::Server& server, const WebSocket::Server::client_request_t& req)
{
    using namespace json_spirit;

    Vault* vault = synchedVault.getVault();

    JsonRpc::Response response;

    const string& method = req.second.getMethod();
    const Array& params = req.second.getParams();

    try
    {
        if (method == "status")
        {
            Object result;
            result.push_back(Pair("name", vault->getName()));
            result.push_back(Pair("schema", (uint64_t)vault->getSchemaVersion()));
            result.push_back(Pair("horizon", (uint64_t)vault->getHorizonTimestamp()));
            response.setResult(result);
        }
        else if (method == "newkeychain")
        {
            if (params.size() != 1)
                throw std::runtime_error("Invalid parameters.");

            std::string keychainName = params[0].get_str();
            vault->newKeychain(keychainName, random_bytes(32));
            response.setResult("success");
        }
        else if (method == "renamekeychain")
        {
            if (params.size() != 2)
                throw std::runtime_error("Invalid parameters.");

            std::string oldName = params[0].get_str();
            std::string newName = params[1].get_str();
            vault->renameKeychain(oldName, newName);
            response.setResult("success"); 
        }
        else if (method == "keychaininfo")
        {
            if (params.size() != 1)
                throw std::runtime_error("Invalid parameters.");

            std::string keychainName = params[0].get_str();
            std::shared_ptr<Keychain> keychain = vault->getKeychain(keychainName);
            Object keychainInfo;
            keychainInfo.push_back(Pair("id", (uint64_t)keychain->id()));
            keychainInfo.push_back(Pair("name", keychain->name()));
            keychainInfo.push_back(Pair("depth", (int)keychain->depth()));
            keychainInfo.push_back(Pair("parent_fp", (uint64_t)keychain->parent_fp()));
            keychainInfo.push_back(Pair("child_num", (uint64_t)keychain->child_num()));
            keychainInfo.push_back(Pair("pubkey", uchar_vector(keychain->pubkey()).getHex()));
            keychainInfo.push_back(Pair("hash", uchar_vector(keychain->hash()).getHex()));
            response.setResult(keychainInfo);
        }
        else if (method == "keychains")
        {
            if (params.size() > 2)
                throw std::runtime_error("Invalid parameters.");

            std::string accountName;
            if (params.size() > 0) accountName = params[0].get_str();

            bool showHidden = params.size() > 1 && params[1].get_bool();

            vector<KeychainView> views = vault->getRootKeychainViews(accountName, showHidden);
            vector<Object> keychainObjects;
            for (auto& view: views)
            {
                Object obj;
                obj.push_back(Pair("id", (uint64_t)view.id));
                obj.push_back(Pair("name", view.name));
                obj.push_back(Pair("private", view.is_private));
                obj.push_back(Pair("encrypted", view.is_encrypted));
                obj.push_back(Pair("locked", view.is_locked));
                obj.push_back(Pair("hash", uchar_vector(view.hash).getHex()));
                keychainObjects.push_back(obj);
            }

            Object result;
            result.push_back(Pair("keychains", Array(keychainObjects.begin(), keychainObjects.end())));
            response.setResult(result); 
        }
        else if (method == "newaccount")
        {
            if (params.size() < 3 || params.size() > 18)
                throw std::runtime_error("Invalid parameters.");

            std::string accountName = params[0].get_str();
            int minsigs = params[1].get_int();
            if (minsigs > ((int)params.size() - 2))
                throw std::runtime_error("Invalid parameters.");

            std::vector<std::string> keychainNames;
            for (size_t i = 2; i < params.size(); i++)
                keychainNames.push_back(params[i].get_str());

            vault->unlockChainCodes(secure_bytes_t()); // TODO: add a method to unlock chaincodes using passphrase
            vault->newAccount(accountName, minsigs, keychainNames);
            vault->lockChainCodes();
            response.setResult("success");
        }
        else if (method == "renameaccount")
        {
            if (params.size() != 2)
                throw std::runtime_error("Invalid parameters.");

            std::string oldName = params[0].get_str();
            std::string newName = params[1].get_str();
            vault->renameAccount(oldName, newName);
            response.setResult("success");
        }
        else if (method == "accountinfo")
        {
            if (params.size() != 1)
                throw std::runtime_error("Invalid parameters.");

            std::string accountName = params[0].get_str();
            AccountInfo accountInfo = vault->getAccountInfo(accountName);
            uint64_t balance = vault->getAccountBalance(accountName, 0);
            uint64_t confirmedBalance = vault->getAccountBalance(accountName, 1);

            Object result;
            result.push_back(Pair("id", (uint64_t)accountInfo.id()));
            result.push_back(Pair("name", accountInfo.name()));
            result.push_back(Pair("minsigs", (int)accountInfo.minsigs()));
            result.push_back(Pair("keychains", Array(accountInfo.keychain_names().begin(), accountInfo.keychain_names().end())));
            result.push_back(Pair("unused_pool_size", (uint64_t)accountInfo.unused_pool_size()));
            result.push_back(Pair("time_created", (uint64_t)accountInfo.time_created()));
            result.push_back(Pair("bins", Array(accountInfo.bin_names().begin(), accountInfo.bin_names().end())));
            result.push_back(Pair("balance", balance));
            result.push_back(Pair("confirmed_balance", confirmedBalance));
            response.setResult(result); 
        }
        else if (method == "listaccounts")
        {
            if (params.size() > 0)
                throw std::runtime_error("Invalid parameters.");

            vector<AccountInfo> accounts = vault->getAllAccountInfo();
            vector<Object> accountObjects;
            for (auto& account: accounts)
            {
                Object accountObject;
                accountObject.push_back(Pair("id", (uint64_t)account.id()));
                accountObject.push_back(Pair("name", account.name()));
                accountObject.push_back(Pair("minsigs", (int)account.minsigs()));

                accountObject.push_back(Pair("keychains", Array(account.keychain_names().begin(), account.keychain_names().end())));

                accountObjects.push_back(accountObject);
            }
            Object result;
            result.push_back(Pair("accounts", Array(accountObjects.begin(), accountObjects.end())));
            response.setResult(result);
        }
        else if (method == "issuescript")
        {
            if (params.size() < 1 || params.size() > 3)
                throw std::runtime_error("Invalid parameters.");

            std::string accountName = params[0].get_str();
            std::string label;
            if (params.size() > 1) label = params[1].get_str();
            std::string binName;
            if (params.size() > 2) binName = params[2].get_str();
            if (binName.empty()) binName = DEFAULT_BIN_NAME;

            std::shared_ptr<SigningScript> script = vault->issueSigningScript(accountName, binName, label);

            Object result;
            result.push_back(Pair("account", accountName));
            result.push_back(Pair("label", label));
            result.push_back(Pair("account_bin", binName));
            result.push_back(Pair("script", uchar_vector(script->txoutscript()).getHex()));
            result.push_back(Pair("address", getAddressFromScript(script->txoutscript(), BITCOIN_BASE58_VERSIONS)));
            response.setResult(result);
        }
        else if (method == "bestblockinfo")
        {
            if (params.size() > 0)
                throw std::runtime_error("Invalid parameters.");

            uint64_t height = vault->getBestHeight();

            Object result;
            result.push_back(Pair("height", height));
            response.setResult(result);
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

    std::string logFile = config.getDataDir() + "/coinsocket.log"; 
    INIT_LOGGER(logFile.c_str());

    signal(SIGINT, &finish);
    signal(SIGTERM, &finish);

    SynchedVault synchedVault(config.getDataDir() + "/blocktree.dat");

    WebSocket::Server wsServer(config.getWebSocketPort(), config.getAllowedIps());
    wsServer.setOpenCallback(&openCallback);
    wsServer.setCloseCallback(&closeCallback);
    wsServer.setRequestCallback([&](WebSocket::Server& server, const WebSocket::Server::client_request_t& req)
    {
        requestCallback(synchedVault, server, req);
    });

    try
    {
        cout << "Starting websocket server on port " << config.getWebSocketPort() << "..." << flush;
        LOGGER(info) << "Starting websocket server on port " << config.getWebSocketPort() << "..." << flush;
        wsServer.start();
        cout << "done." << endl;
        LOGGER(info) << "done." << endl;
    }
    catch (const exception& e)
    {
        cout << endl;
        LOGGER(info) << endl;
        cerr << "Error starting websocket server: " << e.what() << endl;
        return 1;
    }
    
    synchedVault.subscribeTxInserted([&](std::shared_ptr<Tx> tx)
    {
        std::string hash = uchar_vector(tx->hash()).getHex();
        LOGGER(debug) << "Transaction inserted: " << hash << endl;
        std::stringstream msg;
        msg << "{\"type\":\"tx_inserted\", \"tx\":" << tx->toJson() << "}";
        wsServer.sendAll(msg.str());
    });

    synchedVault.subscribeTxStatusChanged([&](std::shared_ptr<Tx> tx)
    {
        LOGGER(debug) << "Transaction status changed: " << uchar_vector(tx->hash()).getHex() << " New status: " << Tx::getStatusString(tx->status()) << endl;
        std::stringstream msg;
        msg << "{\"type\":\"tx_status_changed\", \"tx\":" << tx->toJson() << "}";
        wsServer.sendAll(msg.str());
    });

    synchedVault.subscribeMerkleBlockInserted([&](std::shared_ptr<MerkleBlock> merkleblock)
    {
        LOGGER(debug) << "Merkle block inserted: " << uchar_vector(merkleblock->blockheader()->hash()).getHex() << " Height: " << merkleblock->blockheader()->height() << endl;
    });

    try
    {
        cout << "Opening vault " << config.getDatabaseFile() << endl;
        LOGGER(info) << "Opening vault " << config.getDatabaseFile() << endl;
        synchedVault.openVault(config.getDatabaseFile());
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
    LOGGER(info) << "Stopping vault sync..." << flush;
    synchedVault.stopSync();
    cout << "done." << endl;
    LOGGER(info) << "done." << endl;

    cout << "Stopping websocket server..." << flush;
    LOGGER(info) << "Stopping websocket server..." << flush;
    wsServer.stop();
    cout << "done." << endl;
    LOGGER(info) << "done." << endl;

    return 0;
}
