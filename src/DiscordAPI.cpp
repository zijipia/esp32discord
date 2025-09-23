#include "DiscordAPI.h"

// Constructor
DiscordAPI::DiscordAPI() {
    _botToken = "";
    _clientId = "";
    _clientSecret = "";
    _redirectUri = "";
    _wsConnected = false;
    _wsAuthenticated = false;
    _heartbeatInterval = 0;
    _lastHeartbeat = 0;
    _sequenceNumber = -1;
    _sessionId = "";
    _resumeGatewayUrl = "";
    _lastRequestTime = 0;
    _requestCount = 0;
    _rateLimitReset = 0;
    _onReady = nullptr;
    _onMessage = nullptr;
    _onGuildCreate = nullptr;
    _onError = nullptr;
    _onDebug = nullptr;
    _onRaw = nullptr;
    _lastReconnectAttempt = 0;
    _reconnectAttempts = 0;
    _maxReconnectAttempts = 5;
    _reconnectDelay = 5000; // Start with 5 seconds
    _lastHeartbeatAck = 0;
    _connectionStartTime = 0;
    _heartbeatMissedCount = 0;
    _maxHeartbeatMissed = 3;
    _gatewayIntents = DISCORD_INTENT_DEFAULT;
    
    // Configure SSL for HTTPS requests
    _wifiClient.setInsecure(); // Skip certificate verification for now
    
    // Test debug log in constructor
    _debugLog("DiscordAPI constructor called", DEBUG_LEVEL_INFO);
}

// Destructor
DiscordAPI::~DiscordAPI() {
    // Cleanup WebSocket connection
    if (_wsConnected) {
        _webSocket.disconnect();
    }
    
    // Clear callbacks
    _onReady = nullptr;
    _onMessage = nullptr;
    _onGuildCreate = nullptr;
    _onError = nullptr;
    _onDebug = nullptr;
    _onRaw = nullptr;
}

// Authentication methods
bool DiscordAPI::setBotToken(String token) {
    if (token.length() == 0) {
        _debugLog("Invalid bot token (empty)", DEBUG_LEVEL_ERROR);
        return false;
    }
    
    // Basic validation of bot token format
    if (token.length() < 50) {
        _debugLog("Bot token seems too short (length: " + String(token.length()) + ")", DEBUG_LEVEL_WARNING);
    }
    
    // Check if token starts with expected format
    if (!token.startsWith("MT") && !token.startsWith("OD") && !token.startsWith("MTA")) {
        _debugLog("Bot token format may be invalid (should start with MT/OD/MTA)", DEBUG_LEVEL_WARNING);
    }
    
    _botToken = token;
    _debugLog("Bot token set successfully (length: " + String(token.length()) + ")", DEBUG_LEVEL_INFO);
    _debugLog("This is a VERBOSE test message", DEBUG_LEVEL_VERBOSE);
    return true;
}

bool DiscordAPI::setOAuth2Credentials(String clientId, String clientSecret, String redirectUri) {
    if (clientId.length() == 0 || clientSecret.length() == 0 || redirectUri.length() == 0) {
        return false;
    }
    _clientId = clientId;
    _clientSecret = clientSecret;
    _redirectUri = redirectUri;
    return true;
}

String DiscordAPI::getOAuth2URL(String scope) {
    if (_clientId.length() == 0) {
        return "";
    }
    return "https://discord.com/api/oauth2/authorize?client_id=" + _clientId + 
           "&redirect_uri=" + _redirectUri + 
           "&response_type=code&scope=" + scope;
}

String DiscordAPI::exchangeCodeForToken(String code) {
    if (_clientId.length() == 0 || _clientSecret.length() == 0 || code.length() == 0) {
        return "";
    }
    
    String url = "https://discord.com/api/oauth2/token";
    String body = "client_id=" + _clientId + 
                  "&client_secret=" + _clientSecret + 
                  "&grant_type=authorization_code&code=" + code + 
                  "&redirect_uri=" + _redirectUri;
    
    _httpClient.begin(_wifiClient, url);
    _httpClient.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    int httpResponseCode = _httpClient.POST(body);
    String response = _httpClient.getString();
    _httpClient.end();
    
    if (httpResponseCode == 200) {
        JsonDocument doc;
        deserializeJson(doc, response);
        return doc["access_token"].as<String>();
    }
    
    return "";
}

bool DiscordAPI::testBotToken() {
    if (_botToken.length() == 0) {
        _debugLog("Cannot test bot token: No token set", DEBUG_LEVEL_ERROR);
        return false;
    }
    
    _debugLog("Testing bot token...", DEBUG_LEVEL_INFO);
    _debugLog("Token length: " + String(_botToken.length()), DEBUG_LEVEL_VERBOSE);
    _debugLog("Token starts with: " + _botToken.substring(0, min(10, (int)_botToken.length())), DEBUG_LEVEL_VERBOSE);
    _debugLog("About to make HTTP request to Discord API", DEBUG_LEVEL_VERBOSE);
    
    
    DiscordResponse response = _makeRequest("GET", "/users/@me");
    
    
    _debugLog("API Response Status: " + String(response.statusCode), DEBUG_LEVEL_INFO);
    _debugLog("API Response Body: " + response.body, DEBUG_LEVEL_VERBOSE);
    _debugLog("API Response Success: " + String(response.success ? "true" : "false"), DEBUG_LEVEL_INFO);
    
    if (response.success) {
        _debugLog("Bot token is valid!", DEBUG_LEVEL_INFO);
        return true;
    } else {
        _debugLog("Bot token test failed: " + response.error, DEBUG_LEVEL_ERROR);
        _debugLog("Status code: " + String(response.statusCode), DEBUG_LEVEL_ERROR);
        return false;
    }
}

// Internal methods
String DiscordAPI::_getAuthHeader() {
    if (_botToken.length() > 0) {
        return "Bot " + _botToken;
    }
    return "";
}

