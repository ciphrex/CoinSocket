///////////////////////////////////////////////////////////////////////////////
//
// CoinSocket
//
// commands.h
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
//

#pragma once

#include "jsonobjects.h"

#include <CoinDB/SynchedVault.h>

#include <map>
#include <functional>

typedef std::function<json_spirit::Value(CoinDB::SynchedVault&, const json_spirit::Array&)> cmd_t;

class Command
{
public:
    Command(cmd_t cmd) : m_cmd(cmd) { }

    json_spirit::Value operator()(CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params) const
    {
        return m_cmd(synchedVault, params);
    }

private:
    cmd_t m_cmd;
};

typedef std::pair<std::string, Command> cmd_pair;
typedef std::map<std::string, Command>  command_map_t;

void setDocumentDir(const std::string& documentDir);
const std::string& getDocumentDir();

// Global operations
json_spirit::Value cmd_getvaultinfo(CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_setvaultfromfile(CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_exportvaulttofile(CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);

// Keychain operations
json_spirit::Value cmd_newkeychain(CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_renamekeychain(CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_getkeychaininfo(CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_getkeychains(CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_exportbip32(CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_importbip32(CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);

// Account operations
json_spirit::Value cmd_newaccount(CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_renameaccount(CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_getaccountinfo(CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_getaccounts(CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_issuescript(CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_importaccountfromfile(CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_exportaccounttofile(CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);

// Tx operations
json_spirit::Value cmd_gethistory(CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_gettx(CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_newtx(CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_getsigningrequest(CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_signtx(CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_insertrawtx(CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_sendtx(CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);


// Blockchain operations
json_spirit::Value cmd_getblockheader(CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_getchaintip(CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);

void initCommandMap(command_map_t& command_map);
