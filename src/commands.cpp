///////////////////////////////////////////////////////////////////////////////
//
// CoinSocket
//
// commands.cpp
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
//

#include "commands.h"

#include <CoinQ/CoinQ_script.h>
#include <CoinCore/Base58Check.h>
#include <CoinCore/random.h>

#include "jsonobjects.h"

using namespace json_spirit;
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

// Global operations
Value cmd_getvaultinfo(SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 0)
        throw std::runtime_error("Invalid parameters.");

    Vault* vault = synchedVault.getVault();

    Object result;
    result.push_back(Pair("name", vault->getName()));
    result.push_back(Pair("schema", (uint64_t)vault->getSchemaVersion()));
    result.push_back(Pair("horizon", (uint64_t)vault->getHorizonTimestamp()));
    return result;
}

// Keychain operations
Value cmd_newkeychain(SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 1 || params[0].type() != str_type)
        throw std::runtime_error("Invalid parameters.");

    Vault* vault = synchedVault.getVault();

    std::string keychainName = params[0].get_str();
    vault->newKeychain(keychainName, random_bytes(32));
    return Value("success");
}

Value cmd_renamekeychain(SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 2 || params[0].type() != str_type || params[0].type() != str_type)
        throw std::runtime_error("Invalid parameters.");

    Vault* vault = synchedVault.getVault();

    std::string oldName = params[0].get_str();
    std::string newName = params[1].get_str();
    vault->renameKeychain(oldName, newName);
    return Value("success");
}

Value cmd_getkeychaininfo(SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 1 || params[0].type() != str_type)
        throw std::runtime_error("Invalid parameters.");

    Vault* vault = synchedVault.getVault();

    std::string keychainName = params[0].get_str();
    std::shared_ptr<Keychain> keychain = vault->getKeychain(keychainName);
    Object keychainInfo;
    keychainInfo.push_back(Pair("id", (uint64_t)keychain->id()));
    keychainInfo.push_back(Pair("name", keychain->name()));
    keychainInfo.push_back(Pair("depth", (int)keychain->depth()));
    keychainInfo.push_back(Pair("parentfp", (uint64_t)keychain->parent_fp()));
    keychainInfo.push_back(Pair("childnum", (uint64_t)keychain->child_num()));
    keychainInfo.push_back(Pair("pubkey", uchar_vector(keychain->pubkey()).getHex()));
    keychainInfo.push_back(Pair("hash", uchar_vector(keychain->hash()).getHex()));
    return keychainInfo;
}

Value cmd_getkeychains(SynchedVault& synchedVault, const Array& params)
{
    if (params.size() > 2)
        throw std::runtime_error("Invalid parameters.");

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

Value cmd_exportbip32(SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 1)
        throw std::runtime_error("Invalid parameters.");

    std::string keychainName = params[0].get_str();

    Vault* vault = synchedVault.getVault();

    vault->unlockChainCodes(secure_bytes_t()); // TODO: add a method to unlock chaincodes
    bytes_t extendedkey = vault->getKeychainExtendedKey(keychainName, false);

    Object result;
    result.push_back(Pair("extendedkey", toBase58Check(extendedkey)));
    return result;
}

Value cmd_importbip32(SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 2)
        throw std::runtime_error("Invalid parameters.");

    bytes_t extendedkey;
    if (!fromBase58Check(params[1].get_str(), extendedkey))
        throw std::runtime_error("Invalid extended key.");

    Vault* vault = synchedVault.getVault();

    vault->importKeychainExtendedKey(params[0].get_str(), extendedkey, false);
    return Value("success");
}

Value cmd_newaccount(SynchedVault& synchedVault, const Array& params)
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

    Vault* vault = synchedVault.getVault();

    vault->unlockChainCodes(secure_bytes_t()); // TODO: add a method to unlock chaincodes using passphrase
    vault->newAccount(accountName, minsigs, keychainNames);
    vault->lockChainCodes();
    return Value("success");
}

Value cmd_renameaccount(SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 2 || params[0].type() != str_type || params[1].type() != str_type)
        throw std::runtime_error("Invalid parameters.");

    Vault* vault = synchedVault.getVault();

    std::string oldName = params[0].get_str();
    std::string newName = params[1].get_str();
    vault->renameAccount(oldName, newName);
    return Value("success");
}