DiscordResponse DiscordAPI::_makeRequest(String method, String endpoint, String body) {
    DiscordResponse response;
    response.success = false;
    response.statusCode = 0;
    response.body = "";
    response.error = "";
    
    _debugLog("Sending request: " + method + " " + endpoint, DEBUG_LEVEL_VERBOSE);
    
    // Check rate limiting
    if (isRateLimited()) {
        response.error = "Rate limited. Try again later.";
        _debugLog("Request rate limited: " + endpoint, DEBUG_LEVEL_WARNING);
        return response;
    }
    
    String url = DISCORD_API_BASE + endpoint;
    _debugLog("Full URL: " + url, DEBUG_LEVEL_VERBOSE);
    
    _httpClient.begin(_wifiClient, url);
    
    String authHeader = _getAuthHeader();
    _debugLog("Auth header: " + authHeader, DEBUG_LEVEL_VERBOSE);
    
    if (authHeader.length() > 0) {
        _httpClient.addHeader("Authorization", authHeader);
    }
    _httpClient.addHeader("Content-Type", "application/json");
    _httpClient.addHeader("User-Agent", "DiscordBot (ESP32, 1.0.0)");
    
    _debugLog("HTTP Headers set", DEBUG_LEVEL_VERBOSE);
    
    int httpResponseCode = 0;
    if (method == "GET") {
        _debugLog("Sending GET request...", DEBUG_LEVEL_VERBOSE);
        httpResponseCode = _httpClient.GET();
    } else if (method == "POST") {
        _debugLog("Sending POST request...", DEBUG_LEVEL_VERBOSE);
        httpResponseCode = _httpClient.POST(body);
    } else if (method == "PUT") {
        _debugLog("Sending PUT request...", DEBUG_LEVEL_VERBOSE);
        httpResponseCode = _httpClient.PUT(body);
    } else if (method == "PATCH") {
        _debugLog("Sending PATCH request...", DEBUG_LEVEL_VERBOSE);
        httpResponseCode = _httpClient.PATCH(body);
    } else if (method == "DELETE") {
        _debugLog("Sending DELETE request...", DEBUG_LEVEL_VERBOSE);
        httpResponseCode = _httpClient.sendRequest("DELETE");
    }
    
    _debugLog("HTTP Response Code: " + String(httpResponseCode), DEBUG_LEVEL_VERBOSE);
    
    response.statusCode = httpResponseCode;
    response.body = _httpClient.getString();
    _httpClient.end();
    
    _debugLog("Response body length: " + String(response.body.length()), DEBUG_LEVEL_VERBOSE);
    
    // Update rate limiting
    _lastRequestTime = millis();
    _requestCount++;
    
    if (httpResponseCode >= 200 && httpResponseCode < 300) {
        response.success = true;
        _debugLog("Request successful: " + String(httpResponseCode), DEBUG_LEVEL_VERBOSE);
    } else {
        response.success = false;
        response.error = "HTTP " + String(httpResponseCode) + ": " + response.body;
        _debugLog("Request failed: " + response.error, DEBUG_LEVEL_ERROR);
    }
    
    return response;
}

// REST API methods
DiscordUser DiscordAPI::getCurrentUser() {
    DiscordUser user;
    DiscordResponse response = _makeRequest("GET", "/users/@me");
    
    if (response.success) {
        JsonDocument doc;
        deserializeJson(doc, response.body);
        _parseUser(doc.as<JsonObject>(), user);
    }
    
    return user;
}

DiscordUser DiscordAPI::getUser(String userId) {
    DiscordUser user;
    DiscordResponse response = _makeRequest("GET", "/users/" + userId);
    
    if (response.success) {
        JsonDocument doc;
        deserializeJson(doc, response.body);
        _parseUser(doc.as<JsonObject>(), user);
    }
    
    return user;
}

DiscordGuild DiscordAPI::getGuild(String guildId) {
    DiscordGuild guild;
    DiscordResponse response = _makeRequest("GET", "/guilds/" + guildId);
    
    if (response.success) {
        JsonDocument doc;
        deserializeJson(doc, response.body);
        _parseGuild(doc.as<JsonObject>(), guild);
    }
    
    return guild;
}

DiscordChannel DiscordAPI::getChannel(String channelId) {
    DiscordChannel channel;
    DiscordResponse response = _makeRequest("GET", "/channels/" + channelId);
    
    if (response.success) {
        JsonDocument doc;
        deserializeJson(doc, response.body);
        _parseChannel(doc.as<JsonObject>(), channel);
    }
    
    return channel;
}

DiscordMessage DiscordAPI::getMessage(String channelId, String messageId) {
    DiscordMessage message;
    DiscordResponse response = _makeRequest("GET", "/channels/" + channelId + "/messages/" + messageId);
    
    if (response.success) {
        JsonDocument doc;
        deserializeJson(doc, response.body);
        _parseMessage(doc.as<JsonObject>(), message);
    }
    
    return message;
}

DiscordMessage* DiscordAPI::getChannelMessages(String channelId, int limit, String before, String after, String around) {
    String endpoint = "/channels/" + channelId + "/messages?limit=" + String(limit);
    
    if (before.length() > 0) {
        endpoint += "&before=" + before;
    }
    if (after.length() > 0) {
        endpoint += "&after=" + after;
    }
    if (around.length() > 0) {
        endpoint += "&around=" + around;
    }
    
    DiscordResponse response = _makeRequest("GET", endpoint);
    
    if (response.success) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response.body);
        if (error) {
            _debugLog("JSON parse error in getChannelMessages: " + String(error.c_str()), DEBUG_LEVEL_ERROR);
            return nullptr;
        }
        
        if (doc.is<JsonArray>()) {
            JsonArray messages = doc.as<JsonArray>();
            int messageCount = messages.size();
            
            if (messageCount > 0) {
                DiscordMessage* messageArray = new (std::nothrow) DiscordMessage[messageCount];
                if (messageArray == nullptr) {
                    _debugLog("Failed to allocate memory for messages", DEBUG_LEVEL_ERROR);
                    return nullptr;
                }
                
                for (int i = 0; i < messageCount; i++) {
                    if (messages[i].is<JsonObject>()) {
                        _parseMessage(messages[i].as<JsonObject>(), messageArray[i]);
                    }
                }
                
                return messageArray;
            }
        }
    }
    
    return nullptr;
}

DiscordResponse DiscordAPI::sendMessage(String channelId, String content, bool tts) {
    if (content.length() > DISCORD_MAX_MESSAGE_LENGTH) {
        DiscordResponse response;
        response.success = false;
        response.error = "Message too long. Maximum length is " + String(DISCORD_MAX_MESSAGE_LENGTH) + " characters.";
        return response;
    }
    
    JsonDocument doc;
    doc["content"] = content;
    doc["tts"] = tts;
    
    String body;
    serializeJson(doc, body);
    
    return _makeRequest("POST", "/channels/" + channelId + "/messages", body);
}

DiscordResponse DiscordAPI::editMessage(String channelId, String messageId, String content) {
    if (content.length() > DISCORD_MAX_MESSAGE_LENGTH) {
        DiscordResponse response;
        response.success = false;
        response.error = "Message too long. Maximum length is " + String(DISCORD_MAX_MESSAGE_LENGTH) + " characters.";
        return response;
    }
    
    JsonDocument doc;
    doc["content"] = content;
    
    String body;
    serializeJson(doc, body);
    
    return _makeRequest("PATCH", "/channels/" + channelId + "/messages/" + messageId, body);
}

