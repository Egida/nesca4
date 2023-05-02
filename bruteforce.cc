#include "include/bruteforce.h"
#include "include/callbacks.h"
#include "include/other.h"
#include "include/prints.h"
#include <thread>
#include <curl/curl.h>

nesca_prints nsp;

std::string brute_ftp(const std::string ip, const std::string login, const std::string pass, int brute_log, int verbose){
    std::string result;
    CURL* curl = curl_easy_init();
    bool success = false;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_USERNAME, "");
        curl_easy_setopt(curl, CURLOPT_PASSWORD, "");
        if (verbose){
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        }
        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);
        curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36");
        curl_easy_setopt(curl, CURLOPT_FTPPORT, "-");
        curl_easy_setopt(curl, CURLOPT_FTP_RESPONSE_TIMEOUT, 15L);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
        curl_easy_setopt(curl, CURLOPT_FTP_USE_EPSV, 1L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 15L);

        std::string url = "ftp://" + ip;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        if (brute_log){
            std::cout << nsp.main_nesca_out("FTP", "      try: " + login + "@" + pass, 2, "[BRUTEFORCE]", "", "", "") << std::endl;
        }
        curl_easy_setopt(curl, CURLOPT_USERNAME, login.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, pass.c_str());

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            result = login + ":" + pass + "@";
            curl_easy_cleanup(curl);
            return result;
        }
        else {
            curl_easy_cleanup(curl);
            return "";
        }
    }
    curl_easy_cleanup(curl);
    return "";
}

std::string threads_brute_ftp(const std::string ip, const std::vector<std::string> logins, const std::vector<std::string> passwords, int brute_log, int verbose, int brute_timeout_ms) {
    std::vector<std::thread> threads;
    std::vector<std::string> results;

    for (const auto& login : logins) {
        for (const auto& password : passwords) {
            delay_ms(brute_timeout_ms);
            threads.emplace_back([ip, login, password, brute_log, verbose, &results]() {
                std::string temp = brute_ftp(ip, login, password, brute_log, verbose);
                if (!temp.empty() && temp.length() > 3) {
                    results.push_back(temp);
                }
            });
        }
    }

    for (auto& thread : threads) {
        thread.join();
    }

    if (!results.empty()) {
        return results[0];
    } else {
        return "";
    }
}

std::string brute_ssh(const std::string& ip, const std::string login, const std::string pass, int brute_log, int verbose, int known_hosts){
    std::string result;
    CURL* curl = curl_easy_init();
    bool success = false;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_USERNAME, "");
        curl_easy_setopt(curl, CURLOPT_PASSWORD, "");

        if (verbose){
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        }

        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);
        curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L);
        curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36");
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 15L);
        curl_easy_setopt(curl, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_PASSWORD);

        if (known_hosts){
            curl_easy_setopt(curl, CURLOPT_SSH_HOST_PUBLIC_KEY_MD5, "serverkeymd5");
            curl_easy_setopt(curl, CURLOPT_SSH_KNOWNHOSTS, "/path/to/known_hosts");
        }
        else {
            curl_easy_setopt(curl, CURLOPT_SSH_HOST_PUBLIC_KEY_MD5, "");
        }

        curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, 15L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        std::string url = "sftp://" + ip;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        if (brute_log) {
            std::cout << nsp.main_nesca_out("SSH", "      try: " + login + "@" + pass, 2, "[BRUTEFORCE]", "", "", "") << std::endl;
        }

        curl_easy_setopt(curl, CURLOPT_USERNAME, login.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, pass.c_str());

        CURLcode res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            result = (login + ":" + pass + "@");
            curl_easy_cleanup(curl);
            return result;
        }
        else {
            curl_easy_cleanup(curl);
            return "";
        }
    }
    curl_easy_cleanup(curl);
    return "";
}

std::string threads_brute_ssh(const std::string ip, const std::vector<std::string> logins, const std::vector<std::string> passwords, int brute_log, int verbose, int known_hosts, int brute_timeout_ms) {
    std::vector<std::thread> threads;
    std::vector<std::string> results;

    for (const auto& login : logins) {
        for (const auto& password : passwords) {
            delay_ms(brute_timeout_ms);
            threads.emplace_back([ip, login, password, brute_log, verbose, known_hosts, &results]() {
                std::string temp = brute_ssh(ip, login, password, brute_log, verbose, known_hosts);
                if (!temp.empty() && temp.length() > 3) {
                    results.push_back(temp);
                }
            });
        }
    }

    for (auto& thread : threads) {
        thread.join();
    }

    if (!results.empty()) {
        return results[0];
    } else {
        return "";
    }
}

