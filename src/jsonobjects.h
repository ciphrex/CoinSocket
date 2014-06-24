///////////////////////////////////////////////////////////////////////////////
//
// CoinSocket
//
// jsonobjects.h
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
//

#pragma once

#include <CoinDB/Schema.h>
#include <CoinDB/SigningRequest.h>
#include <stdutils/uchar_vector.h>

#include <json_spirit/json_spirit_reader_template.h>
#include <json_spirit/json_spirit_writer_template.h>
#include <json_spirit/json_spirit_utils.h>

inline json_spirit::Object getBlockHeaderObject(CoinDB::BlockHeader* header)
{
    using namespace json_spirit;

    Object result;
    result.push_back(Pair("hash", uchar_vector(header->hash()).getHex()));
    result.push_back(Pair("height", (uint64_t)header->height()));
    result.push_back(Pair("version", (uint64_t)header->version()));
    result.push_back(Pair("prevhash", uchar_vector(header->prevhash()).getHex()));
    result.push_back(Pair("merkleroot", uchar_vector(header->merkleroot()).getHex()));
    result.push_back(Pair("timestamp", (uint64_t)header->timestamp()));
    result.push_back(Pair("bits", (uint64_t)header->bits()));
    result.push_back(Pair("nonce", (uint64_t)header->nonce()));
    return result;
}

inline json_spirit::Object getTxViewObject(const CoinDB::TxView& txview)
{
    using namespace json_spirit;

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

inline json_spirit::Object getSigningRequestObject(const CoinDB::SigningRequest& req)
{
    using namespace json_spirit;

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