DiscordResponse DiscordAPI::deleteMessage(String channelId, String messageId) {
    return _makeRequest("DELETE", "/channels/" + channelId + "/messages/" + messageId);
}

DiscordResponse DiscordAPI::addReaction(String channelId, String messageId, String emoji) {
    return _makeRequest("PUT", "/channels/" + channelId + "/messages/" + messageId + "/reactions/" + emoji + "/@me");
}

DiscordResponse DiscordAPI::removeReaction(String channelId, String messageId, String emoji, String userId) {
    return _makeRequest("DELETE", "/channels/" + channelId + "/messages/" + messageId + "/reactions/" + emoji + "/" + userId);
}

DiscordResponse DiscordAPI::removeAllReactions(String channelId, String messageId) {
    return _makeRequest("DELETE", "/channels/" + channelId + "/messages/" + messageId + "/reactions");
}

DiscordResponse DiscordAPI::removeAllReactionsForEmoji(String channelId, String messageId, String emoji) {
    return _makeRequest("DELETE", "/channels/" + channelId + "/messages/" + messageId + "/reactions/" + emoji);
}

// WebSocket methods
bool DiscordAPI::connectWebSocket() {
    if (_botToken.length() == 0) {
        _debugLog("Cannot connect WebSocket: Bot token not set", DEBUG_LEVEL_ERROR);
        if (_onError) _onError("Bot token not set");
        return false;
    }
    
    _debugLog("Connecting to WebSocket...", DEBUG_LEVEL_INFO);
    
    // Determine gateway host and path
    const char* defaultGatewayHost = "gateway.discord.gg";
    const char* defaultGatewayPath = "/?v=10&encoding=json";
    String gatewayHost = defaultGatewayHost;
    String gatewayPath = defaultGatewayPath;

    if (_resumeGatewayUrl.length() > 0) {
        _debugLog("Resume gateway URL available: " + _resumeGatewayUrl, DEBUG_LEVEL_VERBOSE);
        _parseGatewayUrl(_resumeGatewayUrl, gatewayHost, gatewayPath);
    } else {
        _debugLog("Using default Discord gateway", DEBUG_LEVEL_VERBOSE);
    }

    if (gatewayHost.length() == 0) {
        gatewayHost = defaultGatewayHost;
    }

    if (gatewayPath.length() == 0) {
        gatewayPath = defaultGatewayPath;
    } else if (!gatewayPath.startsWith("/")) {
        gatewayPath = "/" + gatewayPath;
    }

    _debugLog("Using gateway host: " + gatewayHost, DEBUG_LEVEL_VERBOSE);
    _debugLog("Using gateway path: " + gatewayPath, DEBUG_LEVEL_VERBOSE);
    _debugLog("Using gateway intents mask: " + String((unsigned long)_gatewayIntents), DEBUG_LEVEL_VERBOSE);

    // Try different WebSocket configuration
    _webSocket.beginSSL(gatewayHost.c_str(), 443, gatewayPath.c_str());
    _webSocket.setAuthorization("", _botToken.c_str());
    _webSocket.setReconnectInterval(5000);
    _webSocket.onEvent([this](WStype_t type, uint8_t* payload, size_t length) {
        switch (type) {
            case WStype_DISCONNECTED: {
                _wsConnected = false;
                _wsAuthenticated = false;
                _debugLog("WebSocket disconnected", DEBUG_LEVEL_WARNING);
                uint16_t closeCode = 0;
                if (payload != nullptr && length >= sizeof(uint16_t)) {
                    closeCode = (static_cast<uint16_t>(payload[0]) << 8) | payload[1];
                } else if (payload != nullptr && length == 1) {
                    closeCode = payload[0];
                }

                if (closeCode != 0) {
                    _debugLog("Disconnect code: " + String(closeCode), DEBUG_LEVEL_WARNING);
                    // Log disconnect reason based on code
                    switch (closeCode) {
                        case 1000:
                            _debugLog("Disconnect reason: Normal closure", DEBUG_LEVEL_INFO);
                            break;
                        case 1001:
                            _debugLog("Disconnect reason: Going away", DEBUG_LEVEL_INFO);
                            break;
                        case 1002:
                            _debugLog("Disconnect reason: Protocol error", DEBUG_LEVEL_ERROR);
                            break;
                        case 1003:
                            _debugLog("Disconnect reason: Unsupported data", DEBUG_LEVEL_ERROR);
                            break;
                        case 1006:
                            _debugLog("Disconnect reason: Abnormal closure", DEBUG_LEVEL_ERROR);
                            break;
                        case 1007:
                            _debugLog("Disconnect reason: Invalid frame payload data", DEBUG_LEVEL_ERROR);
                            break;
                        case 1008:
                            _debugLog("Disconnect reason: Policy violation", DEBUG_LEVEL_ERROR);
                            break;
                        case 1009:
                            _debugLog("Disconnect reason: Message too big", DEBUG_LEVEL_ERROR);
                            break;
                        case 1010:
                            _debugLog("Disconnect reason: Missing extension", DEBUG_LEVEL_ERROR);
                            break;
                        case 1011:
                            _debugLog("Disconnect reason: Internal error", DEBUG_LEVEL_ERROR);
                            break;
                        case 4000:
                            _debugLog("Disconnect reason: Unknown error", DEBUG_LEVEL_ERROR);
                            break;
                        case 4001:
                            _debugLog("Disconnect reason: Unknown opcode", DEBUG_LEVEL_ERROR);
                            break;
                        case 4002:
                            _debugLog("Disconnect reason: Decode error", DEBUG_LEVEL_ERROR);
                            break;
                        case 4003:
                            _debugLog("Disconnect reason: Not authenticated", DEBUG_LEVEL_ERROR);
                            break;
                        case 4004:
                            _debugLog("Disconnect reason: Authentication failed", DEBUG_LEVEL_ERROR);
                            break;
                        case 4005:
                            _debugLog("Disconnect reason: Already authenticated", DEBUG_LEVEL_ERROR);
                            break;
                        case 4007:
                            _debugLog("Disconnect reason: Invalid sequence", DEBUG_LEVEL_ERROR);
                            break;
                        case 4008:
                            _debugLog("Disconnect reason: Rate limited", DEBUG_LEVEL_ERROR);
                            break;
                        case 4009:
                            _debugLog("Disconnect reason: Session timed out", DEBUG_LEVEL_ERROR);
                            break;
                        case 4010:
                            _debugLog("Disconnect reason: Invalid shard", DEBUG_LEVEL_ERROR);
                            break;
                        case 4011:
                            _debugLog("Disconnect reason: Sharding required", DEBUG_LEVEL_ERROR);
                            break;
                        case 4012:
                            _debugLog("Disconnect reason: Invalid API version", DEBUG_LEVEL_ERROR);
                            break;
                        case 4013:
                            _debugLog("Disconnect reason: Invalid intent(s)", DEBUG_LEVEL_ERROR);
                            break;
                        case 4014:
                            _debugLog("Disconnect reason: Disallowed intent(s)", DEBUG_LEVEL_ERROR);
                            break;
                        default:
                            _debugLog("Disconnect reason: Unknown code " + String(closeCode), DEBUG_LEVEL_WARNING);
                            break;
                    }
                } else {
                    _debugLog("WebSocket disconnected (no code provided)", DEBUG_LEVEL_WARNING);
                }
                // Reset heartbeat state on disconnect
                _lastHeartbeatAck = 0;
                _heartbeatMissedCount = 0;

                if (_resumeInProgress) {
                    _debugLog("Resume attempt failed before completion, clearing session data", DEBUG_LEVEL_WARNING);
                    _resumeInProgress = false;
                    _sessionId = "";
                    _resumeGatewayUrl = "";
                    _sequenceNumber = -1;
                }
                break;
            }
            case WStype_CONNECTED:
                _wsConnected = true;
                _connectionStartTime = millis();
                _lastHeartbeatAck = millis();
                _heartbeatMissedCount = 0;
                _debugLog("WebSocket connected to Discord gateway", DEBUG_LEVEL_INFO);
                break;
            case WStype_TEXT:
                {
                    if (payload != nullptr && length > 0) {
                        String message = String((char*)payload);
                        _debugLog("Received WebSocket message: " + message.substring(0, min(100, (int)message.length())) + "...", DEBUG_LEVEL_VERBOSE);
                        
                        // Call onRaw callback for every WebSocket message
                        if (_onRaw) {
                            _onRaw(message);
                        }
                        
                        JsonDocument doc;
                        DeserializationError error = deserializeJson(doc, message);
                        if (error) {
                            _debugLog("JSON parse error: " + String(error.c_str()), DEBUG_LEVEL_ERROR);
                            _debugLog("Raw message: " + message.substring(0, min(200, (int)message.length())), DEBUG_LEVEL_VERBOSE);
                        } else {
                            // Check if this is a HELLO message
                            if (doc["op"].as<int>() == OPCODE_HELLO) {
                                _debugLog("Received HELLO message from Discord!", DEBUG_LEVEL_INFO);
                            }
                            _handleWebSocketEvent(doc);
                        }
                    } else {
                        _debugLog("Received empty WebSocket message", DEBUG_LEVEL_WARNING);
                    }
                }
                break;
            case WStype_ERROR:
                _debugLog("WebSocket error occurred", DEBUG_LEVEL_ERROR);
                break;
            default:
                break;
        }
    });
    
    return true;
}