Value cmd_getaccountinfo(SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 1 || params[0].type() != str_type)
        throw std::runtime_error("Invalid parameters.");

    Vault* vault = synchedVault.getVault();

    std::string accountName = params[0].get_str();
    AccountInfo accountInfo = vault->getAccountInfo(accountName);
    uint64_t balance = vault->getAccountBalance(accountName, 0);
    uint64_t confirmedBalance = vault->getAccountBalance(accountName, 1);

    Object result;
    result.push_back(Pair("id", (uint64_t)accountInfo.id()));
    result.push_back(Pair("name", accountInfo.name()));
    result.push_back(Pair("minsigs", (int)accountInfo.minsigs()));
    result.push_back(Pair("keychains", Array(accountInfo.keychain_names().begin(), accountInfo.keychain_names().end())));
    result.push_back(Pair("unusedpoolsize", (uint64_t)accountInfo.unused_pool_size()));
    result.push_back(Pair("timecreated", (uint64_t)accountInfo.time_created()));
    result.push_back(Pair("bins", Array(accountInfo.bin_names().begin(), accountInfo.bin_names().end())));
    result.push_back(Pair("balance", balance));
    result.push_back(Pair("confirmedbalance", confirmedBalance));
    return result;
}

Value cmd_getaccounts(SynchedVault& synchedVault, const Array& params)
{
    if (params.size() > 0)
        throw std::runtime_error("Invalid parameters.");

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

Value cmd_issuescript(SynchedVault& synchedVault, const Array& params)
{
    if (params.size() < 1 || params.size() > 3)
        throw std::runtime_error("Invalid parameters.");

    Vault* vault = synchedVault.getVault();

    std::string accountName = params[0].get_str();
    std::string label;
    if (params.size() > 1) label = params[1].get_str();
    std::string binName;
    if (params.size() > 2) binName = params[2].get_str();
    if (binName.empty()) binName = DEFAULT_BIN_NAME;

    std::shared_ptr<SigningScript> script = vault->issueSigningScript(accountName, binName, label);
    synchedVault.updateBloomFilter();

    std::string address = getAddressFromScript(script->txoutscript(), BITCOIN_BASE58_VERSIONS);
    std::string uri = "bitcoin:";
    uri += address;
    if (!label.empty()) { uri += "?label="; uri += label; }

    Object result;
    result.push_back(Pair("account", accountName));
    result.push_back(Pair("label", label));
    result.push_back(Pair("accountbin", binName));
    result.push_back(Pair("script", uchar_vector(script->txoutscript()).getHex()));
    result.push_back(Pair("address", address));
    result.push_back(Pair("uri", uri)); 
    return result;
}

// Tx operations
Value cmd_gethistory(SynchedVault& synchedVault, const Array& params)
{
    if (params.size() > 1)
        throw std::runtime_error("Invalid parameters.");

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

Value cmd_gettx(SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 1)
        throw std::runtime_error("Invalid parameters.");

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
        throw std::runtime_error("Invalid parameters.");
    }

    Value txObj;
    if (!read_string(tx->toJson(), txObj))
        throw std::runtime_error("Internal error - invalid tx json.");

    Object result;
    result.push_back(Pair("tx", txObj));
    result.push_back(Pair("rawtx", uchar_vector(tx->raw()).getHex()));
    return result;
}

Value cmd_newtx(SynchedVault& synchedVault, const Array& params)
{
    if (params.size() < 3)
        throw std::runtime_error("Invalid parameters.");

    Vault* vault = synchedVault.getVault();

    std::string account = params[0].get_str();

    // Get outputs
    size_t i = 1;
    txouts_t txouts;
    do
    {
        if (params[i].type() != str_type || params[i+1].type() != int_type)
            throw std::runtime_error("Invalid parameters.");

        std::string address = params[i++].get_str();
        uint64_t value = params[i++].get_uint64();
        bytes_t txoutscript = CoinQ::Script::getTxOutScriptForAddress(address, BITCOIN_BASE58_VERSIONS);
        std::shared_ptr<TxOut> txout(new TxOut(value, txoutscript));
        txouts.push_back(txout);
         
    } while (i < (params.size() - 1) && (params[i].type() == str_type));

    uint64_t fee = i < params.size() ? params[i++].get_uint64() : 0;
    uint32_t version = i < params.size() ? (uint32_t)params[i++].get_uint64() : 1;
    uint32_t locktime = i < params.size() ? (uint32_t)params[i++].get_uint64() : 0;

    std::shared_ptr<Tx> tx = vault->createTx(account, version, locktime, txouts, fee, 1, true);

    Value txObj;
    if (!read_string(tx->toJson(), txObj))
        throw std::runtime_error("Internal error - invalid tx json.");

    Object result;
    result.push_back(Pair("tx", txObj));
    result.push_back(Pair("rawtx", uchar_vector(tx->raw()).getHex()));
    return result;
}

