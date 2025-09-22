#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "DiscordAPI.h"

// WiFi Configuration
const char* ssid = "Ziji";
const char* password = "1335555777777";

// Discord Bot Configuration
const char* botToken = "MTMxNjAxOTI1NDU5OTg4MDcwNA.GhaFMp.eoOxun8vVd-9WdAzmmVfPVzq8iXNBxxmubuo9E";
const char* channelId = "1007597358579716106";

// Create Discord API instance
DiscordAPI discord;

// Callback function when bot is ready
void onBotReady(DiscordUser user) {
    Serial.println("Bot is ready!");
    Serial.println("Bot name: " + user.username);
    Serial.println("Bot ID: " + user.id);
    
    // Reset connection state on successful connection
    discord.resetReconnectionState();
    discord.resetConnectionState();
    
    // Send welcome message
    DiscordResponse response = discord.sendMessage(channelId, "🤖 ESP32 Bot connected successfully!");
    if (response.success) {
        Serial.println("Welcome message sent!");
    } else {
        Serial.println("Error sending message: " + response.error);
    }
}

// Callback function when receiving new message
void onMessageReceived(DiscordMessage message) {
    Serial.println("Received message from: " + message.author.username);
    Serial.println("Content: " + message.content);
    
    // Respond to messages
    if (message.content.startsWith("!ping")) {
        DiscordResponse response = discord.sendMessage(message.channel_id, "🏓 Pong! Bot is working normally.");
        if (response.success) {
            Serial.println("Responded to ping command!");
        }
    }
    else if (message.content.startsWith("!time")) {
        String currentTime = String(millis() / 1000) + " seconds";
        DiscordResponse response = discord.sendMessage(message.channel_id, "⏰ Uptime: " + currentTime);
        if (response.success) {
            Serial.println("Sent uptime!");
        }
    }
    else if (message.content.startsWith("!help")) {
        String helpText = "📋 **Available commands:**\n";
        helpText += "`!ping` - Check bot\n";
        helpText += "`!time` - View uptime\n";
        helpText += "`!help` - Show help\n";
        helpText += "`!status` - System status\n";
        helpText += "`!debug` - Debug connection state\n";
        helpText += "`!reset` - Reset connection\n";
        
        DiscordResponse response = discord.sendMessage(message.channel_id, helpText);
        if (response.success) {
            Serial.println("Sent help!");
        }
    }
    else if (message.content.startsWith("!status")) {
        String statusText = "📊 **System Status:**\n";
        statusText += "• WiFi: " + String(WiFi.isConnected() ? "✅ Connected" : "❌ Disconnected") + "\n";
        statusText += "• Discord: " + String(discord.isWebSocketConnected() ? "✅ Connected" : "❌ Disconnected") + "\n";
        statusText += "• Free RAM: " + String(ESP.getFreeHeap()) + " bytes\n";
        statusText += "• Uptime: " + String(millis() / 1000) + " seconds";
        
        DiscordResponse response = discord.sendMessage(message.channel_id, statusText);
        if (response.success) {
            Serial.println("Sent system status!");
        }
    }
    else if (message.content.startsWith("!debug")) {
        discord.debugConnectionState();
        DiscordResponse response = discord.sendMessage(message.channel_id, "🔍 Debug info sent to console");
        if (response.success) {
            Serial.println("Sent debug info!");
        }
    }
    else if (message.content.startsWith("!reset")) {
        discord.forceDisconnect();
        DiscordResponse response = discord.sendMessage(message.channel_id, "🔄 Connection reset initiated");
        if (response.success) {
            Serial.println("Sent reset command!");
        }
    }
}

// Callback function when error occurs
void onError(String error) {
    Serial.println("❌ Discord Error: " + error);
}

// Debug callback function
void onDebug(String message, int level) {
    String prefix = "";
    switch(level) {
        case 0: // ERROR
            prefix = "🔴 [ERROR]";
            break;
        case 1: // WARNING
            prefix = "🟡 [WARNING]";
            break;
        case 2: // INFO
            prefix = "🔵 [INFO]";
            break;
        case 3: // VERBOSE
            prefix = "🟢 [VERBOSE]";
            break;
        default:
            prefix = "⚪ [DEBUG-" + String(level) + "]";
            break;
    }
    Serial.println(prefix + " " + message);
}