void DiscordAPI::disconnectWebSocket() {
    _webSocket.disconnect();
    _wsConnected = false;
    _wsAuthenticated = false;
}

void DiscordAPI::loop() {
    _webSocket.loop();

    unsigned long now = millis();

    if (!_wsConnected) {
        _handleReconnect();
        return;
    }

    if (_heartbeatInterval > 0 && now - _lastHeartbeat >= _heartbeatInterval) {
        _sendHeartbeat();
    }

    if (!_wsAuthenticated) {
        const unsigned long AUTH_TIMEOUT = 15000;
        if (_connectionStartTime > 0 && now - _connectionStartTime > AUTH_TIMEOUT) {
            _debugLog("Authentication timed out while waiting for READY event", DEBUG_LEVEL_WARNING);
            _handleConnectionTimeout();
            _handleReconnect();
        }
        return;
    }

    static unsigned long lastStabilityCheck = 0;
    if (now - lastStabilityCheck > 10000) { // Check every 10 seconds
        if (!_checkConnectionStability()) {
            _handleConnectionTimeout();
            _handleReconnect();
            return;
        }
        lastStabilityCheck = now;
    }
}

bool DiscordAPI::isWebSocketConnected() {
    return _wsConnected && _wsAuthenticated;
}

void DiscordAPI::resetReconnectionState() {
    _reconnectAttempts = 0;
    _reconnectDelay = 5000;
    _lastReconnectAttempt = 0;
    _debugLog("Reconnection state reset", DEBUG_LEVEL_INFO);
}

void DiscordAPI::resetConnectionState() {
    _lastHeartbeatAck = millis();
    _connectionStartTime = millis();
    _heartbeatMissedCount = 0;
    _resumeInProgress = false;
    _debugLog("Connection state reset", DEBUG_LEVEL_INFO);
}

void DiscordAPI::forceDisconnect() {
    _debugLog("Force disconnecting WebSocket", DEBUG_LEVEL_WARNING);
    _resumeInProgress = false;
    
    if (_wsConnected) {
        _webSocket.disconnect();
        _wsConnected = false;
        _wsAuthenticated = false;
    }
    
    // Reset all connection states
    _lastHeartbeatAck = 0;
    _connectionStartTime = 0;
    _heartbeatMissedCount = 0;
    _sequenceNumber = -1;
    _sessionId = "";
    
    // Reset reconnection state
    _reconnectAttempts = 0;
    _reconnectDelay = 5000;
    _lastReconnectAttempt = 0;
    
    _debugLog("Force disconnect complete", DEBUG_LEVEL_INFO);
}

void DiscordAPI::debugConnectionState() {
    _debugLog("=== Connection State Debug ===", DEBUG_LEVEL_INFO);
    _debugLog("WebSocket Connected: " + String(_wsConnected ? "Yes" : "No"), DEBUG_LEVEL_INFO);
    _debugLog("WebSocket Authenticated: " + String(_wsAuthenticated ? "Yes" : "No"), DEBUG_LEVEL_INFO);
    _debugLog("Heartbeat Interval: " + String(_heartbeatInterval) + "ms", DEBUG_LEVEL_INFO);
    _debugLog("Last Heartbeat: " + String(millis() - _lastHeartbeat) + "ms ago", DEBUG_LEVEL_INFO);
    _debugLog("Last Heartbeat ACK: " + String(millis() - _lastHeartbeatAck) + "ms ago", DEBUG_LEVEL_INFO);
    _debugLog("Sequence Number: " + String(_sequenceNumber), DEBUG_LEVEL_INFO);
    _debugLog("Session ID: " + _sessionId, DEBUG_LEVEL_INFO);
    _debugLog("Resume Gateway URL: " + _resumeGatewayUrl, DEBUG_LEVEL_INFO);
    _debugLog("Reconnect Attempts: " + String(_reconnectAttempts) + "/" + String(_maxReconnectAttempts), DEBUG_LEVEL_INFO);
    _debugLog("Reconnect Delay: " + String(_reconnectDelay) + "ms", DEBUG_LEVEL_INFO);
    _debugLog("Heartbeat Missed Count: " + String(_heartbeatMissedCount), DEBUG_LEVEL_INFO);
    _debugLog("Connection Start Time: " + String(millis() - _connectionStartTime) + "ms ago", DEBUG_LEVEL_INFO);
}

