#include <Arduino.h>
#include "DiscordAPI.h"

// Cấu hình WiFi
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Cấu hình Discord Bot
const char* botToken = "YOUR_BOT_TOKEN";
const char* channelId = "YOUR_CHANNEL_ID";

// Tạo instance của Discord API
DiscordAPI discord;

// Hàm callback khi bot sẵn sàng
void onBotReady(DiscordUser user) {
    Serial.println("Bot đã sẵn sàng!");
    Serial.println("Tên bot: " + user.username);
    Serial.println("ID bot: " + user.id);
    
    // Gửi tin nhắn chào mừng
    DiscordResponse response = discord.sendMessage(channelId, "🤖 Bot ESP32 đã kết nối thành công!");
    if (response.success) {
        Serial.println("Đã gửi tin nhắn chào mừng!");
    } else {
        Serial.println("Lỗi gửi tin nhắn: " + response.error);
    }
}

// Hàm callback khi nhận tin nhắn mới
void onMessageReceived(DiscordMessage message) {
    Serial.println("Nhận tin nhắn từ: " + message.author.username);
    Serial.println("Nội dung: " + message.content);
    
    // Phản hồi tin nhắn
    if (message.content.startsWith("!ping")) {
        DiscordResponse response = discord.sendMessage(message.channel_id, "🏓 Pong! Bot đang hoạt động bình thường.");
        if (response.success) {
            Serial.println("Đã phản hồi lệnh ping!");
        }
    }
    else if (message.content.startsWith("!time")) {
        String currentTime = String(millis() / 1000) + " giây";
        DiscordResponse response = discord.sendMessage(message.channel_id, "⏰ Thời gian hoạt động: " + currentTime);
        if (response.success) {
            Serial.println("Đã gửi thời gian hoạt động!");
        }
    }
    else if (message.content.startsWith("!help")) {
        String helpText = "📋 **Các lệnh có sẵn:**\n";
        helpText += "`!ping` - Kiểm tra bot\n";
        helpText += "`!time` - Xem thời gian hoạt động\n";
        helpText += "`!help` - Hiển thị trợ giúp\n";
        helpText += "`!status` - Trạng thái hệ thống\n";
        
        DiscordResponse response = discord.sendMessage(message.channel_id, helpText);
        if (response.success) {
            Serial.println("Đã gửi trợ giúp!");
        }
    }
    else if (message.content.startsWith("!status")) {
        String statusText = "📊 **Trạng thái hệ thống:**\n";
        statusText += "• WiFi: " + String(WiFi.isConnected() ? "✅ Kết nối" : "❌ Mất kết nối") + "\n";
        statusText += "• Discord: " + String(discord.isWebSocketConnected() ? "✅ Kết nối" : "❌ Mất kết nối") + "\n";
        statusText += "• RAM tự do: " + String(ESP.getFreeHeap()) + " bytes\n";
        statusText += "• Uptime: " + String(millis() / 1000) + " giây";
        
        DiscordResponse response = discord.sendMessage(message.channel_id, statusText);
        if (response.success) {
            Serial.println("Đã gửi trạng thái hệ thống!");
        }
    }
}

// Hàm callback khi có lỗi
void onError(String error) {
    Serial.println("Lỗi Discord: " + error);
}

void setup() {
    Serial.begin(115200);
    Serial.println("🚀 Khởi động ESP32 Discord Bot...");
    
    // Kết nối WiFi
    WiFi.begin(ssid, password);
    Serial.print("Đang kết nối WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("✅ Đã kết nối WiFi!");
    Serial.println("IP: " + WiFi.localIP().toString());
    
    // Cấu hình Discord Bot
    if (!discord.setBotToken(botToken)) {
        Serial.println("❌ Lỗi: Không thể thiết lập bot token!");
        return;
    }
    
    // Thiết lập các callback
    discord.onReady(onBotReady);
    discord.onMessage(onMessageReceived);
    discord.onError(onError);
    
    // Kết nối WebSocket
    if (discord.connectWebSocket()) {
        Serial.println("✅ Đã kết nối Discord WebSocket!");
    } else {
        Serial.println("❌ Lỗi kết nối Discord WebSocket!");
    }
    
    Serial.println("🎉 Bot đã sẵn sàng hoạt động!");
}

void loop() {
    // Xử lý WebSocket events
    discord.loop();
    
    // Kiểm tra kết nối WiFi
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("⚠️ Mất kết nối WiFi, đang thử kết nối lại...");
        WiFi.begin(ssid, password);
        delay(5000);
    }
    
    // Kiểm tra kết nối Discord
    if (!discord.isWebSocketConnected()) {
        Serial.println("⚠️ Mất kết nối Discord, đang thử kết nối lại...");
        discord.connectWebSocket();
        delay(5000);
    }
    
    // Gửi tin nhắn định kỳ (mỗi 5 phút)
    static unsigned long lastMessageTime = 0;
    if (millis() - lastMessageTime > 300000) { // 5 phút
        lastMessageTime = millis();
        
        String statusMessage = "💓 Bot vẫn đang hoạt động! Uptime: " + String(millis() / 1000) + " giây";
        DiscordResponse response = discord.sendMessage(channelId, statusMessage);
        if (response.success) {
            Serial.println("Đã gửi tin nhắn trạng thái định kỳ!");
        }
    }
    
    delay(100);
}