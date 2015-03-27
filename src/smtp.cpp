///////////////////////////////////////////////////////////////////////////////
//
// smtp.cpp
//
// Copyright (c) 2015 Eric Lombrozo
//
// All Rights Reserved.

#include "smtp.h"

#include <curl/curl.h>
#include <string.h>

#include <functional>

#include <iostream>

struct upload_status { int lines_read; };

//Note: using this global is an ugly temporary hack to support CURLOPT_READFUNCTION as in the examples. In particular, calls to SmtpTls::send() are not thread-safe.
static const char** payload_text;

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp)
{
  struct upload_status *upload_ctx = (struct upload_status *)userp;
  const char *data;

  if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
    return 0;
  }

  data = payload_text[upload_ctx->lines_read];

  if(data) {
    size_t len = strlen(data);
    memcpy(ptr, data, len);
    upload_ctx->lines_read++;

    return len;
  }

  return 0;
}
void SmtpTls::send(bool verifycert, bool verbose) const
{
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("Error initializing curl instance.");

    curl_easy_setopt(curl, CURLOPT_USERNAME, username_.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, password_.c_str());
    curl_easy_setopt(curl, CURLOPT_URL, url_.c_str());
    curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
    if (!verifycert)
    {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    }

    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from_.c_str());

    struct curl_slist* recipients = NULL;
    for (auto& to: tos_)    { recipients = curl_slist_append(recipients, to.c_str()); }
    for (auto& cc: ccs_)    { recipients = curl_slist_append(recipients, cc.c_str()); }
    for (auto& bcc: bccs_)  { recipients = curl_slist_append(recipients, bcc.c_str()); }
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

    std::vector<std::string> payload;
    for (auto& to: tos_)    { std::string s("To: "); s += to; s += "\r\n"; payload.push_back(s); }
    for (auto& cc: ccs_)    { std::string s("Cc: "); s += cc; s += "\r\n"; payload.push_back(s); }
    { std::string s("From: "); s += from_; s += "\r\n"; payload.push_back(s); }
    { std::string s("Subject: "); s += subject_; s += "\r\n"; payload.push_back(s); }
    { std::string s(body_); s += "\r\n"; payload.push_back(s); }

    payload_text = new const char*[payload.size() + 1];
    int i = 0;
    for (auto& line: payload) { payload_text[i++] = line.c_str(); }
    payload_text[payload.size()] = NULL;

    struct upload_status upload_ctx;
    upload_ctx.lines_read = 0;

/*
    std::function<size_t(void*, size_t, size_t, void*)> payload_source = [&](void* ptr, size_t size, size_t nmemb, void* userp) {
        std::cout << "payload_source called!" << std::endl;
        if ((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) return 0;
 
        struct upload_status* upload_ctx_ = (struct upload_status*)userp;
        const char* data = (upload_ctx_->lines_read < payload.size()) ? payload[upload_ctx_->lines_read].c_str() : nullptr;
        if (!data) return 0;

        size_t len = strlen(data);
        memcpy(ptr, data, len);
        upload_ctx_->lines_read++;
        return (int)len;
    };
*/
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);//payload_source.target<size_t(void*, size_t, size_t, void*)>());

    curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    if (verbose)
    {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    }

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);

    delete[] payload_text;
    
    if (res != CURLE_OK) throw std::runtime_error(std::string("curl_easy_perform() failed: ") + curl_easy_strerror(res));
}
