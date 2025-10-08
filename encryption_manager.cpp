#include "encryption_manager.h"
#include <iostream>
#include <stdexcept>
#include <memory>

// Helper for OpenSSL error handling
void EncryptionManager::handleOpenSSLError(const std::string& func_name) const {
    std::cerr << "OpenSSL Error in " << func_name << ": ";
    char err_buf[256];
    ERR_error_string_n(ERR_get_error(), err_buf, sizeof(err_buf));
    std::cerr << err_buf << std::endl;
    throw std::runtime_error("OpenSSL error in " + func_name);
}

EncryptionManager::EncryptionManager()
    : m_dhParams(nullptr),
      m_dhCtx(nullptr),
      m_dhKeyPair(nullptr),
      m_dhPeerPublicKey(nullptr)
{
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
}

EncryptionManager::~EncryptionManager() {
    if (m_dhParams) EVP_PKEY_free(m_dhParams);
    if (m_dhCtx) EVP_PKEY_CTX_free(m_dhCtx);
    if (m_dhKeyPair) EVP_PKEY_free(m_dhKeyPair);
    if (m_dhPeerPublicKey) EVP_PKEY_free(m_dhPeerPublicKey);
    
    // Clear sensitive data
    m_sharedSecret.clear();
    m_aesKey.clear();
    m_iv.clear();
    
    EVP_cleanup();
    ERR_free_strings();
}

bool EncryptionManager::generateDhParams() {
    try {
        m_dhCtx = EVP_PKEY_CTX_new_id(EVP_PKEY_DH, nullptr);
        if (!m_dhCtx) handleOpenSSLError("EVP_PKEY_CTX_new_id");

        if (EVP_PKEY_paramgen_init(m_dhCtx) <= 0) handleOpenSSLError("EVP_PKEY_paramgen_init");
        if (EVP_PKEY_CTX_set_dh_paramgen_prime_len(m_dhCtx, 2048) <= 0) handleOpenSSLError("EVP_PKEY_CTX_set_dh_paramgen_prime_len");
        if (EVP_PKEY_paramgen(m_dhCtx, &m_dhParams) <= 0) handleOpenSSLError("EVP_PKEY_paramgen");
        return true;
    } catch (const std::runtime_error& e) {
        std::cerr << "Error generating DH parameters: " << e.what() << std::endl;
        return false;
    }
}

std::string EncryptionManager::getDhParamsPem() const {
    if (!m_dhParams) return "";
    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio) handleOpenSSLError("BIO_new");
    if (PEM_write_bio_Parameters(bio, m_dhParams) <= 0) handleOpenSSLError("PEM_write_bio_Parameters");
    
    BUF_MEM* bptr;
    BIO_get_mem_ptr(bio, &bptr);
    std::string pem(bptr->data, bptr->length);
    BIO_free_all(bio);
    return pem;
}

bool EncryptionManager::setDhParamsPem(const std::string& pem_params) {
    BIO* bio = BIO_new_mem_buf(pem_params.c_str(), pem_params.length());
    if (!bio) handleOpenSSLError("BIO_new_mem_buf");
    m_dhParams = PEM_read_bio_Parameters(bio, nullptr);
    BIO_free_all(bio);
    if (!m_dhParams) handleOpenSSLError("PEM_read_bio_Parameters");
    return true;
}

bool EncryptionManager::generateDhKeyPair() {
    try {
        if (!m_dhParams) {
            std::cerr << "DH parameters not set. Cannot generate key pair." << std::endl;
            return false;
        }

        m_dhCtx = EVP_PKEY_CTX_new(m_dhParams, nullptr);
        if (!m_dhCtx) handleOpenSSLError("EVP_PKEY_CTX_new");

        if (EVP_PKEY_keygen_init(m_dhCtx) <= 0) handleOpenSSLError("EVP_PKEY_keygen_init");
        if (EVP_PKEY_keygen(m_dhCtx, &m_dhKeyPair) <= 0) handleOpenSSLError("EVP_PKEY_keygen");
        return true;
    } catch (const std::runtime_error& e) {
        std::cerr << "Error generating DH key pair: " << e.what() << std::endl;
        return false;
    }
}

std::string EncryptionManager::getDhPublicKeyPem() const {
    if (!m_dhKeyPair) return "";
    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio) handleOpenSSLError("BIO_new");
    if (PEM_write_bio_PUBKEY(bio, m_dhKeyPair) <= 0) handleOpenSSLError("PEM_write_bio_PUBKEY");
    
    BUF_MEM* bptr;
    BIO_get_mem_ptr(bio, &bptr);
    std::string pem(bptr->data, bptr->length);
    BIO_free_all(bio);
    return pem;
}

bool EncryptionManager::setDhPeerPublicKeyPem(const std::string& pem_public_key) {
    BIO* bio = BIO_new_mem_buf(pem_public_key.c_str(), pem_public_key.length());
    if (!bio) handleOpenSSLError("BIO_new_mem_buf");
    m_dhPeerPublicKey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free_all(bio);
    if (!m_dhPeerPublicKey) handleOpenSSLError("PEM_read_bio_PUBKEY");
    return true;
}