Value cmd_getsigningrequest(SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 1)
        throw std::runtime_error("Invalid parameters.");

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
        throw std::runtime_error("Invalid parameters.");
    }

    return getSigningRequestObject(req);
}

// TODO: Mutex to prevent multiple clients from simultaneously unlocking and locking keychains
Value cmd_signtx(SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 3 || params[1].type() != str_type || params[2].type() != str_type)
        throw std::runtime_error("Invalid parameters.");

    Vault* vault = synchedVault.getVault();

    std::string keychain = params[1].get_str();
    std::vector<std::string> keychains;
    keychains.push_back(keychain);

    std::shared_ptr<Tx> tx;
    try
    {
        if (params[0].type() == str_type)
        {
            vault->unlockChainCodes(uchar_vector("1234"));
            vault->unlockKeychain(keychain, secure_bytes_t());
            tx = vault->signTx(uchar_vector(params[0].get_str()), keychains, true);
        }
        else if (params[0].type() == int_type)
        {
            vault->unlockChainCodes(uchar_vector("1234"));
            vault->unlockKeychain(keychain, secure_bytes_t());
            tx = vault->signTx(params[0].get_uint64(), keychains, true);
        }
        else
        {
            throw std::runtime_error("Invalid parameters.");
        }
    }
    catch (const std::runtime_error& e)
    {
        vault->lockAllKeychains();
        vault->lockChainCodes();
        throw e;
    }

    vault->lockAllKeychains();
    vault->lockChainCodes();
    return getSigningRequestObject(vault->getSigningRequest(tx->id(), true));
}

Value cmd_insertrawtx(SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 1 || params[0].type() != str_type)
        throw std::runtime_error("Invalid parameters.");

    Vault* vault = synchedVault.getVault();

    uchar_vector rawtx(params[0].get_str());
    std::shared_ptr<Tx> tx(new Tx());
    tx->set(rawtx);
    tx = vault->insertTx(tx);
    if (!tx)
        throw std::runtime_error("Transaction not inserted.");

    Value txObj;
    if (!read_string(tx->toJson(), txObj))
        throw std::runtime_error("Internal error - invalid tx json.");

    Object result;
    result.push_back(Pair("tx", txObj));
    result.push_back(Pair("rawtx", uchar_vector(tx->raw()).getHex()));
    return result;
}

// Blockchain operations
Value cmd_getblockheader(SynchedVault& synchedVault, const Array& params)
{
    if (params.size() != 1)
        throw std::runtime_error("Invalid parameters.");

    Vault* vault = synchedVault.getVault();

    uint32_t height = (uint32_t)params[0].get_uint64();
    std::shared_ptr<BlockHeader> header = vault->getBlockHeader(height);
    return getBlockHeaderObject(header.get());
}

Value cmd_getchaintip(SynchedVault& synchedVault, const Array& params)
{
    if (params.size() > 0)
        throw std::runtime_error("Invalid parameters.");

    Vault* vault = synchedVault.getVault();
    uint32_t height = vault->getBestHeight();
    std::shared_ptr<BlockHeader> header = vault->getBlockHeader(height);
    return getBlockHeaderObject(header.get());
}


void initCommandMap(command_map_t& command_map)
{
    command_map.clear();

    // Global operations
    command_map.insert(cmd_pair("getvaultinfo", Command(&cmd_getvaultinfo)));

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

    // Tx operations
    command_map.insert(cmd_pair("gethistory", Command(&cmd_gethistory)));
    command_map.insert(cmd_pair("gettx", Command(&cmd_gettx)));
    command_map.insert(cmd_pair("newtx", Command(&cmd_newtx)));
    command_map.insert(cmd_pair("getsigningrequest", Command(&cmd_getsigningrequest)));
    command_map.insert(cmd_pair("signtx", Command(&cmd_signtx)));
    command_map.insert(cmd_pair("insertrawtx", Command(&cmd_insertrawtx)));

    // Blockchain operations
    command_map.insert(cmd_pair("getblockheader", Command(&cmd_getblockheader)));
    command_map.insert(cmd_pair("getchaintip", Command(&cmd_getchaintip)));
}
