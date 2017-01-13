///////////////////////////////////////////////////////////////////////////////
//
// CoinSocket
//
// channels.cpp
//
// Copyright (c) 2014-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "channels.h"

using namespace CoinSocket;

static Channels    g_channels;
static ChannelSets g_channelSets;

const Channels& CoinSocket::getChannels()
{
    return g_channels;
}

void CoinSocket::addChannel(const std::string& channel)
{
    g_channels.insert(channel);
}

bool CoinSocket::channelExists(const std::string& channel)
{
    return g_channels.count(channel);
}



const ChannelSets& CoinSocket::getChannelSets()
{
    return g_channelSets;
}

void CoinSocket::addChannelToSet(const std::string& channelSet, const std::string& channel)
{
    g_channelSets.insert(ChannelSetItem(channelSet, channel));
}

ChannelRange CoinSocket::getChannelRange(const std::string& channelSet)
{
    return g_channelSets.equal_range(channelSet);
}

bool CoinSocket::isChannelRangeEmpty(const ChannelRange& range)
{
    return (range.first == range.second);
}

