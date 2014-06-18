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

#include <CoinDB/Vault.h>

#include <map>
#include <functional>

typedef std::function<json_spirit::Value(CoinDB::Vault*, const json_spirit::Array&)> cmd_t;

class Command
{
public:
    Command(cmd_t cmd) : m_cmd(cmd) { }

    json_spirit::Value operator()(CoinDB::Vault* vault, const json_spirit::Array& params) const
    {
        return m_cmd(vault, params);
    }

private:
    cmd_t m_cmd;
};

typedef std::pair<std::string, Command> cmd_pair;
typedef std::map<std::string, Command>  command_map_t;

json_spirit::Value cmd_status(CoinDB::Vault* vault, const json_spirit::Array& params);
json_spirit::Value cmd_newkeychain(CoinDB::Vault* vault, const json_spirit::Array& params);
json_spirit::Value cmd_renamekeychain(CoinDB::Vault* vault, const json_spirit::Array& params);
json_spirit::Value cmd_keychaininfo(CoinDB::Vault* vault, const json_spirit::Array& params);
json_spirit::Value cmd_keychains(CoinDB::Vault* vault, const json_spirit::Array& params);
json_spirit::Value cmd_exportbip32(CoinDB::Vault* vault, const json_spirit::Array& params);
json_spirit::Value cmd_importbip32(CoinDB::Vault* vault, const json_spirit::Array& params);
json_spirit::Value cmd_newaccount(CoinDB::Vault* vault, const json_spirit::Array& params);
json_spirit::Value cmd_renameaccount(CoinDB::Vault* vault, const json_spirit::Array& params);
json_spirit::Value cmd_accountinfo(CoinDB::Vault* vault, const json_spirit::Array& params);
json_spirit::Value cmd_listaccounts(CoinDB::Vault* vault, const json_spirit::Array& params);
json_spirit::Value cmd_issuescript(CoinDB::Vault* vault, const json_spirit::Array& params);
json_spirit::Value cmd_txs(CoinDB::Vault* vault, const json_spirit::Array& params);
json_spirit::Value cmd_gettx(CoinDB::Vault* vault, const json_spirit::Array& params);
json_spirit::Value cmd_blockheader(CoinDB::Vault* vault, const json_spirit::Array& params);
json_spirit::Value cmd_bestblockheader(CoinDB::Vault* vault, const json_spirit::Array& params);
json_spirit::Value cmd_newtx(CoinDB::Vault* vault, const json_spirit::Array& params);

void initCommandMap(command_map_t& command_map);
