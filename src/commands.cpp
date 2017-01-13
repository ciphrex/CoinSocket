///////////////////////////////////////////////////////////////////////////////
//
// CoinSocket
//
// commands.cpp
//
// Copyright (c) 2014-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "commands.h"
#include "alerts.h"
#include "events.h"
#include "txproposal.h"
#include "config.h"
#include "coinparams.h"
#include "channels.h"
#include "jsonobjects.h"

#include <CoinQ/CoinQ_script.h>
#include <CoinCore/Base58Check.h>
#include <CoinCore/random.h>

#include "CoinSocketExceptions.h"

#include <WebSocketAPI/Server.h>
#include <CoinDB/SynchedVault.h>

#include <mutex>

using namespace CoinSocket;
using namespace WebSocket;
using namespace json_spirit;
using namespace CoinDB;
using namespace std;

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

// Globals
mutex g_txSubmissionMutex;

static string g_documentDir;
void setDocumentDir(const string& documentDir) { g_documentDir = documentDir; }
const string& getDocumentDir() { return g_documentDir; }

// Channel operations
Value cmd_subscribe(Server& server, websocketpp::connection_hdl hdl, SynchedVault& /*synchedVault*/, const Array& params)
{
    using namespace json_spirit;

    Channels subscriptions;
    for (auto& param: params)
    {
        if (param.type() != str_type) throw CommandInvalidParametersException();
        std::string channel = param.get_str();
        if (channelExists(channel))
        {
            subscriptions.insert(channel);
        }
        else
        {
            ChannelRange range = getChannelRange(channel);
            if (isChannelRangeEmpty(range)) throw CommandInvalidChannelsException();
            for (ChannelSets::iterator it = range.first; it != range.second; ++it) { subscriptions.insert(it->second); }
        }
    }

    for (auto& channel: subscriptions) { server.addToChannel(channel, hdl); }
    return Value("success");
}

Value cmd_unsubscribe(Server& server, websocketpp::connection_hdl hdl, SynchedVault& /*synchedVault*/, const Array& params)
{
    using namespace json_spirit;

    if (params.size() == 0)
    {
        server.removeFromAllChannels(hdl);
        return Value("success");
    }

    Channels subscriptions;
    for (auto& param: params)
    {
        if (param.type() != str_type) throw CommandInvalidParametersException();
        std::string channel = param.get_str();
        if (channelExists(channel))
        {
            subscriptions.insert(channel);
        }
        else
        {
            ChannelRange range = getChannelRange(channel);
            if (isChannelRangeEmpty(range)) throw CommandInvalidChannelsException();
            for (ChannelSets::iterator it = range.first; it != range.second; ++it) { subscriptions.insert(it->second); }
        }
    }

    for (auto& channel: subscriptions) { server.removeFromChannel(channel, hdl); }
    return Value("success");
}

Value cmd_getchannels(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& /*synchedVault*/, const Array& params)
{
    if (params.size() != 0) throw CommandInvalidParametersException();

    return Array(getChannels().begin(), getChannels().end());
}

// Global operations
Value cmd_getstatus(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 0) throw CommandInvalidParametersException();

    Vault* vault = synchedVault.getVault();

    Object result;
    result.push_back(Pair("name", vault->getName()));
    result.push_back(Pair("schema", (uint64_t)vault->getSchemaVersion()));
    result.push_back(Pair("horizontime", (uint64_t)vault->getHorizonTimestamp()));
    result.push_back(Pair("horizonheight", (uint64_t)vault->getHorizonHeight()));
    result.push_back(Pair("syncstatus", getSyncStatusObject(synchedVault)));
    return result;
}

Value cmd_setvaultfromfile(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 1 || params[0].type() != str_type)
        throw CommandInvalidParametersException();

    // TODO: make sure the file is strictly in the allowed dir
    Vault* vault = synchedVault.getVault();
    string filepath = g_documentDir + "/" + params[0].get_str();

    synchedVault.suspendBlockUpdates();
    try
    {   
        vault->importVault(filepath);
        synchedVault.syncBlocks();
        return Value("success");
    }
    catch (const exception& e)
    {
        synchedVault.syncBlocks();
        throw e;
    }
}

Value cmd_exportvaulttofile(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 2 || params[0].type() != str_type || params[1].type() != bool_type)
        throw CommandInvalidParametersException();

    // TODO: make sure the file is strictly in the allowed dir
    Vault* vault = synchedVault.getVault();
    string filepath = g_documentDir + "/" + params[0].get_str();
    vault->exportVault(filepath, params[1].get_bool());
    return Value("success");
}

// Keychain operations
Value cmd_newkeychain(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() < 1 || params.size() > 2 || params[0].type() != str_type || (params.size() > 1 && params[1].type() != str_type))
        throw CommandInvalidParametersException();

    Vault* vault = synchedVault.getVault();

    std::string keychainName = params[0].get_str();
    secure_bytes_t seed = params.size() > 1 ? uchar_vector(params[1].get_str()) : random_bytes(32);
    std::shared_ptr<Keychain> keychain = vault->newKeychain(keychainName, seed);
    return getKeychainObject(*keychain);
}

Value cmd_renamekeychain(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 2 || params[0].type() != str_type || params[0].type() != str_type)
        throw CommandInvalidParametersException();

    Vault* vault = synchedVault.getVault();

    std::string oldName = params[0].get_str();
    std::string newName = params[1].get_str();
    vault->renameKeychain(oldName, newName);
    return Value("success");
}

Value cmd_getkeychaininfo(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 1 || params[0].type() != str_type)
        throw CommandInvalidParametersException();

    Vault* vault = synchedVault.getVault();

    std::string keychainName = params[0].get_str();
    std::shared_ptr<Keychain> keychain = vault->getKeychain(keychainName);
    return getKeychainObject(*keychain);
}

