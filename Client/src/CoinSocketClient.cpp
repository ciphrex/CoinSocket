////////////////////////////////////////////////////////////////////////////////
//
// CoinSocketClient.cpp
//
// Copyright (c) 2014 Eric Lombrozo, all rights reserved
//

#include "CoinSocketClient.h"

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

using namespace std;
using namespace json_spirit;
using namespace CoinSocket;

/// Public Methods
CoinSocketClient::CoinSocketClient()
{
    bConnected = false;
    sequence = 0;

    client.set_access_channels(websocketpp::log::alevel::all);
    client.set_error_channels(websocketpp::log::elevel::all);

    client.init_asio();

    client.set_open_handler(bind(&CoinSocketClient::onOpen, this, ::_1));
    client.set_close_handler(bind(&CoinSocketClient::onClose, this, ::_1));
    client.set_fail_handler(bind(&CoinSocketClient::onFail, this, ::_1));
    client.set_message_handler(bind(&CoinSocketClient::onMessage, this, ::_1, ::_2));
}

CoinSocketClient::~CoinSocketClient()
{
    if (bConnected)
    {
        pConnection->close(websocketpp::close::status::going_away, "");
        bConnected = false;
    }
}

void CoinSocketClient::start(const string& serverUrl, OpenHandler on_open, CloseHandler on_close, LogHandler on_log, ErrorHandler on_error)
{
    if (bConnected) throw runtime_error("Already connected.");

    error_code_t error_code;
    pConnection = client.get_connection(serverUrl, error_code);
    if (error_code)
    {
        client.get_alog().write(websocketpp::log::alevel::app, error_code.message());
        throw runtime_error(error_code.message());
    }

    sequence = 0; 
    this->on_open = on_open;
    this->on_close = on_close;
    this->on_log = on_log;
    this->on_error = on_error;
    client.connect(pConnection);
    client.run();

    client.reset();
    bConnected = false;
}

void CoinSocketClient::stop()
{
    if (bConnected)
    {
        pConnection->close(websocketpp::close::status::going_away, "");
    }
}

void CoinSocketClient::send(Object& cmd, ResultCallback resultCallback, ErrorCallback errorCallback)
{
    cmd.push_back(Pair("id", sequence));
    if (resultCallback || errorCallback)
    {
        callback_map[sequence] = CallbackPair(resultCallback, errorCallback);
    }
    sequence++;
    string cmdStr = write_string<Value>(cmd, false);
    if (on_log) on_log(string("Sending command: ") + cmdStr);
    pConnection->send(cmdStr);
}

CoinSocketClient& CoinSocketClient::on(const string& eventType, EventHandler handler)
{
    event_handler_map[eventType] = handler;
    return *this;
}

/// Protected Methods
void CoinSocketClient::onOpen(connection_hdl_t hdl)
{
    bConnected = true;
    if (on_log) on_log("Connection opened.");
    if (on_open) on_open(hdl);
}

void CoinSocketClient::onClose(connection_hdl_t hdl)
{
    bConnected = false;
    if (on_log) on_log("Connection closed.");
    if (on_close) on_close(hdl);
}

void CoinSocketClient::onFail(connection_hdl_t hdl)
{
    bConnected = false;
    if (on_error) on_error("Connection failed.");
}

void CoinSocketClient::onMessage(connection_hdl_t hdl, message_ptr_t msg)
{
    string json = msg->get_payload();

    try
    {
        if (on_log)
        {
            stringstream ss;
            ss << "Received message from server: " << json;
            on_log(ss.str());
        }

        Value value;
        read_string(json, value);
        const Object& obj = value.get_obj();
        const Value& id = find_value(obj, "id");

        const Value& result = find_value(obj, "result");
        if (result.type() != null_type && id.type() == int_type)
        {
            onResult(result, id.get_uint64());
            return;
        }

        const Value& error = find_value(obj, "error");
        if (error.type() != null_type && id.type() == int_type)
        {
            onError(error, id.get_uint64());
            return;
        }

        const Value& event = find_value(obj, "event");
        if (event.type() == str_type)
        {
            auto it = event_handler_map.find(event.get_str());
            if (it != event_handler_map.end())
            {
                const Value& data = find_value(obj, "data");
                it->second(data);
            }
            return;
        }

        if (on_error)
        {
            stringstream ss;
            ss << "Invalid server message: " << json;
            on_error(ss.str());
        }
    }
    catch (const exception& e)
    {
        if (on_error)
        {
            stringstream ss;
            ss << "Server message parse error: " << json << " - " << e.what();
            on_error(ss.str());
        }
    }
}

void CoinSocketClient::onResult(const Value& result, uint64_t id)
{
    if (on_log)
    {
        string json = write_string<Value>(result, true);
        stringstream ss;
        ss << "Received result for id " << id << ": " << json;
        on_log(ss.str());
    }

    try
    {
        auto it = callback_map.find(id);
        if (it != callback_map.end() && it->second.first)
        {
            it->second.first(result);
            callback_map.erase(id);
        }
    }
    catch (const exception& e)
    {
        if (on_error)
        {
            stringstream ss;
            ss << "Server result parse error for id " << id << ": " << e.what();
            on_error(ss.str());
        }
    }
}

void CoinSocketClient::onError(const Value& error, uint64_t id)
{
    if (on_log)
    {
        string json = write_string<Value>(error, true);
        stringstream ss;
        ss << "Received error for id " << id << ": " << json;
        on_log(ss.str());
    }

    try
    {
        auto it = callback_map.find(id);
        if (it != callback_map.end() && it->second.second)
        {
            it->second.second(error);
            callback_map.erase(id);
        }
    }
    catch (const exception& e)
    {
        if (on_error)
        {
            stringstream ss;
            ss << "Server error parse error for id " << id << ": " << e.what();
            on_error(ss.str());
        }
    }
}

