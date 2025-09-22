#include <Arduino.h>
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
            prefix = "⚪ [DEBUG]";
            break;
    }
    Serial.println(prefix + " " + message);
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
    
    // Configure Discord Bot
    if (!discord.setBotToken(botToken)) {
        Serial.println("❌ Error: Cannot set bot token!");
        return;
    }
    
    // Set up callbacks
    discord.onReady(onBotReady);
    discord.onMessage(onMessageReceived);
    discord.onError(onError);
    discord.onDebug(onDebug);
    
    // Connect WebSocket
    if (discord.connectWebSocket()) {
        Serial.println("✅ Discord WebSocket connected!");
    } else {
        Serial.println("❌ Discord WebSocket connection failed!");
    }
    
    Serial.println("🎉 Bot is ready to operate!");
}

void loop() {
    // Process WebSocket events
    discord.loop();
    
    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("⚠️ WiFi disconnected, reconnecting...");
        Serial.println("Signal strength: " + String(WiFi.RSSI()) + " dBm");
        WiFi.begin(ssid, password);
        delay(5000);
    }
    
    // Check Discord connection
    if (!discord.isWebSocketConnected()) {
        Serial.println("⚠️ Discord disconnected, reconnecting...");
        Serial.println("Free heap: " + String(ESP.getFreeHeap()) + " bytes");
        discord.connectWebSocket();
        delay(5000);
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