Value cmd_getkeychains(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() > 2)
        throw CommandInvalidParametersException();

    std::string accountName;
    if (params.size() > 0) accountName = params[0].get_str();

    bool showHidden = params.size() > 1 && params[1].get_bool();

    Vault* vault = synchedVault.getVault();

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
    return result;
}

Value cmd_exportbip32(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 1)
        throw CommandInvalidParametersException();

    std::string keychainName = params[0].get_str();

    Vault* vault = synchedVault.getVault();

    bytes_t extendedkey = vault->exportBIP32(keychainName, false);

    Object result;
    result.push_back(Pair("extendedkey", toBase58Check(extendedkey)));
    return result;
}

Value cmd_importbip32(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 2)
        throw CommandInvalidParametersException();

    bytes_t extendedkey;
    if (!fromBase58Check(params[1].get_str(), extendedkey))
        throw CommandInvalidParametersException();

    Vault* vault = synchedVault.getVault();

    vault->importBIP32(params[0].get_str(), extendedkey);
    return Value("success");
}

Value cmd_newaccount(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() < 3 || params.size() > 18)
        throw CommandInvalidParametersException();

    std::string accountName = params[0].get_str();
    int minsigs = params[1].get_int();
    if (minsigs > ((int)params.size() - 2))
        throw CommandInvalidParametersException();

    std::vector<std::string> keychainNames;
    for (size_t i = 2; i < params.size(); i++)
        keychainNames.push_back(params[i].get_str());

    Vault* vault = synchedVault.getVault();

    vault->newAccount(accountName, minsigs, keychainNames);
    synchedVault.syncBlocks();
    AccountInfo accountInfo = vault->getAccountInfo(accountName);
    return getAccountInfoObject(accountInfo);
}

Value cmd_renameaccount(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 2 || params[0].type() != str_type || params[1].type() != str_type)
        throw CommandInvalidParametersException();

    Vault* vault = synchedVault.getVault();

    std::string oldName = params[0].get_str();
    std::string newName = params[1].get_str();
    vault->renameAccount(oldName, newName);
    return Value("success");
}

Value cmd_getaccountinfo(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 1 || params[0].type() != str_type)
        throw CommandInvalidParametersException();

    Vault* vault = synchedVault.getVault();

    std::string accountName = params[0].get_str();
    AccountInfo accountInfo = vault->getAccountInfo(accountName);
    uint64_t balance = vault->getAccountBalance(accountName, 0);
    uint64_t confirmedBalance = vault->getAccountBalance(accountName, 1);

    Object result = getAccountInfoObject(accountInfo);
    result.push_back(Pair("balance", balance));
    result.push_back(Pair("confirmedbalance", confirmedBalance));
    return result;
}

Value cmd_getaccounts(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() > 0)
        throw CommandInvalidParametersException();

    Vault* vault = synchedVault.getVault();

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
    return result;
}

Value cmd_issuescript(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() < 1 || params.size() > 4)
        throw CommandInvalidParametersException();

    Vault* vault = synchedVault.getVault();

    std::string accountName = params[0].get_str();
    std::string label;
    if (params.size() > 1) label = params[1].get_str();
    std::string binName;
    if (params.size() > 2) binName = params[2].get_str();
    if (binName.empty()) binName = DEFAULT_BIN_NAME;
    uint32_t index = params.size() > 3 ? (uint32_t)params[3].get_uint64() : 0;

    std::shared_ptr<SigningScript> script = vault->issueSigningScript(accountName, binName, label, index);
    if (synchedVault.isConnected()) { synchedVault.updateBloomFilter(); }

    std::string address = CoinQ::Script::getAddressForTxOutScript(script->txoutscript(), getCoinParams().address_versions());
    std::string uri = "bitcoin:";
    uri += address;
    if (!label.empty()) { uri += "?label="; uri += label; }

    Object result;
    result.push_back(Pair("account", accountName));
    result.push_back(Pair("label", label));
    result.push_back(Pair("accountbin", binName));
    result.push_back(Pair("index", (uint64_t)script->index()));
    result.push_back(Pair("script", uchar_vector(script->txoutscript()).getHex()));
    result.push_back(Pair("address", address));
    result.push_back(Pair("uri", uri)); 
    return result;
}

Value cmd_issuecontactscript(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() < 2 || params.size() > 4)
        throw CommandInvalidParametersException();

    Vault* vault = synchedVault.getVault();

    std::string accountName = params[0].get_str();
    std::string userName = params[1].get_str();
    if (userName.empty()) throw CommandInvalidParametersException();
    std::string label;
    if (params.size() > 2) label = params[2].get_str();
    std::string binName;
    if (params.size() > 3) binName = params[3].get_str();
    if (binName.empty()) binName = DEFAULT_BIN_NAME;

    std::shared_ptr<SigningScript> script = vault->issueSigningScript(accountName, binName, label, 0, userName);
    if (synchedVault.isConnected()) { synchedVault.updateBloomFilter(); }

    std::string address = CoinQ::Script::getAddressForTxOutScript(script->txoutscript(), getCoinParams().address_versions());
    std::string uri = "bitcoin:";
    uri += address;
    if (!label.empty()) { uri += "?label="; uri += label; }

    Object result;
    result.push_back(Pair("account", accountName));
    result.push_back(Pair("username", userName));
    result.push_back(Pair("label", label));
    result.push_back(Pair("accountbin", binName));
    result.push_back(Pair("index", (uint64_t)script->index()));
    result.push_back(Pair("script", uchar_vector(script->txoutscript()).getHex()));
    result.push_back(Pair("address", address));
    result.push_back(Pair("uri", uri)); 
    return result;
}

Value cmd_importaccountfromfile(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 1 || params[0].type() != str_type)
        throw CommandInvalidParametersException();

    // TODO: make sure the file is strictly in the allowed dir
    Vault* vault = synchedVault.getVault();
    string filepath = g_documentDir + "/" + params[0].get_str();

    synchedVault.suspendBlockUpdates();
    try
    {
        unsigned int privkeysimported = 1;        
        vault->importAccount(filepath, privkeysimported);
        synchedVault.syncBlocks();
        return Value("success");
    }
    catch (const exception& e)
    {
        synchedVault.syncBlocks();
        throw e;
    }
}