// Gateway intent configuration
void DiscordAPI::setGatewayIntents(uint32_t intents) {
    _gatewayIntents = intents;
    _debugLog("Gateway intents set to " + String((unsigned long)_gatewayIntents), DEBUG_LEVEL_INFO);
}

void DiscordAPI::addGatewayIntent(uint32_t intent) {
    _gatewayIntents |= intent;
    _debugLog("Gateway intents updated (add) => " + String((unsigned long)_gatewayIntents), DEBUG_LEVEL_VERBOSE);
}

void DiscordAPI::removeGatewayIntent(uint32_t intent) {
    _gatewayIntents &= ~intent;
    _debugLog("Gateway intents updated (remove) => " + String((unsigned long)_gatewayIntents), DEBUG_LEVEL_VERBOSE);
}

uint32_t DiscordAPI::getGatewayIntents() const {
    return _gatewayIntents;
}

// Event handlers
void DiscordAPI::onReady(void (*callback)(DiscordUser user)) {
    _onReady = callback;
}

void DiscordAPI::onMessage(void (*callback)(DiscordMessage message)) {
    _onMessage = callback;
}

void DiscordAPI::onGuildCreate(void (*callback)(DiscordGuild guild)) {
    _onGuildCreate = callback;
}

void DiscordAPI::onError(void (*callback)(String error)) {
    _onError = callback;
}

void DiscordAPI::onDebug(void (*callback)(String message, int level)) {
    _onDebug = callback;
}

void DiscordAPI::onRaw(void (*callback)(String rawMessage)) {
    _onRaw = callback;
}

// WebSocket event handling
void DiscordAPI::_handleWebSocketEvent(JsonDocument& doc) {
    if (doc.isNull() || !doc.is<JsonObject>()) {
        _debugLog("Invalid JSON document received", DEBUG_LEVEL_ERROR);
        return;
    }
    
    if (doc["s"].is<int>()) {
        _sequenceNumber = doc["s"].as<int>();
    }

    int op = doc["op"].as<int>();
    String eventType = doc["t"].as<String>();
        
    _debugLog("Processing WebSocket event: OP=" + String(op) + ", Type=" + eventType, DEBUG_LEVEL_VERBOSE);
    
    switch (op) {
        case OPCODE_HELLO:
            if (doc["d"].is<JsonObject>()) {
                _heartbeatInterval = doc["d"]["heartbeat_interval"].as<int>();
                unsigned long now = millis();
                _lastHeartbeat = now;
                _lastHeartbeatAck = now;
                _heartbeatMissedCount = 0;

                if (_heartbeatInterval > 0) {
                    _lastHeartbeat = now - _heartbeatInterval;
                    _debugLog("Scheduling immediate heartbeat after HELLO (interval: " + String(_heartbeatInterval) + "ms)", DEBUG_LEVEL_VERBOSE);
                }

                _debugLog("Received HELLO, heartbeat interval: " + String(_heartbeatInterval) + "ms", DEBUG_LEVEL_INFO);
                _debugLog("Session ID: " + _sessionId, DEBUG_LEVEL_VERBOSE);
                _debugLog("Resume URL: " + _resumeGatewayUrl, DEBUG_LEVEL_VERBOSE);

                bool canResume = (_sessionId.length() > 0 && _resumeGatewayUrl.length() > 0 && _sequenceNumber >= 0);
                if (canResume) {
                    _debugLog("Attempting to resume session after Hello", DEBUG_LEVEL_INFO);
                    _resume();
                } else {
                    if (_sessionId.length() > 0 && _resumeGatewayUrl.length() > 0 && _sequenceNumber < 0) {
                        _debugLog("Stored session data missing sequence number; falling back to IDENTIFY", DEBUG_LEVEL_WARNING);
                    } else {
                        _debugLog("No session info, identifying after Hello", DEBUG_LEVEL_INFO);
                    }
                    _resumeInProgress = false;
                    _identify();
                }
            } else {
                _debugLog("Invalid HELLO message format", DEBUG_LEVEL_ERROR);
                _resumeInProgress = false;
                _identify();
            }
            break;
            
        case OPCODE_HEARTBEAT_ACK:
            _lastHeartbeatAck = millis();
            _heartbeatMissedCount = 0;
            _debugLog("Heartbeat ACK received", DEBUG_LEVEL_VERBOSE);
            break;

        case OPCODE_HEARTBEAT:
            _debugLog("Received heartbeat request from Discord", DEBUG_LEVEL_VERBOSE);
            _sendHeartbeat();
            break;
            
        case OPCODE_DISPATCH:
            if (eventType == EVENT_READY) {
                _debugLog("Received READY event from Discord", DEBUG_LEVEL_INFO);
                if (doc["d"].is<JsonObject>()) {
                    _wsAuthenticated = true;
                    _resumeInProgress = false;
                    _sessionId = doc["d"]["session_id"].as<String>();
                    _resumeGatewayUrl = doc["d"]["resume_gateway_url"].as<String>();
                    _debugLog("Bot ready! Session ID: " + _sessionId, DEBUG_LEVEL_INFO);
                    _debugLog("Resume Gateway URL: " + _resumeGatewayUrl, DEBUG_LEVEL_VERBOSE);
                    _debugLog("WebSocket authentication successful!", DEBUG_LEVEL_INFO);
                    
                    // Reset connection state on successful authentication
                    _lastHeartbeatAck = millis();
                    _connectionStartTime = millis();
                    _heartbeatMissedCount = 0;
                    _reconnectAttempts = 0;
                    
                    if (_onReady && doc["d"]["user"].is<JsonObject>()) {
                        DiscordUser user;
                        _parseUser(doc["d"]["user"], user);
                        _onReady(user);
                    }
                } else {
                    _debugLog("Invalid READY message format", DEBUG_LEVEL_ERROR);
                    // Force disconnect and reconnect on invalid READY
                    _wsAuthenticated = false;
                    _webSocket.disconnect();
                }
            } else if (eventType == EVENT_MESSAGE_CREATE) {
                if (_onMessage && doc["d"].is<JsonObject>()) {
                    DiscordMessage message;
                    _parseMessage(doc["d"], message);
                    _onMessage(message);
                }
            } else if (eventType == EVENT_GUILD_CREATE) {
                if (_onGuildCreate && doc["d"].is<JsonObject>()) {
                    DiscordGuild guild;
                    _parseGuild(doc["d"], guild);
                    _onGuildCreate(guild);
                }
            }
            break;
            
        case OPCODE_INVALID_SESSION:
            _wsAuthenticated = false;
            _resumeInProgress = false;
            _debugLog("Invalid session, retrying identify...", DEBUG_LEVEL_WARNING);
            // Reset session info for fresh identify
            _sessionId = "";
            _resumeGatewayUrl = "";
            delay(1000);
            _identify();
            break;
            
        case OPCODE_RECONNECT:
            _debugLog("Received reconnect command from Discord", DEBUG_LEVEL_WARNING);
            disconnectWebSocket();
            delay(2000); // Longer delay for server-initiated reconnect
            connectWebSocket();
            break;
            
        case OPCODE_RESUMED:
            _debugLog("Connection resumed successfully", DEBUG_LEVEL_INFO);
            _wsAuthenticated = true;
            _resumeInProgress = false;
            _connectionStartTime = millis();
            _lastHeartbeatAck = millis();
            _heartbeatMissedCount = 0;
            break;
            
        default:
            _debugLog("Unknown opcode received: " + String(op), DEBUG_LEVEL_WARNING);
            if (doc["d"].is<JsonObject>()) {
                String dataStr;
                serializeJson(doc["d"], dataStr);
                _debugLog("Data: " + dataStr, DEBUG_LEVEL_VERBOSE);
            }
            break;
    }

}

