///////////////////////////////////////////////////////////////////////////////
//
// CoinSocket
//
// jsonobjects.cpp
//
// Copyright (c) 2014-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "jsonobjects.h"
#include "coinparams.h"
#include "txproposal.h"

#include <CoinQ/CoinQ_script.h>
#include <CoinDB/Schema.h>
#include <CoinDB/SigningRequest.h>
#include <CoinDB/SynchedVault.h>
#include <stdutils/uchar_vector.h>

using namespace CoinSocket;
using namespace CoinDB;
using namespace json_spirit;

Object CoinSocket::getSyncStatusObject(const SynchedVault& synchedVault)
{
    Object result;
    result.push_back(Pair("status", SynchedVault::getStatusString(synchedVault.getStatus())));
    result.push_back(Pair("syncheight", (uint64_t)synchedVault.getSyncHeight()));
    result.push_back(Pair("synchash", uchar_vector(synchedVault.getSyncHash()).getHex()));
    result.push_back(Pair("bestheight", (uint64_t)synchedVault.getBestHeight()));
    result.push_back(Pair("besthash", uchar_vector(synchedVault.getBestHash()).getHex()));
    return result;
}

Object CoinSocket::getBlockHeaderObject(const BlockHeader& header)
{
    Object result;
    result.push_back(Pair("hash", uchar_vector(header.hash()).getHex()));
    result.push_back(Pair("height", (uint64_t)header.height()));
    result.push_back(Pair("version", (uint64_t)header.version()));
    result.push_back(Pair("prevhash", uchar_vector(header.prevhash()).getHex()));
    result.push_back(Pair("merkleroot", uchar_vector(header.merkleroot()).getHex()));
    result.push_back(Pair("timestamp", (uint64_t)header.timestamp()));
    result.push_back(Pair("bits", (uint64_t)header.bits()));
    result.push_back(Pair("nonce", (uint64_t)header.nonce()));
    return result;
}

Object CoinSocket::getKeychainObject(const Keychain& keychain)
{
    Object result;
    result.push_back(Pair("id", (uint64_t)keychain.id()));
    result.push_back(Pair("name", keychain.name()));
    result.push_back(Pair("depth", (int)keychain.depth()));
    result.push_back(Pair("parentfp", (uint64_t)keychain.parent_fp()));
    result.push_back(Pair("childnum", (uint64_t)keychain.child_num()));
    result.push_back(Pair("pubkey", uchar_vector(keychain.pubkey()).getHex()));
    result.push_back(Pair("hash", uchar_vector(keychain.hash()).getHex()));
    return result;
}

Object CoinSocket::getUserObject(const CoinDB::User& user)
{
    Object result;
    result.push_back(Pair("id", (uint64_t)user.id()));
    result.push_back(Pair("username", user.username()));
    result.push_back(Pair("txoutscript_whitelist_enabled", user.isTxOutScriptWhitelistEnabled()));

    std::set<bytes_t> scripts = user.txoutscript_whitelist();
    std::vector<std::string> addresses;
    for (auto& script: scripts)
    {
        addresses.push_back(CoinQ::Script::getAddressForTxOutScript(script, getCoinParams().address_versions()));
    }
    result.push_back(Pair("addresses", Array(addresses.begin(), addresses.end())));
    return result;
}

Object CoinSocket::getAccountInfoObject(const AccountInfo& accountInfo)
{
    Object result;
    result.push_back(Pair("id", (uint64_t)accountInfo.id()));
    result.push_back(Pair("name", accountInfo.name()));
    result.push_back(Pair("minsigs", (int)accountInfo.minsigs()));
    result.push_back(Pair("keychains", Array(accountInfo.keychain_names().begin(), accountInfo.keychain_names().end())));
    result.push_back(Pair("unusedpoolsize", (uint64_t)accountInfo.unused_pool_size()));
    result.push_back(Pair("timecreated", (uint64_t)accountInfo.time_created()));
    result.push_back(Pair("bins", Array(accountInfo.bin_names().begin(), accountInfo.bin_names().end())));
    return result;
}

Object CoinSocket::getTxViewObject(const TxView& txview)
{
    bytes_t hash = txview.status == CoinDB::Tx::UNSIGNED
        ? txview.unsigned_hash : txview.hash;

    Object result;
    result.push_back(Pair("id", (uint64_t)txview.id));
    result.push_back(Pair("hash", uchar_vector(hash).getHex()));
    result.push_back(Pair("status", CoinDB::Tx::getStatusString(txview.status, true)));
    result.push_back(Pair("version", (uint64_t)txview.version));
    result.push_back(Pair("locktime", (uint64_t)txview.locktime));
    result.push_back(Pair("timestamp", (uint64_t)txview.timestamp));
    result.push_back(Pair("height", (uint64_t)txview.height));
    return result;
}