Value cmd_exportaccounttofile(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 3 || params[0].type() != str_type || params[1].type() != str_type || params[2].type() != bool_type)
        throw CommandInvalidParametersException();

    // TODO: make sure the file is strictly in the allowed dir
    Vault* vault = synchedVault.getVault();
    string account = params[0].get_str();
    string filepath = g_documentDir + "/" + params[1].get_str();
    vault->exportAccount(account, filepath, params[2].get_bool());
    return Value("success");
}

// Tx operations
Value cmd_synctxs(Server& server, websocketpp::connection_hdl hdl, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() > 1 || (params.size() == 1 && params[0].type() != int_type))
        throw CommandInvalidParametersException();

    Vault* vault = synchedVault.getVault();

    uint32_t minheight = params.size() > 0 ? (uint32_t)params[0].get_uint64() : 0;
    std::vector<TxView> txviews = vault->getTxViews(Tx::ALL, 0, -1, minheight);

    txs_t txs;

    std::vector<Object> txViewObjs; 
    for (auto& txview: txviews)
    {
        shared_ptr<Tx> tx = vault->getTx(txview.hash);
        txs.push_back(tx);
    }

    for (auto& tx: txs)
    {
        sendTxJsonEvent(UPDATED, server, hdl, synchedVault, tx);
    }

    Object result;
    return Value("success");
}

Value cmd_gethistory(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() > 1 || (params.size() == 1 && params[0].type() != int_type))
        throw CommandInvalidParametersException();

    Vault* vault = synchedVault.getVault();

    uint32_t minheight = params.size() > 0 ? (uint32_t)params[0].get_uint64() : 0;
    std::vector<TxView> txviews = vault->getTxViews(Tx::ALL, 0, -1, minheight);

    std::vector<Object> txViewObjs; 
    for (auto& txview: txviews)
    {
        txViewObjs.push_back(getTxViewObject(txview));
    }

    Object result;
    result.push_back(Pair("txs", Array(txViewObjs.begin(), txViewObjs.end())));
    return result;
}

Value cmd_getunsigned(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 0)
        throw CommandInvalidParametersException();

    Vault* vault = synchedVault.getVault();

    std::vector<TxView> txviews = vault->getTxViews(Tx::UNSIGNED);

    std::vector<Object> txViewObjs; 
    for (auto& txview: txviews)
    {
        txViewObjs.push_back(getTxViewObject(txview));
    }

    Object result;
    result.push_back(Pair("txs", Array(txViewObjs.begin(), txViewObjs.end())));
    return result;
}

Value cmd_gettx(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 1)
        throw CommandInvalidParametersException();

    Vault* vault = synchedVault.getVault();

    std::shared_ptr<Tx> tx;
    if (params[0].type() == str_type)
    {
        uchar_vector hash(params[0].get_str());
        tx = vault->getTx(hash); 
    }
    else if (params[0].type() == int_type)
    {
        tx = vault->getTx((unsigned long)params[0].get_uint64());
    }
    else
    {
        throw CommandInvalidParametersException();
    }

/*
    Value txVal;
    if (!read_string(tx->toJson(true, true), txVal))
        throw InternalTxJsonInvalidException(); 

    Object txObj = txVal.get_obj();
*/
    Object txObj = getTxObject(*tx, true, true);
    txObj.push_back(Pair("assettype", getCoinParams().currency_symbol()));

    uint32_t height = tx->blockheader() ? tx->blockheader()->height() : 0;
    bool bFinal = (height > 0) && (synchedVault.getSyncHeight() >= height + getConfig().getMinConf() - 1);
    txObj.push_back(Pair("final", bFinal));

    return txObj;
}

Value cmd_getserializedtx(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 1)
        throw CommandInvalidParametersException();

    Vault* vault = synchedVault.getVault();

    std::shared_ptr<Tx> tx;
    if (params[0].type() == str_type)
    {
        uchar_vector hash(params[0].get_str());
        tx = vault->getTx(hash); 
    }
    else if (params[0].type() == int_type)
    {
        tx = vault->getTx((unsigned long)params[0].get_uint64());
    }
    else
    {
        throw CommandInvalidParametersException();
    }

    string serializedTx = vault->exportTx(tx);

    Object result;
    result.push_back(Pair("hash", uchar_vector(tx->hash()).getHex()));
    result.push_back(Pair("serializedtx", serializedTx));
    return result;
}

Value cmd_getserializedunsignedtxs(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 1 || params[0].type() != str_type) throw CommandInvalidParametersException();

    Vault* vault = synchedVault.getVault();

    std::vector<std::string> txs = vault->getSerializedUnsignedTxs(params[0].get_str());

    Object result;
    result.push_back(Pair("serializedtxs", Array(txs.begin(), txs.end())));
    return result;
}

Value cmd_newtx(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() < 3)
        throw CommandInvalidParametersException();

    Vault* vault = synchedVault.getVault();

    std::string account = params[0].get_str();

    // Get outputs
    size_t i = 1;
    txouts_t txouts;
    do
    {
        if (params[i].type() != str_type || params[i+1].type() != int_type)
            throw CommandInvalidParametersException();

        std::string address = params[i++].get_str();
        if (!CoinQ::Script::isValidAddress(address, getCoinParams().address_versions()))
            throw DataFormatInvalidAddressException();

        uint64_t value = params[i++].get_uint64();
        bytes_t txoutscript = CoinQ::Script::getTxOutScriptForAddress(address, getCoinParams().address_versions());
        std::shared_ptr<TxOut> txout(new TxOut(value, txoutscript));
        txouts.push_back(txout);
         
    } while (i < (params.size() - 1) && (params[i].type() == str_type));

    uint64_t fee = i < params.size() ? params[i++].get_uint64() : 0;
    uint32_t version = i < params.size() ? (uint32_t)params[i++].get_uint64() : 1;
    uint32_t locktime = i < params.size() ? (uint32_t)params[i++].get_uint64() : 0;

    std::shared_ptr<Tx> tx = vault->createTx(account, version, locktime, txouts, fee, 1, true);