void DiscordAPI::_sendHeartbeat() {
    if (!_wsConnected) {
        _debugLog("Cannot send heartbeat: WebSocket not connected", DEBUG_LEVEL_WARNING);
        return;
    }
    
    JsonDocument doc;
    doc["op"] = OPCODE_HEARTBEAT;
    if(_sequenceNumber >= 0) {
        doc["d"] = _sequenceNumber;
    }else{
        doc["d"] = nullptr;
    }
    
    String message;
    serializeJson(doc, message);
    
    bool sent = _webSocket.sendTXT(message);
    _lastHeartbeat = millis();
    
    if (sent) {
        _debugLog("Sent heartbeat, sequence: " + String(_sequenceNumber), DEBUG_LEVEL_VERBOSE);
    } else {
        _debugLog("Failed to send heartbeat", DEBUG_LEVEL_ERROR);
        // Don't immediately disconnect on failed heartbeat, let the stability check handle it
    }
}

void DiscordAPI::_identify() {
    _resumeInProgress = false;
    JsonDocument doc;
    doc["op"] = OPCODE_IDENTIFY;
    
    JsonObject d = doc["d"].to<JsonObject>();
    d["token"] = _botToken;
    
    JsonObject properties = d["properties"].to<JsonObject>();
    properties["os"] = "esp32";
    properties["browser"] = "DiscordBot";
    properties["device"] = "esp32";
    
    // d["compress"] = false;
    // d["large_threshold"] = 250;
    
    // Add intents - configurable via setGatewayIntents()
    d["intents"] = _gatewayIntents;

    String message = "";
    serializeJson(doc, message);

    bool sent = _webSocket.sendTXT(message);

    // Debug: Log the identify packet (without token for security)
    String debugMessage = message;
    int tokenStart = debugMessage.indexOf("\"token\":\"");
    if (tokenStart != -1) {
        int tokenEnd = debugMessage.indexOf("\"", tokenStart + 9);
        if (tokenEnd != -1) {
            debugMessage = debugMessage.substring(0, tokenStart + 9) + "***HIDDEN***" + debugMessage.substring(tokenEnd);
        }
    }
    _debugLog("Sending IDENTIFY packet: " + debugMessage, DEBUG_LEVEL_INFO);
    
    if (sent) {
        _debugLog("IDENTIFY packet sent successfully", DEBUG_LEVEL_INFO);
        _debugLog("Waiting for READY event from Discord...", DEBUG_LEVEL_VERBOSE);
    } else {
        _debugLog("Failed to send IDENTIFY packet", DEBUG_LEVEL_ERROR);
    }
}

void DiscordAPI::_resume() {
    if (_sessionId.length() == 0) {
        _debugLog("No session ID, identifying...", DEBUG_LEVEL_WARNING);
        _identify();
        return;
    }
    
    if (_resumeGatewayUrl.length() == 0) {
        _debugLog("No resume gateway URL, identifying...", DEBUG_LEVEL_WARNING);
        _identify();
        return;
    }
    
    if (_sequenceNumber < 0) {
        _debugLog("No sequence number available for resume, identifying...", DEBUG_LEVEL_WARNING);
        _identify();
        return;
    }
    
    _debugLog("Attempting to resume session: " + _sessionId, DEBUG_LEVEL_INFO);
    _resumeInProgress = true;
    
    JsonDocument doc;
    doc["op"] = OPCODE_RESUME;
    
    JsonObject d = doc["d"].to<JsonObject>();
    d["token"] = _botToken;
    d["session_id"] = _sessionId;
    d["seq"] = _sequenceNumber;
    
    String message;
    serializeJson(doc, message);
    _webSocket.sendTXT(message);
    _debugLog("Sent RESUME packet, session: " + _sessionId, DEBUG_LEVEL_INFO);
}

// Parsing methods
void DiscordAPI::_parseUser(JsonObject userObj, DiscordUser& user) {
    if (userObj.isNull()) {
        _debugLog("Invalid user object", DEBUG_LEVEL_ERROR);
        return;
    }
    
    user.id = userObj["id"].as<String>();
    user.username = userObj["username"].as<String>();
    user.discriminator = userObj["discriminator"].as<String>();
    user.global_name = userObj["global_name"].as<String>();
    user.avatar = userObj["avatar"].as<String>();
    user.bot = userObj["bot"].as<bool>();
    user.system = userObj["system"].as<bool>();
    user.mfa_enabled = userObj["mfa_enabled"].as<bool>();
    user.banner = userObj["banner"].as<String>();
    user.accent_color = userObj["accent_color"].as<int>();
    user.locale = userObj["locale"].as<String>();
    user.verified = userObj["verified"].as<bool>();
    user.email = userObj["email"].as<String>();
    user.flags = userObj["flags"].as<int>();
    user.premium_type = userObj["premium_type"].as<int>();
    user.public_flags = userObj["public_flags"].as<int>();
    user.avatar_decoration = userObj["avatar_decoration"].as<String>();
}

