///////////////////////////////////////////////////////////////////////////////
//
// smtp.h
//
// Copyright (c) 2015 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <stdexcept>
#include <vector>

class SmtpTls
{
public:
    SmtpTls() { }
    SmtpTls(const std::string& username, const std::string& password, const std::string& url) :
        username_(username), password_(password), url_(url) { }

    void set(const std::string& username, const std::string& password, const std::string& url) { username_ = username; password_ = password; url_ = url; }
    bool isSet() const { return (!username_.empty() && !password_.empty() && !url_.empty()); }
    void setSubject(const std::string& subject) { subject_ = subject; }
    void setBody(const std::string& body) { body_ = body; }
    void setFrom(const std::string& from) { from_ = from; }
    void addTo(const std::string& to) { tos_.push_back(to); }
    void addCc(const std::string& cc) { ccs_.push_back(cc); }
    void addBcc(const std::string& bcc) { bccs_.push_back(bcc); }

    void send(bool verifycert = false, bool verbose = false) const;

private:
    std::string username_;
    std::string password_;
    std::string url_;

    std::string from_;
    std::string subject_;
    std::string body_;
    std::vector<std::string> tos_;
    std::vector<std::string> ccs_;
    std::vector<std::string> bccs_;

    std::string payload_text_;
};
