#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

struct PlayerData {
    float posX = 0.0f;
    float posZ = 0.0f;
    float yaw  = 0.0f;
    int hp     = 100;
    int score  = 0;
};

// Forward Declarations
float parseFloat(const std::string& json, const std::string& key);
int parseInt(const std::string& json, const std::string& key);

const std::string SERVER_IP = "172.20.10.2";
const int SERVER_PORT = 8080;

inline std::string sendToServer(const std::string& jsonData) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        return "{\"success\":false,\"message\":\"Socket creation failed\"}";
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP.c_str(), &addr.sin_addr);
    addr.sin_port = htons(SERVER_PORT);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(sock);
        return "{\"success\":false,\"message\":\"Connection failed.\"}";
    }

    send(sock, jsonData.c_str(), (int)jsonData.length(), 0);

    char buffer[4096] = { 0 };
    int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
    closesocket(sock);

    if (bytes <= 0) {
        return "{\"success\":false,\"message\":\"No response from server\"}";
    }

    buffer[bytes] = '\0';
    return std::string(buffer);
}

inline bool parseSuccess(const std::string& json) {
    size_t pos = json.find("\"success\"");
    if (pos == std::string::npos) return false;

    size_t colon = json.find(":", pos);
    if (colon == std::string::npos) return false;

    size_t start = colon + 1;
    while (start < json.length() && (json[start] == ' ' || json[start] == '\t')) start++;

    return json.substr(start, 4) == "true";
}

inline std::string parseMessage(const std::string& json) {
    size_t pos = json.find("\"message\"");
    if (pos == std::string::npos) return "";

    size_t colon = json.find(":", pos);
    if (colon == std::string::npos) return "";

    size_t start = json.find("\"", colon + 1);
    if (start == std::string::npos) return "";

    size_t end = json.find("\"", start + 1);
    if (end == std::string::npos) return "";

    return json.substr(start + 1, end - start - 1);
}

inline float parseFloat(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return 0.0f;

    size_t colon = json.find(":", pos);
    if (colon == std::string::npos) return 0.0f;

    size_t start = colon + 1;
    while (start < json.length() && (json[start] == ' ' || json[start] == '\t')) start++;

    size_t end = start;
    while (end < json.length() && (isdigit(json[end]) || json[end] == '.' || json[end] == '-')) end++;

    if (end > start) {
        return std::stof(json.substr(start, end - start));
    }
    return 0.0f;
}

inline int parseInt(const std::string& json, const std::string& key) {
    return (int)parseFloat(json, key);
}

inline bool registerPlayer(const std::string& username, const std::string& password) {
    std::string json = "{";
    json += "\"command\":\"REGISTER\",";
    json += "\"player_id\":\"" + username + "\",";
    json += "\"password\":\"" + password + "\"";
    json += "}";

    std::string response = sendToServer(json);
    return parseSuccess(response);
}

// Fetch complete latest state from server
inline bool loadGameFromServer(const std::string& playerName, float& x, float& y, float& z, float& yaw, int& health, int& score) {
    std::string json = "{\"command\":\"LOAD\",\"player_id\":\"" + playerName + "\"}";
    std::string response = sendToServer(json);

    if (!parseSuccess(response)) return false;

    x = parseFloat(response, "x");
    y = parseFloat(response, "y");
    z = parseFloat(response, "z");
    yaw = parseFloat(response, "yaw");
    health = parseInt(response, "health");
    score = parseInt(response, "score");

    return true;
}

inline bool loginPlayer(const std::string& username, const std::string& password, PlayerData& outData, std::string& outError) {
    // Offline bypass for developer credentials
    if (username == "dev" && password == "dev") {
        outData.posX  = 0.0f;
        outData.posZ  = 0.0f;
        outData.yaw   = 0.0f;
        outData.hp    = 100;
        outData.score = 0;
        return true;
    }

    std::string json = "{";
    json += "\"command\":\"LOGIN\",";
    json += "\"player_id\":\"" + username + "\",";
    json += "\"password\":\"" + password + "\"";
    json += "}";

    std::string response = sendToServer(json);

    if (parseSuccess(response)) {
        float tempY = 0.0f;
        // Fetch saved profile from database
        if (!loadGameFromServer(username, outData.posX, tempY, outData.posZ, outData.yaw, outData.hp, outData.score)) {
            // New user initial state fallback
            outData.posX = 0.0f;
            outData.posZ = 0.0f;
            outData.yaw  = 0.0f;
            outData.hp   = 100;
            outData.score = 0;
        }
        return true;
    } else {
        outError = parseMessage(response);
        if (outError.empty()) outError = "Server connection failed.";
        return false;
    }
}

// Full game state save payload for heartbeats and manual saves
inline bool saveGameToServer(const std::string& playerName, float x, float y, float z, float yaw, int health, int score) {
    // Offline bypass for developer account
    if (playerName == "dev") {
        return true;
    }

    std::string json = "{";
    json += "\"command\":\"SAVE\",";
    json += "\"player_id\":\"" + playerName + "\",";
    json += "\"position\":{\"x\":" + std::to_string(x) + ",\"y\":" + std::to_string(y) + ",\"z\":" + std::to_string(z) + "},";
    json += "\"yaw\":" + std::to_string(yaw) + ",";
    json += "\"health\":" + std::to_string(health) + ",";
    json += "\"score\":" + std::to_string(score);
    json += "}";

    std::string response = sendToServer(json);
    return parseSuccess(response);
}