void DiscordAPI::_parseMessage(JsonObject messageObj, DiscordMessage& message) {
    if (messageObj.isNull()) {
        _debugLog("Invalid message object", DEBUG_LEVEL_ERROR);
        return;
    }
    
    message.id = messageObj["id"].as<String>();
    message.channel_id = messageObj["channel_id"].as<String>();
    message.guild_id = messageObj["guild_id"].as<String>();
    message.content = messageObj["content"].as<String>();
    message.timestamp = messageObj["timestamp"].as<String>();
    message.edited_timestamp = messageObj["edited_timestamp"].as<String>();
    message.tts = messageObj["tts"].as<bool>();
    message.mention_everyone = messageObj["mention_everyone"].as<bool>();
    message.nonce = messageObj["nonce"].as<String>();
    message.pinned = messageObj["pinned"].as<bool>();
    message.webhook_id = messageObj["webhook_id"].as<String>();
    message.type = messageObj["type"].as<int>();
    message.application_id = messageObj["application_id"].as<String>();
    message.flags = messageObj["flags"].as<int>();
    message.position = messageObj["position"].as<int>();
    
    // Parse author
    if (messageObj["author"].is<JsonObject>()) {
        _parseUser(messageObj["author"], message.author);
    }
    
    // Parse mentions
    if (messageObj["mentions"].is<JsonArray>()) {
        JsonArray mentions = messageObj["mentions"];
        message.mentions_count = mentions.size();
        if (message.mentions_count > 0) {
            message.mentions = new (std::nothrow) String[message.mentions_count];
            if (message.mentions != nullptr) {
                for (int i = 0; i < message.mentions_count; i++) {
                    message.mentions[i] = mentions[i]["id"].as<String>();
                }
            } else {
                _debugLog("Failed to allocate memory for mentions", DEBUG_LEVEL_ERROR);
                message.mentions_count = 0;
            }
        }
    }
}

void DiscordAPI::_parseChannel(JsonObject channelObj, DiscordChannel& channel) {
    if (channelObj.isNull()) {
        _debugLog("Invalid channel object", DEBUG_LEVEL_ERROR);
        return;
    }
    
    channel.id = channelObj["id"].as<String>();
    channel.type = channelObj["type"].as<int>();
    channel.guild_id = channelObj["guild_id"].as<String>();
    channel.position = channelObj["position"].as<int>();
    channel.name = channelObj["name"].as<String>();
    channel.topic = channelObj["topic"].as<String>();
    channel.nsfw = channelObj["nsfw"].as<bool>();
    channel.last_message_id = channelObj["last_message_id"].as<String>();
    channel.bitrate = channelObj["bitrate"].as<int>();
    channel.user_limit = channelObj["user_limit"].as<int>();
    channel.rate_limit_per_user = channelObj["rate_limit_per_user"].as<int>();
    channel.icon = channelObj["icon"].as<String>();
    channel.owner_id = channelObj["owner_id"].as<String>();
    channel.application_id = channelObj["application_id"].as<String>();
    channel.parent_id = channelObj["parent_id"].as<String>();
    channel.last_pin_timestamp = channelObj["last_pin_timestamp"].as<String>();
    channel.rtc_region = channelObj["rtc_region"].as<String>();
    channel.video_quality_mode = channelObj["video_quality_mode"].as<int>();
    channel.message_count = channelObj["message_count"].as<int>();
    channel.member_count = channelObj["member_count"].as<int>();
    channel.permissions = channelObj["permissions"].as<String>();
    channel.flags = channelObj["flags"].as<int>();
    channel.default_auto_archive_duration = channelObj["default_auto_archive_duration"].as<int>();
    channel.default_thread_rate_limit_per_user = channelObj["default_thread_rate_limit_per_user"].as<int>();
}

// Helper function to call debug callback
void DiscordAPI::_debugLog(String message, int level) {
    if (_onDebug) {
        _onDebug(message, level);
    }
}

bool DiscordAPI::_shouldReconnect() {
    if (_wsConnected && _wsAuthenticated) {
        return false; // Already connected and authenticated
    }
    
    if (_reconnectAttempts >= _maxReconnectAttempts) {
        _debugLog("Max reconnection attempts reached (" + String(_maxReconnectAttempts) + ")", DEBUG_LEVEL_ERROR);
        return false;
    }
    
    unsigned long currentTime = millis();
    if (currentTime - _lastReconnectAttempt < _reconnectDelay) {
        return false; // Not enough time has passed
    }
    
    return true;
}

void DiscordAPI::_handleReconnect() {
    if (!_shouldReconnect()) {
        return;
    }
    
    _reconnectAttempts++;
    _lastReconnectAttempt = millis();
    
    _debugLog("Attempting reconnection #" + String(_reconnectAttempts) + "/" + String(_maxReconnectAttempts), DEBUG_LEVEL_WARNING);
    
    // Disconnect first if connected
    if (_wsConnected) {
        _webSocket.disconnect();
        _wsConnected = false;
        _wsAuthenticated = false;
    }

    _resumeInProgress = false;
    
    // Add delay before reconnecting to avoid spam (longer delay for stability)
    delay(2000);
    
    // Try to connect
    if (connectWebSocket()) {
        _debugLog("Reconnection attempt successful", DEBUG_LEVEL_INFO);
        _reconnectAttempts = 0; // Reset on success
        _reconnectDelay = 5000; // Reset delay
        // Identify/Resume will be handled in Hello event
    } else {
        _debugLog("Reconnection attempt failed", DEBUG_LEVEL_WARNING);
        // Exponential backoff: increase delay by 1.5x, max 30 seconds for faster recovery
        _reconnectDelay = min((unsigned long)(_reconnectDelay * 1.5), 30000UL);
        _debugLog("Next reconnection in " + String(_reconnectDelay / 1000) + " seconds", DEBUG_LEVEL_INFO);
    }
}

bool DiscordAPI::_checkConnectionStability() {
    if (!_wsConnected || !_wsAuthenticated) {
        return false;
    }
    
    unsigned long currentTime = millis();
    
    // Only check heartbeat if we have a valid interval
    if (_heartbeatInterval > 0) {
        // Check if we haven't received heartbeat ACK for too long (5x interval for more tolerance)
        if (currentTime - _lastHeartbeatAck > _heartbeatInterval * 5) {
            _heartbeatMissedCount++;
            _debugLog("Heartbeat ACK missed, count: " + String(_heartbeatMissedCount) + "/" + String(_maxHeartbeatMissed), DEBUG_LEVEL_WARNING);
            
            if (_heartbeatMissedCount >= _maxHeartbeatMissed) {
                _debugLog("Too many missed heartbeats, connection unstable", DEBUG_LEVEL_ERROR);
                return false;
            }
        }
    }
    
    // Check if connection has been stable for at least 30 seconds before applying strict checks
    if (currentTime - _connectionStartTime < 30000) {
        return true; // Give connection time to stabilize
    }
    
    return true;
}

