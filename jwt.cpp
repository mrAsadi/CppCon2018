#include "jwt.hpp"
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

JwtHelper::JwtHelper(const std::string &secretKey) : secretKey_(secretKey)
{
}

std::string JwtHelper::createToken(const json::object &claims) const
{
    // Merge claims with the default header
    json::object headerWithClaims = header_;
    for (const auto &claim : claims)
    {
        headerWithClaims[claim.key()] = claim.value();
    }

    // Encode header and payload as base64
    std::string encodedHeader = base64Encode(json::serialize(headerWithClaims));
    std::string encodedPayload = base64Encode(json::serialize(claims));

    // Combine header and payload with a period
    std::string headerPayload = encodedHeader + "." + encodedPayload;

    // Create signature
    std::string signature = sign(headerPayload);

    // Encode signature as base64
    std::string encodedSignature = base64Encode(signature);

    // Combine header, payload, and signature with periods to form the final JWT token
    return headerPayload + "." + encodedSignature;
}

std::string JwtHelper::base64Encode(const std::string &input) const
{
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, input.c_str(), static_cast<int>(input.length()));
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);
    BIO_set_close(bio, BIO_NOCLOSE);
    BIO_free_all(bio);

    return std::string(bufferPtr->data, bufferPtr->length);
}

std::string JwtHelper::sign(const std::string &data) const
{
    unsigned int digestLength;
    unsigned char *digest = HMAC(EVP_sha256(), secretKey_.c_str(), static_cast<int>(secretKey_.length()),
                                 reinterpret_cast<const unsigned char *>(data.c_str()), data.length(),
                                 nullptr, &digestLength);

    return std::string(reinterpret_cast<char *>(digest), digestLength);
}
