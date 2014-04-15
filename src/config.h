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

class CoinSocketConfig
{
public:
    CoinSocketConfig() : m_bHelp(false) { }

    void init(int argc, char* argv[]);

    const std::string& getConfigFile() const { return m_configFile; }
    const std::string& getDatabaseFile() const { return m_databaseFile; }
    const std::string& getPeerHost() const { return m_peerHost; }
    const std::string& getPeerPort() const { return m_peerPort; }
    const std::string& getWebSocketPort() const { return m_webSocketPort; }

    bool help() const { return m_bHelp; }
    const std::string& getHelpOptions() const { return m_helpOptions; }

private:
    std::string m_configFile;
    std::string m_databaseFile;
    std::string m_peerHost;
    std::string m_peerPort;
    std::string m_webSocketPort; 

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
        ("peerhost", po::value<std::string>(&m_peerHost), "peer hostname")
        ("peerport", po::value<std::string>(&m_peerPort), "peer port")
        ("wsport", po::value<std::string>(&m_webSocketPort), "port to listen for inbound websocket connections")
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
    if (!vm.count("peerhost"))  { m_peerHost = "localhost";     }
    if (!vm.count("peerport"))  { m_peerPort = "8333";          }
    if (!vm.count("wsport"))    { m_webSocketPort = "12345";    }
}