/*
    Value txObj;
    if (!read_string(tx->toJson(true, true), txObj))
        throw InternalTxJsonInvalidException(); 
*/
    Object txObj = getTxObject(*tx, true, true); 

    return txObj;
}

Value cmd_createtx(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() < 5)
        throw CommandInvalidParametersException();

    Vault* vault = synchedVault.getVault();

    std::string username = params[0].get_str();
    std::string account = params[1].get_str();

    // Get outputs
    size_t i = 2;
    txouts_t txouts;
    do
    {
        if (params[i].type() != str_type || params[i+1].type() != str_type || params[i+2].type() != int_type)
            throw CommandInvalidParametersException();

	std::string sending_label = params[i++].get_str();
        std::string address = params[i++].get_str();
        if (!CoinQ::Script::isValidAddress(address, getCoinParams().address_versions()))
            throw DataFormatInvalidAddressException();

        uint64_t value = params[i++].get_uint64();
        bytes_t txoutscript = CoinQ::Script::getTxOutScriptForAddress(address, getCoinParams().address_versions());
        std::shared_ptr<TxOut> txout(new TxOut(value, txoutscript));
	txout->sending_label(sending_label);
        txouts.push_back(txout);
         
    } while (i < (params.size() - 1) && (params[i].type() == str_type));

    uint64_t fee = i < params.size() ? params[i++].get_uint64() : 0;
    uint32_t version = i < params.size() ? (uint32_t)params[i++].get_uint64() : 1;
    uint32_t locktime = i < params.size() ? (uint32_t)params[i++].get_uint64() : 0;

    std::shared_ptr<Tx> tx;
    try
    {
        tx = vault->createTx(username, account, version, locktime, txouts, fee, 1, true);
    }
    catch (const AccountInsufficientFundsException& e)
    {
        if (getSmtpTls().isSet())
        {
            try
            {
                LOGGER(trace) << "Sending insufficient funds email alert." << endl;
                getSmtpTls().setSubject("Insufficient funds");
                std::stringstream body;
                body << "An insufficient funds error has occured.\r\n\r\n"
                     << "username:  " << username << "\r\n"
                     << "account:   " << account << "\r\n"
                     << "requested: " << e.requested() << "\r\n"
                     << "available: " << e.available() << "\r\n"
                     << "txouts: " << "\r\n";
                for (auto& txout: txouts)
                {
                    body << "    label:   " << txout->sending_label() << "\r\n"
                         << "    address: " << CoinQ::Script::getAddressForTxOutScript(txout->script(), getCoinParams().address_versions()) << "\r\n"
                         << "    amount:  " << txout->value() << "\r\n\r\n";
                }
                getSmtpTls().setBody(body.str());
                getSmtpTls().send();
            }
            catch (const exception& e)
            {
                LOGGER(error) << "Error occured sending alert email: " << e.what() << endl;
            }
        }

        throw;
    }

/*
    Value txObj;
    if (!read_string(tx->toJson(true, true), txObj))
        throw InternalTxJsonInvalidException(); 
*/
    Object txObj = getTxObject(*tx, true, true);

    return txObj;
}

Value cmd_proposetx(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() < 5 || params.size() > 6 || params[0].type() != str_type || params[1].type() != str_type)
        throw CommandInvalidParametersException();

    string username = params[0].get_str();
    string account = params[1].get_str();

    // Get outputs
    size_t i = 2;
    txouts_t txouts;
    do
    {
        if (params[i].type() != str_type || params[i+1].type() != str_type || params[i+2].type() != int_type)
            throw CommandInvalidParametersException();

	string sending_label = params[i++].get_str();
        string address = params[i++].get_str();
        if (!CoinQ::Script::isValidAddress(address, getCoinParams().address_versions()))
            throw DataFormatInvalidAddressException();

        uint64_t value = params[i++].get_uint64();
        bytes_t txoutscript = CoinQ::Script::getTxOutScriptForAddress(address, getCoinParams().address_versions());
        shared_ptr<TxOut> txout(new TxOut(value, txoutscript));
	txout->sending_label(sending_label);
        txouts.push_back(txout);
         
    } while (i < (params.size() - 1) && (params[i].type() == str_type));

    uint64_t fee = i < params.size() ? params[i++].get_uint64() : DEFAULT_TX_FEE;

    shared_ptr<TxProposal> txProposal = make_shared<TxProposal>(username, account, txouts, fee);
    addTxProposal(txProposal);

    return getTxProposalObject(*txProposal);
}

Value cmd_listtxproposals(Server& server, websocketpp::connection_hdl hdl, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 0)
        throw CommandInvalidParametersException();

    txproposals_t txProposals = getTxProposals();
    vector<Object> txProposalObjs;
    for (auto& txProposal: txProposals)
    {
        txProposalObjs.push_back(getTxProposalObject(*txProposal));
    }

    Object result;
    result.push_back(Pair("txproposals", Array(txProposalObjs.begin(), txProposalObjs.end())));
    return result;
}

Value cmd_gettxproposal(Server& server, websocketpp::connection_hdl hdl, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 1 || params[0].type() != str_type)
        throw CommandInvalidParametersException();

    uchar_vector hash(params[0].get_str());
    shared_ptr<TxProposal> txProposal = getTxProposal(hash);

    return getTxProposalObject(*txProposal);
}

