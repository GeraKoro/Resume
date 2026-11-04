#include "env_config.h"

#include <cstdlib>
#include <fstream>
#include <string>

static std::string KeyFromEnvFile(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return "";
    }

    std::string line;
    const std::string k_key_prefix = "YANDEX_API_KEY=";
    if (std::getline(in, line) && line.rfind(k_key_prefix, 0) == 0) {
        const std::string key = line.substr(k_key_prefix.size());
        if (!key.empty()) {
            return key;
        }
    }

    return "";
}

std::string GetApiKey() {
    if (const char* env_key = std::getenv("YANDEX_API_KEY")) {
        if (*env_key != '\0') {
            return env_key;
        }
    }

    const std::string env_path[] = {".env", "../.env", "../../.env"};

    for (const auto& path : env_path) {
        const std::string key = KeyFromEnvFile(path);
        if (!key.empty()) {
            return key;
        }
    }

    return "";
}