std::string brute_rtsp(const std::string ip, const std::string login, const std::string pass, int brute_log, int verbose){
    std::string result;
    curl_global_init(CURL_GLOBAL_ALL);
    CURL* curl = curl_easy_init();

    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_USERNAME, "");
        curl_easy_setopt(curl, CURLOPT_PASSWORD, "");
        if (verbose)
        {
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        }
        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);
        curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "LibVLC/2.1.5 (LIVE555 Streaming Media v2014.05.27"); 
        curl_easy_setopt(curl, CURLOPT_RTSP_TRANSPORT, "TCP");
        curl_easy_setopt(curl, CURLOPT_RTSP_REQUEST, CURL_RTSPREQ_OPTIONS);

        std::string url = "rtsp://" + ip;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        if (brute_log){
            std::cout << nsp.main_nesca_out("RTSP", "      try: " + login + "@" + pass, 2, "[BRUTEFORCE]", "", "", "") << std::endl;
        }
        curl_easy_setopt(curl, CURLOPT_USERNAME, login.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, pass.c_str());

        CURLcode res = curl_easy_perform(curl);

        if (res == CURLE_OK){
            result = login + ":" + pass + "@";
            curl_easy_cleanup(curl);
            return result;
        }
        else {
            curl_easy_cleanup(curl);
            return "";
        }
    }
    curl_global_cleanup();
    return "";
}
std::string threads_brute_rtsp(const std::string ip, const std::vector<std::string> logins, const std::vector<std::string> passwords, int brute_log, int verbose, int brute_timeout_ms) {
    std::vector<std::thread> threads;
    std::vector<std::string> results;

    for (const auto& login : logins) {
        for (const auto& password : passwords) {
            delay_ms(brute_timeout_ms);
            threads.emplace_back([ip, login, password, brute_log, verbose, &results]() {
                std::string temp = brute_rtsp(ip, login, password, brute_log, verbose);
                if (!temp.empty() && temp.length() > 3) {
                    results.push_back(temp);
                }
            });
        }
    }

    for (auto& thread : threads) {
        thread.join();
    }

    if (!results.empty()) {
        return results[0];
    } else {
        return "";
    }
}


std::string brute_http(const std::string ip, const std::string login, const std::string pass, int brute_log, int verbose){
    std::string result;
    CURL* curl = curl_easy_init();
    bool success = false;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
        curl_easy_setopt(curl, CURLOPT_USERNAME, login.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, pass.c_str());
        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);
        curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36");
        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
        if (verbose){
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        }
        std::string url_with_protocol = "http://" + ip;
        curl_easy_setopt(curl, CURLOPT_URL, url_with_protocol.c_str());

        if (brute_log){
            std::cout << nsp.main_nesca_out("HTTP", "      try: " + login + "@" + pass, 2, "[BRUTEFORCE]", "", "", "") << std::endl;
        }

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            result = login + ":" + pass + "@";
            curl_easy_cleanup(curl);
            return result;
        }
        else {
            curl_easy_cleanup(curl);
            return "";
        }
    }
    curl_easy_cleanup(curl);
    return "";
}

std::string threads_brute_http(const std::string ip, const std::vector<std::string> logins, const std::vector<std::string> passwords, int brute_log, int verbose, int brute_timeout_ms) {
    std::vector<std::thread> threads;
    std::vector<std::string> results;

    for (const auto& login : logins) {
        for (const auto& password : passwords) {
            delay_ms(brute_timeout_ms);
            threads.emplace_back([ip, login, password, brute_log, verbose, &results]() {
                std::string temp = brute_http(ip, login, password, brute_log, verbose);
                if (!temp.empty() && temp.length() > 3) {
                    results.push_back(temp);
                }
            });
        }
    }

    for (auto& thread : threads) {
        thread.join();
    }

    if (!results.empty()) {
        return results[0];
    } else {
        return "";
    }
}