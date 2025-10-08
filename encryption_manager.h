#ifndef ENCRYPTION_MANAGER_H
#define ENCRYPTION_MANAGER_H

#include <string>
#include <vector>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/err.h>

class EncryptionManager {
public:
    EncryptionManager();
    ~EncryptionManager();

    // Generate Diffie-Hellman parameters
    bool generateDhParams();
    std::string getDhParamsPem() const;
    bool setDhParamsPem(const std::string& pem_params);

    // Generate Diffie-Hellman key pair
    bool generateDhKeyPair();
    std::string getDhPublicKeyPem() const;
    bool setDhPeerPublicKeyPem(const std::string& pem_public_key);

    // Compute shared secret
    bool computeSharedSecret();
    bool hasSharedSecret() const;

    // Encrypt/Decrypt data using AES-256-CBC
    std::vector<unsigned char> encrypt(const std::string& plaintext);
    std::string decrypt(const std::vector<unsigned char>& ciphertext);

private:
    EVP_PKEY* m_dhParams;
    EVP_PKEY_CTX* m_dhCtx;
    EVP_PKEY* m_dhKeyPair;
    EVP_PKEY* m_dhPeerPublicKey;
    std::vector<unsigned char> m_sharedSecret;
    std::vector<unsigned char> m_aesKey; // Derived from shared secret
    std::vector<unsigned char> m_iv;     // Initialization Vector for AES

    bool deriveAesKeyAndIv();
    void handleOpenSSLError(const std::string& func_name) const;
};

#endif // ENCRYPTION_MANAGER_H