Value cmd_canceltxproposal(Server& server, websocketpp::connection_hdl hdl, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 1 || params[0].type() != str_type)
        throw CommandInvalidParametersException();

    uchar_vector hash(params[0].get_str());
    cancelTxProposal(hash);

    return Value("success");
}

Value cmd_submittxproposal(Server& server, websocketpp::connection_hdl hdl, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 1 || params[0].type() != str_type)
        throw CommandInvalidParametersException();

    shared_ptr<TxProposal> txProposal;
    shared_ptr<Tx> tx;
    uchar_vector hash(params[0].get_str());
    submitTxProposal(hash);

    return Value("success");
}

Value cmd_listtxsubmissions(Server& server, websocketpp::connection_hdl hdl, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 0)
        throw CommandInvalidParametersException();

    txproposals_t txProposals = getTxSubmissions();
    vector<Object> txProposalObjs;
    for (auto& txProposal: txProposals)
    {
        Object obj = getTxProposalObject(*txProposal);
        txProposalObjs.push_back(obj);
    }

    Object result;
    result.push_back(Pair("txsubmissions", Array(txProposalObjs.begin(), txProposalObjs.end())));
    return result;
}

Value cmd_approvetx(Server& server, websocketpp::connection_hdl hdl, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 1 || params[0].type() != str_type)
        throw CommandInvalidParametersException();

    Vault* vault = synchedVault.getVault();

    shared_ptr<TxProposal> txSubmission;
    shared_ptr<Tx> tx;
    uchar_vector hash(params[0].get_str());

    {
        lock_guard<mutex> lock(g_txSubmissionMutex);
        txSubmission = getTxSubmission(hash);
        tx = vault->createTx(txSubmission->account(), DEFAULT_TX_VERSION, DEFAULT_TX_LOCKTIME, txSubmission->txouts(), txSubmission->fee(), 1, true);
        approveTxSubmission(hash, tx->unsigned_hash());
    }

    return Value("success");
}

Value cmd_canceltx(Server& server, websocketpp::connection_hdl hdl, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 1 || params[0].type() != str_type)
        throw CommandInvalidParametersException();

    shared_ptr<TxProposal> txSubmission;
    shared_ptr<Tx> tx;
    uchar_vector hash(params[0].get_str());

    {
        lock_guard<mutex> lock(g_txSubmissionMutex);
        txSubmission = getTxSubmission(hash);
        cancelTxSubmission(hash);
        sendTxChannelEvent(server, txSubmission);
    }

    return Value("success");
}

Value cmd_rejecttx(Server& server, websocketpp::connection_hdl hdl, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 1 || params[0].type() != str_type)
        throw CommandInvalidParametersException();

    shared_ptr<TxProposal> txSubmission;
    shared_ptr<Tx> tx;
    uchar_vector hash(params[0].get_str());

    {
        lock_guard<mutex> lock(g_txSubmissionMutex);
        txSubmission = getTxSubmission(hash);
        rejectTxSubmission(hash);
        sendTxChannelEvent(server, txSubmission);
    }

    return Value("success");
}

Value cmd_listprocessedtxsubmissions(Server& server, websocketpp::connection_hdl hdl, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 0)
        throw CommandInvalidParametersException();

    txproposals_t txProposals = getProcessedTxSubmissions();
    vector<Object> txProposalObjs;
    for (auto& txProposal: txProposals)
    {
        txProposalObjs.push_back(getTxProposalObject(*txProposal));
    }

    Object result;
    result.push_back(Pair("processedtxsubmissions", Array(txProposalObjs.begin(), txProposalObjs.end())));
    return result;
}

Value cmd_newlabeledtx(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() < 5)
        throw CommandInvalidParametersException();

    Vault* vault = synchedVault.getVault();

    std::string account = params[0].get_str();

    // Get outputs
    size_t i = 1;
    txouts_t txouts;
    do
    {
        if (params[i].type() != str_type || params[i+1].type() != str_type || params[i+2].type() != str_type || params[i+3].type() != int_type)
            throw CommandInvalidParametersException();

	std::string username = params[i++].get_str();
	std::string sending_label = params[i++].get_str();
        std::string address = params[i++].get_str();
        if (!CoinQ::Script::isValidAddress(address, getCoinParams().address_versions()))
            throw DataFormatInvalidAddressException();

        uint64_t value = params[i++].get_uint64();
        bytes_t txoutscript = CoinQ::Script::getTxOutScriptForAddress(address, getCoinParams().address_versions());
        std::shared_ptr<TxOut> txout(new TxOut(value, txoutscript));
	txout->sending_label(sending_label);
        txouts.push_back(txout);
         
    } while (i < (params.size() - 1) && (params[i].type() == str_type));

    uint64_t fee = i < params.size() ? params[i++].get_uint64() : 0;
    uint32_t version = i < params.size() ? (uint32_t)params[i++].get_uint64() : DEFAULT_TX_VERSION;
    uint32_t locktime = i < params.size() ? (uint32_t)params[i++].get_uint64() : DEFAULT_TX_LOCKTIME;

    std::shared_ptr<Tx> tx;
    try
    {
        tx = vault->createTx(account, version, locktime, txouts, fee, 1, true);
    }
    catch (const AccountInsufficientFundsException& e)
    {
        if (getSmtpTls().isSet())
        {
            try
            {
                LOGGER(trace) << "Sending insufficient funds email alert." << endl;
                getSmtpTls().setSubject("Insufficient funds");
                std::stringstream body;
                body << "An insufficient funds error has occured.\r\n\r\n"
                     //<< "username:  N/A"\r\n"
                     << "account:   " << account << "\r\n"
                     << "requested: " << e.requested() << "\r\n"
                     << "available: " << e.available() << "\r\n"
                     << "txouts: " << "\r\n";
                for (auto& txout: txouts)
                {
                    body << "    label:   " << txout->sending_label() << "\r\n"
                         << "    address: " << CoinQ::Script::getAddressForTxOutScript(txout->script(), getCoinParams().address_versions()) << "\r\n"
                         << "    amount:  " << txout->value() << "\r\n\r\n";
                }
                getSmtpTls().setBody(body.str());
                getSmtpTls().send();
            }
            catch (const exception& e)
            {
                LOGGER(error) << "Error occured sending alert email: " << e.what() << endl;
            }
        }

        throw;
    }