bool EncryptionManager::computeSharedSecret() {
    try {
        if (!m_dhKeyPair || !m_dhPeerPublicKey) {
            std::cerr << "Local key pair or peer public key not set. Cannot compute shared secret." << std::endl;
            return false;
        }

        EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(m_dhKeyPair, nullptr);
        if (!ctx) handleOpenSSLError("EVP_PKEY_CTX_new for derivation");

        if (EVP_PKEY_derive_init(ctx) <= 0) handleOpenSSLError("EVP_PKEY_derive_init");
        if (EVP_PKEY_derive_set_peer(ctx, m_dhPeerPublicKey) <= 0) handleOpenSSLError("EVP_PKEY_derive_set_peer");

        size_t secret_len;
        if (EVP_PKEY_derive(ctx, nullptr, &secret_len) <= 0) handleOpenSSLError("EVP_PKEY_derive (get length)");

        m_sharedSecret.resize(secret_len);
        if (EVP_PKEY_derive(ctx, m_sharedSecret.data(), &secret_len) <= 0) handleOpenSSLError("EVP_PKEY_derive");
        m_sharedSecret.resize(secret_len); // Adjust size if actual length is smaller

        EVP_PKEY_CTX_free(ctx);
        return deriveAesKeyAndIv();
    } catch (const std::runtime_error& e) {
        std::cerr << "Error computing shared secret: " << e.what() << std::endl;
        return false;
    }
}

bool EncryptionManager::hasSharedSecret() const {
    return !m_sharedSecret.empty();
}

bool EncryptionManager::deriveAesKeyAndIv() {
    if (m_sharedSecret.empty()) {
        std::cerr << "Shared secret not available for AES key derivation." << std::endl;
        return false;
    }

    // Use a KDF (Key Derivation Function) like HKDF or PBKDF2 for better security
    // For simplicity, we'll just hash the shared secret to get a fixed-size key and IV.
    // In a real-world application, use a proper KDF.
    
    // SHA256 hash of shared secret for AES key
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    if (!EVP_Digest(m_sharedSecret.data(), m_sharedSecret.size(), hash, &hash_len, EVP_sha256(), nullptr)) {
        handleOpenSSLError("EVP_Digest for AES key");
        return false;
    }
    m_aesKey.assign(hash, hash + EVP_CIPHER_key_length(EVP_aes_256_cbc()));

    // Generate a random IV for each encryption operation, or derive it differently.
    // For now, we'll generate a fixed IV for demonstration.
    // In a real application, IV should be unique for each encryption and sent with ciphertext.
    m_iv.resize(EVP_CIPHER_iv_length(EVP_aes_256_cbc()));
    if (RAND_bytes(m_iv.data(), m_iv.size()) <= 0) {
        handleOpenSSLError("RAND_bytes for IV");
        return false;
    }
    return true;
}

std::vector<unsigned char> EncryptionManager::encrypt(const std::string& plaintext) {
    if (!hasSharedSecret() || m_aesKey.empty() || m_iv.empty()) {
        std::cerr << "Encryption key or IV not available." << std::endl;
        return {};
    }

    std::vector<unsigned char> ciphertext;
    int len;
    int ciphertext_len;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) handleOpenSSLError("EVP_CIPHER_CTX_new");

    try {
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, m_aesKey.data(), m_iv.data()) <= 0) {
            handleOpenSSLError("EVP_EncryptInit_ex");
        }

        ciphertext.resize(plaintext.length() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
        if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, (const unsigned char*)plaintext.data(), plaintext.length()) <= 0) {
            handleOpenSSLError("EVP_EncryptUpdate");
        }
        ciphertext_len = len;

        if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) <= 0) {
            handleOpenSSLError("EVP_EncryptFinal_ex");
        }
        ciphertext_len += len;
        ciphertext.resize(ciphertext_len);
    } catch (const std::runtime_error& e) {
        std::cerr << "Encryption failed: " << e.what() << std::endl;
        ciphertext.clear();
    }
    EVP_CIPHER_CTX_free(ctx);
    return ciphertext;
}

std::string EncryptionManager::decrypt(const std::vector<unsigned char>& ciphertext) {
    if (!hasSharedSecret() || m_aesKey.empty() || m_iv.empty() || ciphertext.empty()) {
        std::cerr << "Decryption key, IV, or ciphertext not available." << std::endl;
        return "";
    }

    std::string plaintext;
    int len;
    int plaintext_len;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) handleOpenSSLError("EVP_CIPHER_CTX_new");

    try {
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, m_aesKey.data(), m_iv.data()) <= 0) {
            handleOpenSSLError("EVP_DecryptInit_ex");
        }

        plaintext.resize(ciphertext.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
        if (EVP_DecryptUpdate(ctx, (unsigned char*)plaintext.data(), &len, ciphertext.data(), ciphertext.size()) <= 0) {
            handleOpenSSLError("EVP_DecryptUpdate");
        }
        plaintext_len = len;

        if (EVP_DecryptFinal_ex(ctx, (unsigned char*)plaintext.data() + len, &len) <= 0) {
            handleOpenSSLError("EVP_DecryptFinal_ex");
        }
        plaintext_len += len;
        plaintext.resize(plaintext_len);
    } catch (const std::runtime_error& e) {
        std::cerr << "Decryption failed: " << e.what() << std::endl;
        plaintext.clear();
    }
    EVP_CIPHER_CTX_free(ctx);
    return plaintext;
}