void DiscordAPI::_handleConnectionTimeout() {
    _debugLog("Connection timeout detected, forcing disconnect", DEBUG_LEVEL_WARNING);
    _resumeInProgress = false;
    
    if (_wsConnected) {
        _webSocket.disconnect();
        _wsConnected = false;
        _wsAuthenticated = false;
    }
    
    // Reset reconnection state
    _reconnectAttempts = 0;
    _reconnectDelay = 5000;
    _lastReconnectAttempt = 0;
}

void DiscordAPI::_parseGatewayUrl(const String& url, String& hostOut, String& pathOut) {
    if (url.length() == 0) {
        return;
    }

    String working = url;
    working.trim();

    if (working.startsWith("wss://")) {
        working = working.substring(6);
    } else if (working.startsWith("https://")) {
        working = working.substring(8);
    } else if (working.startsWith("ws://")) {
        working = working.substring(5);
    }

    int slashIndex = working.indexOf("/");
    if (slashIndex == -1) {
        working.trim();
        if (working.length() > 0) {
            hostOut = working;
        }
        return;
    }

    String hostPart = working.substring(0, slashIndex);
    String pathPart = working.substring(slashIndex);
    hostPart.trim();
    pathPart.trim();

    if (hostPart.length() > 0) {
        hostOut = hostPart;
    }

    if (pathPart.length() > 0) {
        if (!pathPart.startsWith("/")) {
            pathPart = "/" + pathPart;
        }
        pathOut = pathPart;
    }
}


void DiscordAPI::_parseGuild(JsonObject guildObj, DiscordGuild& guild) {
    if (guildObj.isNull()) {
        _debugLog("Invalid guild object", DEBUG_LEVEL_ERROR);
        return;
    }
    
    guild.id = guildObj["id"].as<String>();
    guild.name = guildObj["name"].as<String>();
    guild.icon = guildObj["icon"].as<String>();
    guild.icon_hash = guildObj["icon_hash"].as<String>();
    guild.splash = guildObj["splash"].as<String>();
    guild.discovery_splash = guildObj["discovery_splash"].as<String>();
    guild.owner = guildObj["owner"].as<bool>();
    guild.owner_id = guildObj["owner_id"].as<String>();
    guild.permissions = guildObj["permissions"].as<String>();
    guild.region = guildObj["region"].as<String>();
    guild.afk_channel_id = guildObj["afk_channel_id"].as<String>();
    guild.afk_timeout = guildObj["afk_timeout"].as<int>();
    guild.widget_enabled = guildObj["widget_enabled"].as<bool>();
    guild.widget_channel_id = guildObj["widget_channel_id"].as<String>();
    guild.verification_level = guildObj["verification_level"].as<int>();
    guild.default_message_notifications = guildObj["default_message_notifications"].as<int>();
    guild.explicit_content_filter = guildObj["explicit_content_filter"].as<int>();
    guild.mfa_level = guildObj["mfa_level"].as<int>();
    guild.application_id = guildObj["application_id"].as<String>();
    guild.system_channel_id = guildObj["system_channel_id"].as<String>();
    guild.system_channel_flags = guildObj["system_channel_flags"].as<int>();
    guild.rules_channel_id = guildObj["rules_channel_id"].as<String>();
    guild.max_presences = guildObj["max_presences"].as<int>();
    guild.max_members = guildObj["max_members"].as<int>();
    guild.vanity_url_code = guildObj["vanity_url_code"].as<String>();
    guild.description = guildObj["description"].as<String>();
    guild.banner = guildObj["banner"].as<String>();
    guild.premium_tier = guildObj["premium_tier"].as<int>();
    guild.premium_subscription_count = guildObj["premium_subscription_count"].as<int>();
    guild.preferred_locale = guildObj["preferred_locale"].as<String>();
    guild.public_updates_channel_id = guildObj["public_updates_channel_id"].as<String>();
    guild.max_video_channel_users = guildObj["max_video_channel_users"].as<int>();
    guild.max_stage_video_channel_users = guildObj["max_stage_video_channel_users"].as<int>();
    guild.nsfw_level = guildObj["nsfw_level"].as<int>();
    guild.premium_progress_bar_enabled = guildObj["premium_progress_bar_enabled"].as<bool>();
    guild.safety_alerts_channel_id = guildObj["safety_alerts_channel_id"].as<String>();
}

// Utility methods
String DiscordAPI::getBotInviteURL(String permissions) {
    if (_clientId.length() == 0) {
        return "";
    }
    return "https://discord.com/api/oauth2/authorize?client_id=" + _clientId + 
           "&permissions=" + permissions + "&scope=bot";
}

String DiscordAPI::formatUserMention(String userId) {
    return "<@" + userId + ">";
}

String DiscordAPI::formatChannelMention(String channelId) {
    return "<#" + channelId + ">";
}

String DiscordAPI::formatRoleMention(String roleId) {
    return "<@&" + roleId + ">";
}

String DiscordAPI::formatEmoji(String name, String id, bool animated) {
    if (animated) {
        return "<a:" + name + ":" + id + ">";
    }
    return "<:" + name + ":" + id + ">";
}

String DiscordAPI::formatTimestamp(String timestamp, String style) {
    return "<t:" + timestamp + ":" + style + ">";
}

String DiscordAPI::formatCodeBlock(String code, String language) {
    return "```" + language + "\n" + code + "\n```";
}

String DiscordAPI::formatInlineCode(String code) {
    return "`" + code + "`";
}

String DiscordAPI::formatBold(String text) {
    return "**" + text + "**";
}

String DiscordAPI::formatItalic(String text) {
    return "*" + text + "*";
}

String DiscordAPI::formatUnderline(String text) {
    return "__" + text + "__";
}

String DiscordAPI::formatStrikethrough(String text) {
    return "~~" + text + "~~";
}

String DiscordAPI::formatSpoiler(String text) {
    return "||" + text + "||";
}

String DiscordAPI::formatQuote(String text) {
    return "> " + text;
}

String DiscordAPI::formatBlockQuote(String text) {
    return ">>> " + text;
}

// Rate limiting
bool DiscordAPI::isRateLimited() {
    if (_rateLimitReset > 0 && millis() < _rateLimitReset) {
        return true;
    }
    return false;
}

int DiscordAPI::getRemainingRequests() {
    return DISCORD_RATE_LIMIT - _requestCount;
}

unsigned long DiscordAPI::getRateLimitReset() {
    return _rateLimitReset;
}

// Error handling
String DiscordAPI::getLastError() {
    return "";
}

void DiscordAPI::clearError() {
    // Clear any error state
}




