#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/err.h>

class WMBusDecryptor {
private:
    std::vector<uint8_t> aes_key;

    std::vector<uint8_t> hexStringToBytes(const std::string& hex) {
        std::vector<uint8_t> bytes;
        for (size_t i = 0; i < hex.length(); i += 2) {
            std::string byteString = hex.substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(strtol(byteString.c_str(), nullptr, 16));
            bytes.push_back(byte);
        }
        return bytes;
    }

    std::string bytesToHexString(const std::vector<uint8_t>& bytes) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (uint8_t byte : bytes) {
            ss << std::setw(2) << static_cast<int>(byte);
        }
        return ss.str();
    }

public:
    WMBusDecryptor(const std::string& key_hex) {
        aes_key = hexStringToBytes(key_hex);
        if (aes_key.size() != 16) {
            throw std::invalid_argument("AES key must be 16 bytes (128 bits)");
        }
    }

    struct WMBusTelegram {
        uint8_t l_field;
        uint8_t c_field;
        std::vector<uint8_t> m_field;
        uint8_t ci_field;
        std::vector<uint8_t> encrypted_payload;
        std::vector<uint8_t> decrypted_payload;

        uint32_t serial_number;
        uint16_t manufacturer;
        uint8_t version;
        uint8_t device_type;
    };

    WMBusTelegram parseTelegram(const std::string& telegram_hex) {
        WMBusTelegram telegram;
        std::vector<uint8_t> raw_data = hexStringToBytes(telegram_hex);

        if (raw_data.size() < 10) {
            throw std::invalid_argument("Telegram too short");
        }

        telegram.l_field = raw_data[0];
        telegram.c_field = raw_data[1];

        telegram.m_field = std::vector<uint8_t>(raw_data.begin() + 2, raw_data.begin() + 8);
        telegram.ci_field = raw_data[8];

        telegram.manufacturer = (telegram.m_field[1] << 8) | telegram.m_field[0];
        telegram.serial_number = (telegram.m_field[5] << 24) | (telegram.m_field[4] << 16) | 
                               (telegram.m_field[3] << 8) | telegram.m_field[2];
        telegram.version = telegram.m_field.size() > 6 ? telegram.m_field[6] : 0;
        telegram.device_type = telegram.m_field.size() > 7 ? telegram.m_field[7] : 0;

        telegram.encrypted_payload = std::vector<uint8_t>(raw_data.begin() + 9, raw_data.end());

        return telegram;
    }

    std::vector<uint8_t> decryptAES128CBC(const std::vector<uint8_t>& encrypted_data, 
                                         const std::vector<uint8_t>& iv) {
        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            throw std::runtime_error("Failed to create cipher context");
        }

        if (EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr, aes_key.data(), iv.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to initialize decryption");
        }

        std::vector<uint8_t> decrypted_data(encrypted_data.size() + AES_BLOCK_SIZE);
        int len = 0;
        int decrypted_len = 0;

        if (EVP_DecryptUpdate(ctx, decrypted_data.data(), &len, encrypted_data.data(), encrypted_data.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to decrypt data");
        }
        decrypted_len = len;

        if (EVP_DecryptFinal_ex(ctx, decrypted_data.data() + len, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to finalize decryption");
        }
        decrypted_len += len;

        EVP_CIPHER_CTX_free(ctx);

        decrypted_data.resize(decrypted_len);
        return decrypted_data;
    }

    std::vector<uint8_t> generateIV(const WMBusTelegram& telegram) {
        std::vector<uint8_t> iv(16, 0x00);

        iv[0] = telegram.m_field[0];
        iv[1] = telegram.m_field[1];

        iv[2] = telegram.m_field[2];
        iv[3] = telegram.m_field[3];
        iv[4] = telegram.m_field[4];
        iv[5] = telegram.m_field[5];

        if (telegram.m_field.size() > 6) {
            iv[6] = telegram.version;
            iv[7] = telegram.device_type;
        }

        return iv;
    }

    WMBusTelegram decryptTelegram(const std::string& telegram_hex) {
        WMBusTelegram telegram = parseTelegram(telegram_hex);

        std::cout << "Telegram Analysis:" << std::endl;
        std::cout << "L-field: " << static_cast<int>(telegram.l_field) << std::endl;
        std::cout << "C-field: 0x" << std::hex << static_cast<int>(telegram.c_field) << std::endl;
        std::cout << "M-field: " << bytesToHexString(telegram.m_field) << std::endl;
        std::cout << "CI-field: 0x" << std::hex << static_cast<int>(telegram.ci_field) << std::endl;
        std::cout << "Encrypted payload length: " << telegram.encrypted_payload.size() << " bytes" << std::endl;

        std::vector<uint8_t> iv = generateIV(telegram);
        std::cout << "Generated IV: " << bytesToHexString(iv) << std::endl;

        try {
            telegram.decrypted_payload = decryptAES128CBC(telegram.encrypted_payload, iv);
            std::cout << "Decryption successful!" << std::endl;
            std::cout << "Decrypted payload: " << bytesToHexString(telegram.decrypted_payload) << std::endl;

        } catch (const std::exception& e) {
            std::cout << "Decryption failed: " << e.what() << std::endl;

            std::vector<uint8_t> zero_iv(16, 0x00);
            try {
                telegram.decrypted_payload = decryptAES128CBC(telegram.encrypted_payload, zero_iv);
                std::cout << "Decryption with zero IV successful!" << std::endl;
                std::cout << "Decrypted payload: " << bytesToHexString(telegram.decrypted_payload) << std::endl;
            } catch (const std::exception& e2) {
                std::cout << "Decryption with zero IV also failed: " << e2.what() << std::endl;
            }
        }

        return telegram;
    }

    void displayHumanReadable(const WMBusTelegram& telegram) {
        std::cout << std::endl << "Human Readable Output:" << std::endl;
        std::cout << "======================" << std::endl;

        std::cout << "Manufacturer: 0x" << std::hex << telegram.manufacturer << std::endl;
        std::cout << "Serial Number: " << std::dec << telegram.serial_number << std::endl;
        std::cout << "Version: " << static_cast<int>(telegram.version) << std::endl;
        std::cout << "Device Type: 0x" << std::hex << static_cast<int>(telegram.device_type) << std::endl;

        if (!telegram.decrypted_payload.empty()) {
            std::cout << "Decrypted Data: " << bytesToHexString(telegram.decrypted_payload) << std::endl;
        }
    }
};

int main() {
    try {
        std::string key_hex = "4255794d3dccfd46953146e701b7db68";
        std::string message_hex = "a144c5142785895070078c20607a9d00902537ca231fa2da5889be8df3673ec136aebfb80d4ce395ba98f6b3844a115e4be1b1c9f0a2d5ffbb92906aa388deaa82c929310e9e5c4c0922a784df89cf0ded833be8da996eb5885409b6c9867978dea24001d68c603408d758a1e2b91c42ebad86a9b9d287880083bb0702850574d7b51e9c209ed68e0374e9b01febfd92b4cb9410fdeaf7fb526b742dc9a8d0682653";

        std::cout << "W-MBus Telegram AES-128 Decryptor" << std::endl;
        std::cout << "=================================" << std::endl;
        std::cout << "Key: " << key_hex << std::endl;
        std::cout << "Telegram: " << message_hex.substr(0, 50) << "..." << std::endl << std::endl;

        WMBusDecryptor decryptor(key_hex);
        WMBusDecryptor::WMBusTelegram telegram = decryptor.decryptTelegram(message_hex);

        decryptor.displayHumanReadable(telegram);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