Object CoinSocket::getSigningRequestObject(const SigningRequest& req)
{
    std::vector<std::string> keychain_names;
    std::vector<std::string> keychain_hashes;
    for (auto& keychain_pair: req.keychain_info())
    {
        keychain_names.push_back(keychain_pair.first);
        keychain_hashes.push_back(uchar_vector(keychain_pair.second).getHex());
    }
    std::string hash = uchar_vector(req.hash()).getHex();
    std::string rawtx = uchar_vector(req.rawtx()).getHex();

    Object result;
    result.push_back(Pair("hash", hash));
    result.push_back(Pair("sigsneeded", (uint64_t)req.sigs_needed()));
    result.push_back(Pair("keychains", Array(keychain_names.begin(), keychain_names.end())));
    result.push_back(Pair("keychainhashes", Array(keychain_hashes.begin(), keychain_hashes.end())));
    result.push_back(Pair("rawtx", rawtx));
    return result;
}

Object CoinSocket::getTxProposalObject(const CoinSocket::TxProposal& txProposal)
{
    std::string statusString;
    switch (txProposal.status())
    {
        case TxProposal::PENDING:
            statusString = "PENDING";
            break;
        case TxProposal::APPROVED:
            statusString = "APPROVED";
            break;
        case TxProposal::CANCELED:
            statusString = "CANCELED";
            break;
        case TxProposal::REJECTED:
            statusString = "REJECTED";
            break;
        default:
            statusString = "UNKNOWN";
    }

    Object result;
    result.push_back(Pair("proposalid", uchar_vector(txProposal.hash()).getHex()));
    result.push_back(Pair("status", statusString));
    result.push_back(Pair("username", txProposal.username()));
    result.push_back(Pair("account", txProposal.account()));

    std::vector<Object> txoutObjs;
    for (auto& txout: txProposal.txouts())  { txoutObjs.push_back(getTxOutObject(*txout)); }

    result.push_back(Pair("txouts", Array(txoutObjs.begin(), txoutObjs.end())));
    result.push_back(Pair("fee", txProposal.fee()));
    result.push_back(Pair("timestamp", txProposal.timestamp()));
    return result;
}

Object CoinSocket::getTxInObject(const CoinDB::TxIn& txin)
{
    Object result;
    result.push_back(Pair("outhash", uchar_vector(txin.outhash()).getHex()));
    result.push_back(Pair("outindex", (uint64_t)txin.outindex()));
    result.push_back(Pair("script", uchar_vector(txin.script()).getHex()));
    result.push_back(Pair("sequence", (uint64_t)txin.sequence()));
    return result;
}

Object CoinSocket::getTxOutObject(const CoinDB::TxOut& txout)
{
    Object result;
    result.push_back(Pair("address", CoinQ::Script::getAddressForTxOutScript(txout.script(), getCoinParams().address_versions())));
    result.push_back(Pair("value", (uint64_t)txout.value()));
    result.push_back(Pair("sending_label", txout.sending_label()));
    result.push_back(Pair("receiving_label", txout.receiving_label()));
    result.push_back(Pair("script", uchar_vector(txout.script()).getHex()));
    return result;
}

Object CoinSocket::getTxObject(const CoinDB::Tx& tx, bool includeRawHex, bool includeSerialized)
{
    Object result;
    result.push_back(Pair("version", (uint64_t)tx.version()));
    result.push_back(Pair("locktime", (uint64_t)tx.locktime()));
    result.push_back(Pair("hash", uchar_vector(tx.hash()).getHex()));
    result.push_back(Pair("unsignedhash", uchar_vector(tx.unsigned_hash()).getHex()));
    result.push_back(Pair("status", CoinDB::Tx::getStatusString(tx.status())));
    if (tx.blockheader())   { result.push_back(Pair("height", (uint64_t)tx.blockheader()->height())); }
    else                    { result.push_back(Pair("height", Value())); /* null */ }

    std::vector<Object> txinObjs;
    for (auto& txin: tx.txins())    { txinObjs.push_back(getTxInObject(*txin)); }
    result.push_back(Pair("txins", Array(txinObjs.begin(), txinObjs.end())));

    std::vector<Object> txoutObjs;
    for (auto& txout: tx.txouts())  { txoutObjs.push_back(getTxOutObject(*txout)); }
    result.push_back(Pair("txouts", Array(txoutObjs.begin(), txoutObjs.end())));

    if (includeRawHex)      { result.push_back(Pair("rawtx", uchar_vector(tx.raw()).getHex())); }
    if (includeSerialized)  { result.push_back(Pair("serializedtx", tx.toSerialized())); }

    return result;
} 
