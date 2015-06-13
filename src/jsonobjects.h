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

#include <json_spirit/json_spirit_reader_template.h>
#include <json_spirit/json_spirit_writer_template.h>
#include <json_spirit/json_spirit_utils.h>

namespace CoinDB
{
    class TxIn;
    class TxOut;
    class Tx;
    class SynchedVault;
    class BlockHeader;
    class Keychain;
    class User;
    class AccountInfo;
    class TxView;
    class SigningRequest;
}

namespace CoinSocket
{

class TxProposal;

json_spirit::Object getSyncStatusObject(const CoinDB::SynchedVault& synchedVault);
json_spirit::Object getBlockHeaderObject(const CoinDB::BlockHeader& header);
json_spirit::Object getKeychainObject(const CoinDB::Keychain& keychain);
json_spirit::Object getUserObject(const CoinDB::User& user);
json_spirit::Object getAccountInfoObject(const CoinDB::AccountInfo& accountInfo);
json_spirit::Object getTxViewObject(const CoinDB::TxView& txview);
json_spirit::Object getSigningRequestObject(const CoinDB::SigningRequest& req);
json_spirit::Object getTxProposalObject(const CoinSocket::TxProposal& txProposal);
json_spirit::Object getTxInObject(const CoinDB::TxIn& txin);
json_spirit::Object getTxOutObject(const CoinDB::TxOut& txout);
json_spirit::Object getTxObject(const CoinDB::Tx& tx, bool includeRaw = false, bool includeSerialized = false);

}
