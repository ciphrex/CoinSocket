////////////////////////////////////////////////////////////////////////////////
//
// CoinSocketClient.h
//
// Copyright (c) 2014 Eric Lombrozo, all rights reserved
//

#pragma once

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <json_spirit/json_spirit_reader_template.h>
#include <json_spirit/json_spirit_writer_template.h>
#include <json_spirit/json_spirit_utils.h>

#include <iostream>
#include <sstream>
#include <map>

namespace CoinSocket
{

typedef websocketpp::client<websocketpp::config::asio_client> client_t;
typedef client_t::connection_ptr                              connection_ptr_t;
typedef websocketpp::config::asio_client::message_type::ptr   message_ptr_t;
typedef websocketpp::connection_hdl                           connection_hdl_t;
typedef websocketpp::lib::error_code                          error_code_t;

typedef std::function<void(const json_spirit::Object&)> callback_t;
typedef std::map<int64_t, callback_t> callback_map_t;
typedef std::map<std::string, callback_t> msg_handler_map_t;

typedef std::function<void(void)> socket_handler_t;
typedef std::function<void(const std::string&)> log_handler_t;

class CoinSocketClient
{
private:
    // WebSocket connection to rippled
    client_t            client;
    std::string         serverUrl;
    connection_ptr_t    pConnection;
    bool                bConnected;

    socket_handler_t    on_open;
    log_handler_t       on_log;

    msg_handler_map_t   msg_handler_map;

    int64_t             sequence;
    callback_map_t      callback_map;

protected:
    // Connection handlers
    void onOpen(connection_hdl_t hdl);
    void onClose(connection_hdl_t hdl);
    void onFail(connection_hdl_t hdl);
    void onMessage(connection_hdl_t, message_ptr_t msg);

    // Ripple message handlers
    void onResponse(const json_spirit::Object& obj);

public:
    // Constructor / Destructor
    CoinSocketClient();
    ~CoinSocketClient();

    // start() blocks until disconnection occurs.
    void start(const std::string& serverUrl, socket_handler_t on_open = NULL, log_handler_t on_log = NULL);
    void stop();

    CoinSocketClient& on(const std::string& messageType, callback_t handler); 
    void sendCommand(json_spirit::Object& cmd, callback_t callback = NULL);
};

} 