/*
    Value txObj;
    if (!read_string(tx->toJson(true, true), txObj))
        throw InternalTxJsonInvalidException(); 
*/

    Object txObj = getTxObject(*tx, true, true);

    return txObj;
}

Value cmd_getsigningrequest(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 1)
        throw CommandInvalidParametersException();

    Vault* vault = synchedVault.getVault();

    SigningRequest req;
    if (params[0].type() == str_type)
    {
        req = vault->getSigningRequest(uchar_vector(params[0].get_str()), true);
    }
    else if (params[0].type() == int_type)
    {
        req = vault->getSigningRequest(params[0].get_uint64(), true);
    }
    else
    {
        throw CommandInvalidParametersException();
    }

    return getSigningRequestObject(req);
}

// TODO: Mutex to prevent multiple clients from simultaneously unlocking and locking keychains
Value cmd_signtx(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 3 || params[1].type() != str_type || params[2].type() != str_type)
        throw CommandInvalidParametersException();

    Vault* vault = synchedVault.getVault();

    std::string keychain = params[1].get_str();
    std::vector<std::string> keychains;
    keychains.push_back(keychain);

    std::shared_ptr<Tx> tx;
    try
    {
        if (params[0].type() == str_type)
        {
            vault->unlockKeychain(keychain, secure_bytes_t());
            tx = vault->signTx(uchar_vector(params[0].get_str()), keychains, true);
        }
        else if (params[0].type() == int_type)
        {
            vault->unlockKeychain(keychain, secure_bytes_t());
            tx = vault->signTx(params[0].get_uint64(), keychains, true);
        }
        else
        {
            throw CommandInvalidParametersException();
        }
    }
    catch (const std::runtime_error& e)
    {
        vault->lockAllKeychains();
        throw e;
    }

    vault->lockAllKeychains();
    return getSigningRequestObject(vault->getSigningRequest(tx->id(), true));
}

Value cmd_insertrawtx(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() < 1 || params.size() > 2 || params[0].type() != str_type)
        throw CommandInvalidParametersException();

    bool trySend = false;
    if (params.size() > 1)
    {
        if (params[1].type() != bool_type)
            throw CommandInvalidParametersException();

        trySend = params[1].get_bool();
    }

    Vault* vault = synchedVault.getVault();

    uchar_vector rawtx(params[0].get_str());
    std::shared_ptr<Tx> tx(new Tx());
    tx->set(rawtx, time(NULL), Tx::UNSENT);
    tx = vault->insertTx(tx);
    if (!tx) throw OperationTransactionNotInsertedException();

    if (trySend && tx->status() == Tx::UNSENT)
    {
        tx = synchedVault.sendTx(tx->id());
    }

/*
    Value txObj;
    if (!read_string(tx->toJson(true, true), txObj))
        throw InternalTxJsonInvalidException();
*/

    Object txObj = getTxObject(*tx, true, true);

    return txObj;
}

Value cmd_insertserializedtx(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() < 1 || params.size() > 2 || params[0].type() != str_type)
        throw CommandInvalidParametersException();

    bool trySend = false;
    if (params.size() > 1)
    {
        if (params[1].type() != bool_type)
            throw CommandInvalidParametersException();

        trySend = params[1].get_bool();
    }

    Vault* vault = synchedVault.getVault();

    std::shared_ptr<Tx> tx = vault->importTxFromString(params[0].get_str());
    if (!tx) throw OperationTransactionNotInsertedException();

    if (trySend && tx->status() == Tx::UNSENT)
    {
        tx = synchedVault.sendTx(tx->id());
    }

/*
    Value txObj;
    if (!read_string(tx->toJson(true, true), txObj))
        throw InternalTxJsonInvalidException();

*/

    Object txObj = getTxObject(*tx, true, true);

    return txObj;
}

Value cmd_sendtx(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 1)
        throw CommandInvalidParametersException();

    std::shared_ptr<Tx> tx;
    if (params[0].type() == str_type)
    {
        uchar_vector hash(params[0].get_str());
        tx = synchedVault.sendTx(hash); 
    }
    else if (params[0].type() == int_type)
    {
        tx = synchedVault.sendTx((unsigned long)params[0].get_uint64());
    }
    else
    {
        throw CommandInvalidParametersException();
    }

    Object result;
    result.push_back(Pair("hash", uchar_vector(tx->hash()).getHex()));
    result.push_back(Pair("rawtx", uchar_vector(tx->raw()).getHex()));
    return result;
}

Value cmd_deletetx(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 1)
        throw CommandInvalidParametersException();

    Vault* vault = synchedVault.getVault();

    std::shared_ptr<Tx> tx;
    if (params[0].type() == str_type)
    {
        uchar_vector hash(params[0].get_str());
        tx = vault->getTx(hash);
        if (tx->status() >= Tx::PROPAGATED) throw OperationTransactionNotDeletedException();

        vault->deleteTx(hash); 
    }
    else if (params[0].type() == int_type)
    {
        tx = vault->getTx((unsigned long)params[0].get_uint64());
        if (tx->status() >= Tx::PROPAGATED) throw OperationTransactionNotDeletedException();

        vault->deleteTx((unsigned long)params[0].get_uint64());
    }
    else
    {
        throw CommandInvalidParametersException();
    }

    return Value("success");
}

