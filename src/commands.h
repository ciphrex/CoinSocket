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

#include <CoinQ/CoinQ_coinparams.h>

#include "smtp.h"

#include <string>
#include <map>
#include <functional>

#include <json_spirit/json_spirit_value.h>
#include <WebSocketAPI/Server.h>

namespace CoinDB { class SynchedVault; }

typedef std::function<json_spirit::Value(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault&, const json_spirit::Array&)> cmd_t;

class Command
{
public:
    Command(cmd_t cmd) : m_cmd(cmd) { }

    json_spirit::Value operator()(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params) const
    {
        return m_cmd(server, hdl, synchedVault, params);
    }

private:
    cmd_t m_cmd;
};

typedef std::pair<std::string, Command> cmd_pair;
typedef std::map<std::string, Command>  command_map_t;

void setDocumentDir(const std::string& documentDir);
const std::string& getDocumentDir();

void setCoinParams(const CoinQ::CoinParams& coinParams);
const CoinQ::CoinParams& getCoinParams();

void setSmtpTls(const std::string& username, const std::string& password, const std::string& url);
SmtpTls& getSmtpTls();

// Channel operations
json_spirit::Value cmd_subscribe(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_unsubscribe(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_getchannels(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);

// Global operations
json_spirit::Value cmd_getstatus(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_setvaultfromfile(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_exportvaulttofile(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);

// Keychain operations
json_spirit::Value cmd_newkeychain(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_renamekeychain(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_getkeychaininfo(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_getkeychains(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_exportbip32(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_importbip32(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);

// Account operations
json_spirit::Value cmd_newaccount(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_renameaccount(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_getaccountinfo(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_getaccounts(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_issuescript(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_issuecontactscript(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_importaccountfromfile(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_exportaccounttofile(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);

// Tx operations
json_spirit::Value cmd_gethistory(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_gettx(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_getserializedtx(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_getserializedunsignedtxs(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_newtx(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_createtx(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_newlabeledtx(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_getsigningrequest(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_signtx(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_insertrawtx(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_insertserializedtx(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_sendtx(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_deletetx(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);

// Blockchain operations
json_spirit::Value cmd_getblockheader(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_getchaintip(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);

// User operations
json_spirit::Value cmd_adduser(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_getuser(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_addaddresstowhitelist(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_removeaddressfromwhitelist(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_clearaddresswhitelist(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);

// Testing operations
json_spirit::Value cmd_fakemerkleblock(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);

// Bitcoin Core compatibility methods
json_spirit::Value cmd_getaccount(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_getaccountaddress(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_getaddressesbyaccount(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_getbalance(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_getbestblockhash(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_getblockcount(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_getblockhash(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_getnewaddress(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_getrawtransaction(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_getreceivedbyaccount(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_getreceivedbyaddress(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_gettransaction(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_gettxout(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_listaccounts(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_listreceivedbyaccount(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_listreceivedbyaddress(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_listsinceblock(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_listtransactions(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_listunspent(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_sendfrom(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_sendmany(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_sendrawtransaction(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_sendtoaddress(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);
json_spirit::Value cmd_validateaddress(WebSocket::Server& server, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, const json_spirit::Array& params);

void initCommandMap(command_map_t& command_map);
