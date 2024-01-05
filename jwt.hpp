#pragma once

#include <string>
#include <boost/json.hpp>

namespace json = boost::json;

class JwtHelper {
public:
    JwtHelper(const std::string &secretKey);

    std::string createToken(const json::object &claims) const;

private:
    std::string base64Encode(const std::string &input) const;
    std::string sign(const std::string &data) const;

private:
    const json::object header_= {
        {"alg", "HS256"},
        {"typ", "JWT"}
    };
    const std::string secretKey_;
};