// Raw WebSocket message callback function
void onRawMessage(String rawMessage) {
    Serial.println("📨 Raw WS Message: " + rawMessage);
    
    // Check if this is a HELLO message
    if (rawMessage.indexOf("\"op\":10") != -1) {
        Serial.println("🎉 HELLO message received from Discord!");
    }
    
    // Check if this is a READY message
    if (rawMessage.indexOf("\"t\":\"READY\"") != -1) {
        Serial.println("🎉 READY message received from Discord!");
    }
    
    // You can parse the raw message here for custom handling
    // For example, you might want to log all messages to a file
    // or implement custom message filtering
}

void setup() {
    Serial.begin(115200);
    Serial.println("🚀 Starting ESP32 Discord Bot...");
    
    // Connect to WiFi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("✅ WiFi connected!");
    Serial.println("IP: " + WiFi.localIP().toString());
    Serial.println("Signal strength: " + String(WiFi.RSSI()) + " dBm");
    
    // Set up callbacks FIRST before any Discord operations
    discord.onReady(onBotReady);
    discord.onMessage(onMessageReceived);
    discord.onError(onError);
    discord.onDebug(onDebug);
    discord.onRaw(onRawMessage);
    
    // Test debug levels
    Serial.println("🧪 Testing debug levels...");
    onDebug("Test VERBOSE message", 3);
    
    // Configure Discord Bot
    Serial.println("🔧 Setting bot token...");
    if (!discord.setBotToken(botToken)) {
        Serial.println("❌ Error: Cannot set bot token!");
        return;
    }
    Serial.println("✅ Bot token set successfully!");
    
    // Skip internet test, go straight to Discord API test
    
    // Test bot token
    if (!discord.testBotToken()) {
        Serial.println("❌ Bot token test failed!");
        return;
    }
    Serial.println("✅ Bot token valid!");
    
    // Connect WebSocket
    if (discord.connectWebSocket()) {
        Serial.println("✅ WebSocket connected!");
    } else {
        Serial.println("❌ WebSocket connection failed!");
    }
}

void loop() {
    discord.loop();
    
    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("⚠️ WiFi disconnected, reconnecting...");
        Serial.println("Signal strength: " + String(WiFi.RSSI()) + " dBm");
        WiFi.begin(ssid, password);
        delay(5000);
    }
    
    // Discord reconnection is now handled automatically by the library
    // Just monitor the connection status with reduced frequency
    static unsigned long lastStatusCheck = 0;
    static int disconnectCount = 0;
    
    if (millis() - lastStatusCheck > 60000) { // Check every 60 seconds
        if (!discord.isWebSocketConnected()) {
            disconnectCount++;
            Serial.println("⚠️ Discord disconnected, library will handle reconnection");
            Serial.println("Disconnect count: " + String(disconnectCount));
            Serial.println("Free heap: " + String(ESP.getFreeHeap()) + " bytes");
            Serial.println("WiFi RSSI: " + String(WiFi.RSSI()) + " dBm");
            
            // If too many disconnects, force reset
            if (disconnectCount > 5) { // Reduced threshold
                Serial.println("🔄 Too many disconnects, forcing reset...");
                discord.forceDisconnect();
                disconnectCount = 0;
            }
        } else {
            // Connection is stable
            Serial.println("✅ Discord connection stable");
            Serial.println("Free heap: " + String(ESP.getFreeHeap()) + " bytes");
            disconnectCount = 0; // Reset counter on stable connection
        }
        
        lastStatusCheck = millis();
    }
    
    // Send periodic message (every 5 minutes)
    static unsigned long lastMessageTime = 0;
    if (millis() - lastMessageTime > 300000) { // 5 minutes
        lastMessageTime = millis();
        
        String statusMessage = "💓 Bot is still running! Uptime: " + String(millis() / 1000) + " seconds";
        DiscordResponse response = discord.sendMessage(channelId, statusMessage);
        if (response.success) {
            Serial.println("Sent periodic status message!");
        }
    }
    
    delay(100);
}