// Blockchain operations
Value cmd_getblockheader(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 1)
        throw CommandInvalidParametersException();

    Vault* vault = synchedVault.getVault();

    std::shared_ptr<BlockHeader> header;
    if (params[0].type() == str_type)
    {
        uchar_vector hash(params[0].get_str());
        header = vault->getBlockHeader(hash);
    }
    else if (params[0].type() == int_type)
    {
        uint32_t height = (uint32_t)params[0].get_uint64();
        header = vault->getBlockHeader(height);
    }
    else
    {
        throw CommandInvalidParametersException();
    }

    return getBlockHeaderObject(*header);
}

Value cmd_getchaintip(Server& /*server*/, websocketpp::connection_hdl /*hdl*/, SynchedVault& synchedVault, const Array& params)
{
    if (params.size() > 0)
        throw CommandInvalidParametersException();

    Vault* vault = synchedVault.getVault();
//    uint32_t height = vault->getBestHeight();
//    std::shared_ptr<BlockHeader> header = vault->getBlockHeader(height);
    std::shared_ptr<BlockHeader> header = vault->getBestBlockHeader();
    if (!header) throw BlockHeaderNotFoundException();
    return getBlockHeaderObject(*header);
}

// User operations
Value cmd_adduser(WebSocket::Server& /*server*/, websocketpp::connection_hdl /*hdl*/, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params)
{
    if (params.size() < 1 || params[0].type() != str_type || (params.size() > 1 && params[1].type() != bool_type) || params.size() > 2)
        throw CommandInvalidParametersException();

    std::string username = params[0].get_str();
    bool enableTxOutScriptWhitelist = (params.size() > 1 && params[1].get_bool());

    Vault* vault = synchedVault.getVault();
    std::shared_ptr<User> user = vault->addUser(username, enableTxOutScriptWhitelist);
    return getUserObject(*user);
}

Value cmd_getuser(WebSocket::Server& /*server*/, websocketpp::connection_hdl /*hdl*/, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params)
{
    if (params.size() != 1 || params[0].type() != str_type)
        throw CommandInvalidParametersException();

    std::string username = params[0].get_str();

    Vault* vault = synchedVault.getVault();
    std::shared_ptr<User> user = vault->getUser(username);
    return getUserObject(*user);
}

Value cmd_addaddresstowhitelist(WebSocket::Server& /*server*/, websocketpp::connection_hdl /*hdl*/, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params)
{
    if (params.size() != 2 || params[0].type() != str_type || params[1].type() != str_type)
        throw CommandInvalidParametersException();

    std::string username = params[0].get_str();
    std::string address = params[1].get_str();

    if (!CoinQ::Script::isValidAddress(address, getCoinParams().address_versions()))
        throw DataFormatInvalidAddressException();

    bytes_t txoutscript = CoinQ::Script::getTxOutScriptForAddress(address, getCoinParams().address_versions());

    Vault* vault = synchedVault.getVault();
    std::shared_ptr<User> user = vault->addTxOutScriptToWhitelist(username, txoutscript);
    return getUserObject(*user);
}

Value cmd_removeaddressfromwhitelist(WebSocket::Server& /*server*/, websocketpp::connection_hdl /*hdl*/, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params)
{
    if (params.size() != 2 || params[0].type() != str_type || params[1].type() != str_type)
        throw CommandInvalidParametersException();

    std::string username = params[0].get_str();
    std::string address = params[1].get_str();

    if (!CoinQ::Script::isValidAddress(address, getCoinParams().address_versions()))
        throw DataFormatInvalidAddressException();

    bytes_t txoutscript = CoinQ::Script::getTxOutScriptForAddress(address, getCoinParams().address_versions());

    Vault* vault = synchedVault.getVault();
    std::shared_ptr<User> user = vault->removeTxOutScriptFromWhitelist(username, txoutscript);
    return getUserObject(*user);
}

Value cmd_clearaddresswhitelist(WebSocket::Server& /*server*/, websocketpp::connection_hdl /*hdl*/, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params)
{
    if (params.size() != 1 || params[0].type() != str_type)
        throw CommandInvalidParametersException();

    std::string username = params[0].get_str();

    Vault* vault = synchedVault.getVault();
    std::shared_ptr<User> user = vault->clearTxOutScriptWhitelist(username);
    return getUserObject(*user);
}

// Testing operations
Value cmd_fakemerkleblock(WebSocket::Server& /*server*/, websocketpp::connection_hdl /*hdl*/, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params)
{
    if (params.size() > 1 || (params.size() > 0 && params[0].type() != int_type))
        throw CommandInvalidParametersException();

    int nExtraLeaves = params.size() > 0 ? params[0].get_int() : 0;
    if (nExtraLeaves < 0) throw CommandInvalidParametersException();

    synchedVault.insertFakeMerkleBlock((unsigned int)nExtraLeaves);
    return Value("success");
}

Value cmd_faketx(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params)
{
    if (params.size() != 2 || params[0].type() != str_type || params[1].type() != int_type)
        throw CommandInvalidParametersException();

    std::string address = params[0].get_str();
        if (!CoinQ::Script::isValidAddress(address, getCoinParams().address_versions()))
            throw DataFormatInvalidAddressException();

    Vault* vault = synchedVault.getVault();
    uint64_t value = params[1].get_uint64();
    bytes_t txoutscript = CoinQ::Script::getTxOutScriptForAddress(address, getCoinParams().address_versions());
    std::shared_ptr<TxOut> txout = std::make_shared<TxOut>(value, txoutscript);
    std::shared_ptr<TxIn> txin = std::make_shared<TxIn>(bytes_t(32, 0), 0, bytes_t(), 0xffffffff);

    try
    {
        std::shared_ptr<SigningScript> script = vault->getSigningScript(txoutscript);
        txout->signingscript(script);
    }
    catch (const SigningScriptNotFoundException& /*e*/)
    {
    }

    txouts_t txouts;
    txouts.push_back(txout);

    txins_t txins;
    txins.push_back(txin);

    std::shared_ptr<Tx> tx = std::make_shared<Tx>();
    tx->set(1, txins, txouts, 0, time(NULL), Tx::UNSIGNED);

    sendTxChannelEvent(INSERTED, server, synchedVault, tx, true);
    return Value("success");
}

