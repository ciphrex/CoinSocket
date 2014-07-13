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

typedef std::function<void(const json_spirit::Value&)> ResultCallback;
typedef std::function<void(const json_spirit::Value&)> ErrorCallback;
typedef std::pair<ResultCallback, ErrorCallback> CallbackPair;
typedef std::map<uint64_t, CallbackPair> CallbackMap;

typedef std::function<void(const json_spirit::Value&)> EventHandler;
typedef std::map<std::string, EventHandler> EventHandlerMap;
 
typedef std::function<void(connection_hdl_t)> OpenHandler;
typedef std::function<void(connection_hdl_t)> CloseHandler;
typedef std::function<void(const std::string&)> LogHandler;
typedef std::function<void(const std::string&)> ErrorHandler;

class CoinSocketClient
{
public:
    // Constructor / Destructor
    CoinSocketClient();
    ~CoinSocketClient();

    // start() blocks until disconnection occurs.
    void start(const std::string& serverUrl, OpenHandler on_open = nullptr, CloseHandler on_close = nullptr, LogHandler on_log = nullptr, ErrorHandler on_error = nullptr);
    void stop();

    // Send formatted command
    void send(json_spirit::Object& cmd, ResultCallback resultCallback = nullptr, ErrorCallback errorCallback = nullptr);

    // Subscribe to events
    CoinSocketClient& on(const std::string& eventType, EventHandler handler);

protected:
    // Connection handlers
    void onOpen(connection_hdl_t hdl);
    void onClose(connection_hdl_t hdl);
    void onFail(connection_hdl_t hdl);
    void onMessage(connection_hdl_t, message_ptr_t msg);

    // Results and errors from commands
    void onResult(const json_spirit::Value& result, uint64_t id);
    void onError(const json_spirit::Value& error, uint64_t id); 

private:
    // WebSocket connection to CoinSocket server
    client_t            client;
    std::string         serverUrl;
    connection_ptr_t    pConnection;
    bool                bConnected;

    OpenHandler         on_open;
    CloseHandler        on_close;
    LogHandler          on_log;
    ErrorHandler        on_error;

    EventHandlerMap     event_handler_map;

    uint64_t            sequence;
    CallbackMap         callback_map;
};

} 
