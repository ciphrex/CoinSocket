///////////////////////////////////////////////////////////////////////////////
//
// CoinSocket
//
// config.h
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
//

#pragma once

#include <string>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <boost/program_options.hpp>

const std::string DEFAULT_DATADIR = ".";
const std::string DEFAULT_PEER_HOST = "localhost";
const std::string DEFAULT_PEER_PORT = "8333";
const std::string DEFAULT_WEBSOCKET_PORT = "12345";
const std::string DEFAULT_ALLOWED_IPS = "^\\[(::1|::ffff:127\\.0\\.0\\.1)\\].*";

class CoinSocketConfig
{
public:
    CoinSocketConfig() : m_bHelp(false) { }

    void init(int argc, char* argv[]);

    const std::string& getConfigFile() const { return m_configFile; }
    const std::string& getDatabaseFile() const { return m_databaseFile; }
    const std::string& getDataDir() const { return m_dataDir; }
    const std::string& getPeerHost() const { return m_peerHost; }
    const std::string& getPeerPort() const { return m_peerPort; }
    const std::string& getWebSocketPort() const { return m_webSocketPort; }
    const std::string& getAllowedIps() const { return m_allowedIps; }
    const std::string& getTlsCertificateFile() const { return m_tlsCertificateFile; }

    bool help() const { return m_bHelp; }
    const std::string& getHelpOptions() const { return m_helpOptions; }

private:
    std::string m_configFile;
    std::string m_databaseFile;
    std::string m_dataDir;
    std::string m_peerHost;
    std::string m_peerPort;
    std::string m_webSocketPort;
    std::string m_allowedIps;
    std::string m_tlsCertificateFile;

    bool m_bHelp;
    std::string m_helpOptions;
};

inline void CoinSocketConfig::init(int argc, char* argv[])
{
    namespace po = boost::program_options;
    po::options_description options("Options");
    options.add_options()
        ("help", "display help message")
        ("config", po::value<std::string>(&m_configFile), "name of the configuration file")
        ("dbfile", po::value<std::string>(&m_databaseFile), "coin database file")
        ("datadir", po::value<std::string>(&m_dataDir), "data directory")
        ("peerhost", po::value<std::string>(&m_peerHost), "peer hostname")
        ("peerport", po::value<std::string>(&m_peerPort), "peer port")
        ("wsport", po::value<std::string>(&m_webSocketPort), "port to listen for inbound websocket connections")
        ("allowedips", po::value<std::string>(&m_allowedIps), "regular expression for allowed ip addresses")
        ("tlscertfile", po::value<std::string>(&m_tlsCertificateFile), "TLS certificate file")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, options), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        m_bHelp = true;
        std::stringstream ss;
        ss << options;
        m_helpOptions = ss.str();
        return;
    }

    if (vm.count("config"))
    {
        std::ifstream config(m_configFile);
        po::store(po::parse_config_file(config, options), vm);
        config.close();
        po::notify(vm);     
    }

    if (!vm.count("dbfile")) throw std::runtime_error("No dbfile specified.");
    if (!vm.count("datadir"))       { m_dataDir = DEFAULT_DATADIR; }
    if (!vm.count("peerhost"))      { m_peerHost = DEFAULT_PEER_HOST; }
    if (!vm.count("peerport"))      { m_peerPort = DEFAULT_PEER_PORT; }
    if (!vm.count("wsport"))        { m_webSocketPort = DEFAULT_WEBSOCKET_PORT; }
    if (!vm.count("allowedips"))    { m_allowedIps = DEFAULT_ALLOWED_IPS; }
}