Value cmd_forcestatus(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params)
{
    if (params.size() != 0)
        throw CommandInvalidParametersException();

    sendStatusEvent(server, synchedVault);
    return Value("success");
}

void initCommandMap(command_map_t& command_map)
{
    command_map.clear();

    // Channel operations
    command_map.insert(cmd_pair("subscribe", Command(&cmd_subscribe)));
    command_map.insert(cmd_pair("unsubscribe", Command(&cmd_unsubscribe)));
    command_map.insert(cmd_pair("getchannels", Command(&cmd_getchannels)));

    // Global operations
    command_map.insert(cmd_pair("getstatus", Command(&cmd_getstatus)));
    //command_map.insert(cmd_pair("setvaultfromfile", Command(&cmd_setvaultfromfile)));
    //command_map.insert(cmd_pair("exportvaulttofile", Command(&cmd_exportvaulttofile)));

    // Keychain operations
    command_map.insert(cmd_pair("newkeychain", Command(&cmd_newkeychain)));
    command_map.insert(cmd_pair("renamekeychain", Command(&cmd_renamekeychain)));
    command_map.insert(cmd_pair("getkeychaininfo", Command(&cmd_getkeychaininfo)));
    command_map.insert(cmd_pair("getkeychains", Command(&cmd_getkeychains)));
    command_map.insert(cmd_pair("exportbip32", Command(&cmd_exportbip32)));
    command_map.insert(cmd_pair("importbip32", Command(&cmd_importbip32)));

    // Account operations
    command_map.insert(cmd_pair("newaccount", Command(&cmd_newaccount)));
    command_map.insert(cmd_pair("renameaccount", Command(&cmd_renameaccount)));
    command_map.insert(cmd_pair("getaccountinfo", Command(&cmd_getaccountinfo)));
    command_map.insert(cmd_pair("getaccounts", Command(&cmd_getaccounts)));
    command_map.insert(cmd_pair("issuescript", Command(&cmd_issuescript)));
    command_map.insert(cmd_pair("issuecontactscript", Command(&cmd_issuecontactscript)));
    command_map.insert(cmd_pair("importaccountfromfile", Command(&cmd_importaccountfromfile)));
    //command_map.insert(cmd_pair("exportaccounttofile", Command(&cmd_exportaccounttofile)));

    // Tx operations
    command_map.insert(cmd_pair("synctxs", Command(&cmd_synctxs)));
    command_map.insert(cmd_pair("gethistory", Command(&cmd_gethistory)));
    command_map.insert(cmd_pair("getunsigned", Command(&cmd_getunsigned)));
    command_map.insert(cmd_pair("gettx", Command(&cmd_gettx)));
    command_map.insert(cmd_pair("getserializedtx", Command(&cmd_getserializedtx)));
    command_map.insert(cmd_pair("getserializedunsignedtxs", Command(&cmd_getserializedunsignedtxs)));
    command_map.insert(cmd_pair("proposetx", Command(&cmd_proposetx)));
    command_map.insert(cmd_pair("gettxproposal", Command(&cmd_gettxproposal)));
    command_map.insert(cmd_pair("listtxproposals", Command(&cmd_listtxproposals)));
    command_map.insert(cmd_pair("canceltxproposal", Command(&cmd_canceltxproposal)));
    command_map.insert(cmd_pair("submittxproposal", Command(&cmd_submittxproposal)));
    command_map.insert(cmd_pair("listtxsubmissions", Command(&cmd_listtxsubmissions)));
    command_map.insert(cmd_pair("approvetx", Command(&cmd_approvetx)));
    command_map.insert(cmd_pair("canceltx", Command(&cmd_canceltx)));
    command_map.insert(cmd_pair("rejecttx", Command(&cmd_rejecttx)));
    command_map.insert(cmd_pair("listprocessedtxsubmissions", Command(&cmd_listprocessedtxsubmissions)));
    command_map.insert(cmd_pair("newtx", Command(&cmd_newtx)));
    command_map.insert(cmd_pair("createtx", Command(&cmd_createtx)));
    command_map.insert(cmd_pair("newlabeledtx", Command(&cmd_newlabeledtx)));
    command_map.insert(cmd_pair("getsigningrequest", Command(&cmd_getsigningrequest)));
    command_map.insert(cmd_pair("signtx", Command(&cmd_signtx)));
    command_map.insert(cmd_pair("insertrawtx", Command(&cmd_insertrawtx)));
    command_map.insert(cmd_pair("insertserializedtx", Command(&cmd_insertserializedtx)));
    command_map.insert(cmd_pair("sendtx", Command(&cmd_sendtx)));
    command_map.insert(cmd_pair("deletetx", Command(&cmd_deletetx)));

    // Blockchain operations
    command_map.insert(cmd_pair("getblockheader", Command(&cmd_getblockheader)));
    command_map.insert(cmd_pair("getchaintip", Command(&cmd_getchaintip)));

    // User operations
    command_map.insert(cmd_pair("adduser", Command(&cmd_adduser)));
    command_map.insert(cmd_pair("getuser", Command(&cmd_getuser)));
    command_map.insert(cmd_pair("addaddresstowhitelist", Command(&cmd_addaddresstowhitelist)));
    command_map.insert(cmd_pair("removeaddressfromwhitelist", Command(&cmd_removeaddressfromwhitelist)));
    command_map.insert(cmd_pair("clearaddresswhitelist", Command(&cmd_clearaddresswhitelist)));

    // Test operations
    //command_map.insert(cmd_pair("fakemerkleblock", Command(&cmd_fakemerkleblock)));
    command_map.insert(cmd_pair("faketx", Command(&cmd_faketx)));
    command_map.insert(cmd_pair("forcestatus", Command(&cmd_forcestatus)));
}
