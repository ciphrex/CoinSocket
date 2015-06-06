///////////////////////////////////////////////////////////////////////////////
//
// CoinSocket
//
// alerts.h
//
// Copyright (c) 2015 Eric Lombrozo
//
// All Rights Reserved.
//

#pragma once

#include <SimpleSmtp/smtp.h>

namespace CoinSocket
{

void setSmtpTls(const std::string& username, const std::string& password, const std::string& url);
SmtpTls& getSmtpTls();

}
