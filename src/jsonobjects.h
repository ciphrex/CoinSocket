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
    class SynchedVault;
    class BlockHeader;
    class Keychain;
    class User;
    class AccountInfo;
    class TxView;
    class SigningRequest;
}

json_spirit::Object getSyncStatusObject(const CoinDB::SynchedVault& synchedVault);
json_spirit::Object getBlockHeaderObject(CoinDB::BlockHeader* header);
json_spirit::Object getKeychainObject(CoinDB::Keychain* keychain);
json_spirit::Object getUserObject(CoinDB::User* user, const unsigned char base58Versions[]);
json_spirit::Object getAccountInfoObject(const CoinDB::AccountInfo& accountInfo);
json_spirit::Object getTxViewObject(const CoinDB::TxView& txview);
json_spirit::Object getSigningRequestObject(const CoinDB::SigningRequest& req);
