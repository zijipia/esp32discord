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
    _sequenceNumber = 0;
    _sessionId = "";
    _lastRequestTime = 0;
    _requestCount = 0;
    _rateLimitReset = 0;
    _onReady = nullptr;
    _onMessage = nullptr;
    _onGuildCreate = nullptr;
    _onError = nullptr;
    _onDebug = nullptr;
}

// Authentication methods
bool DiscordAPI::setBotToken(String token) {
    if (token.length() == 0) {
        _debugLog("Invalid bot token (empty)", DEBUG_LEVEL_ERROR);
        return false;
    }
    _botToken = token;
    _debugLog("Bot token set successfully", DEBUG_LEVEL_INFO);
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
    _httpClient.begin(_wifiClient, url);
    
    if (_getAuthHeader().length() > 0) {
        _httpClient.addHeader("Authorization", _getAuthHeader());
    }
    _httpClient.addHeader("Content-Type", "application/json");
    _httpClient.addHeader("User-Agent", "DiscordBot (ESP32, 1.0.0)");
    
    int httpResponseCode = 0;
    if (method == "GET") {
        httpResponseCode = _httpClient.GET();
    } else if (method == "POST") {
        httpResponseCode = _httpClient.POST(body);
    } else if (method == "PUT") {
        httpResponseCode = _httpClient.PUT(body);
    } else if (method == "PATCH") {
        httpResponseCode = _httpClient.PATCH(body);
    } else if (method == "DELETE") {
        httpResponseCode = _httpClient.sendRequest("DELETE");
    }
    
    response.statusCode = httpResponseCode;
    response.body = _httpClient.getString();
    _httpClient.end();
    
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
        deserializeJson(doc, response.body);
        
        if (doc.is<JsonArray>()) {
            JsonArray messages = doc.as<JsonArray>();
            DiscordMessage* messageArray = new DiscordMessage[messages.size()];
            
            for (int i = 0; i < messages.size(); i++) {
                _parseMessage(messages[i].as<JsonObject>(), messageArray[i]);
            }
            
            return messageArray;
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
    
    _webSocket.begin("gateway.discord.gg", 443, "/?v=10&encoding=json", "wss");
    _webSocket.onEvent([this](WStype_t type, uint8_t* payload, size_t length) {
        switch (type) {
            case WStype_DISCONNECTED:
                _wsConnected = false;
                _wsAuthenticated = false;
                _debugLog("WebSocket disconnected", DEBUG_LEVEL_WARNING);
                break;
            case WStype_CONNECTED:
                _wsConnected = true;
                _debugLog("WebSocket connected", DEBUG_LEVEL_INFO);
                break;
            case WStype_TEXT:
                {
                    String message = String((char*)payload);
                    _debugLog("Received WebSocket message: " + message.substring(0, min(100, (int)message.length())) + "...", DEBUG_LEVEL_VERBOSE);
                    JsonDocument doc;
                    deserializeJson(doc, message);
                    _handleWebSocketEvent(doc);
                }
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
    
    if (_wsConnected && _wsAuthenticated && _heartbeatInterval > 0) {
        if (millis() - _lastHeartbeat >= _heartbeatInterval) {
            _sendHeartbeat();
        }
    }
}

bool DiscordAPI::isWebSocketConnected() {
    return _wsConnected && _wsAuthenticated;
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

// WebSocket event handling
void DiscordAPI::_handleWebSocketEvent(JsonDocument& doc) {
    int op = doc["op"];
    String eventType = doc["t"];
    
    _debugLog("Processing WebSocket event: OP=" + String(op) + ", Type=" + eventType, DEBUG_LEVEL_VERBOSE);
    
    switch (op) {
        case OPCODE_HELLO:
            _heartbeatInterval = doc["d"]["heartbeat_interval"];
            _lastHeartbeat = millis();
            _debugLog("Received HELLO, heartbeat interval: " + String(_heartbeatInterval) + "ms", DEBUG_LEVEL_INFO);
            _identify();
            break;
            
        case OPCODE_HEARTBEAT_ACK:
            _debugLog("Heartbeat ACK received", DEBUG_LEVEL_VERBOSE);
            break;
            
        case OPCODE_DISPATCH:
            if (eventType == EVENT_READY) {
                _wsAuthenticated = true;
                _sessionId = doc["d"]["session_id"].as<String>();
                _debugLog("Bot ready! Session ID: " + _sessionId, DEBUG_LEVEL_INFO);
                
                if (_onReady) {
                    DiscordUser user;
                    _parseUser(doc["d"]["user"], user);
                    _onReady(user);
                }
            } else if (eventType == EVENT_MESSAGE_CREATE) {
                if (_onMessage) {
                    DiscordMessage message;
                    _parseMessage(doc["d"], message);
                    _onMessage(message);
                }
            } else if (eventType == EVENT_GUILD_CREATE) {
                if (_onGuildCreate) {
                    DiscordGuild guild;
                    _parseGuild(doc["d"], guild);
                    _onGuildCreate(guild);
                }
            }
            break;
            
        case OPCODE_INVALID_SESSION:
            _wsAuthenticated = false;
            _debugLog("Invalid session, retrying identify...", DEBUG_LEVEL_WARNING);
            delay(1000);
            _identify();
            break;
            
        case OPCODE_RECONNECT:
            _debugLog("Received reconnect command from Discord", DEBUG_LEVEL_WARNING);
            disconnectWebSocket();
            delay(1000);
            connectWebSocket();
            break;
    }
    
    if (doc["s"].is<int>()) {
        _sequenceNumber = doc["s"];
    }
}

void DiscordAPI::_sendHeartbeat() {
    JsonDocument doc;
    doc["op"] = OPCODE_HEARTBEAT;
    doc["d"] = _sequenceNumber;
    
    String message;
    serializeJson(doc, message);
    _webSocket.sendTXT(message);
    _lastHeartbeat = millis();
    _debugLog("Sent heartbeat, sequence: " + String(_sequenceNumber), DEBUG_LEVEL_VERBOSE);
}

void DiscordAPI::_identify() {
    JsonDocument doc;
    doc["op"] = OPCODE_IDENTIFY;
    
    JsonObject d = doc["d"].to<JsonObject>();
    d["token"] = _botToken;
    
    JsonObject properties = d["properties"].to<JsonObject>();
    properties["os"] = "ESP32";
    properties["browser"] = "DiscordBot";
    properties["device"] = "ESP32";
    
    d["compress"] = false;
    d["large_threshold"] = 250;
    
    String message;
    serializeJson(doc, message);
    _webSocket.sendTXT(message);
    _debugLog("Sent IDENTIFY packet", DEBUG_LEVEL_INFO);
}

void DiscordAPI::_resume() {
    if (_sessionId.length() == 0) {
        _debugLog("No session ID, identifying...", DEBUG_LEVEL_WARNING);
        _identify();
        return;
    }
    
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
        message.mentions = new String[message.mentions_count];
        for (int i = 0; i < message.mentions_count; i++) {
            message.mentions[i] = mentions[i]["id"].as<String>();
        }
    }
}

void DiscordAPI::_parseChannel(JsonObject channelObj, DiscordChannel& channel) {
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

void DiscordAPI::_parseGuild(JsonObject guildObj, DiscordGuild& guild) {
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
