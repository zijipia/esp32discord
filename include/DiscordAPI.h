#ifndef DISCORD_API_H
#define DISCORD_API_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>

// Discord API endpoints
#define DISCORD_API_BASE "https://discord.com/api/v10"
#define DISCORD_WS_GATEWAY "wss://gateway.discord.gg/?v=10&encoding=json"

// Discord API limits
#define DISCORD_RATE_LIMIT 50 // requests per second
#define DISCORD_MAX_MESSAGE_LENGTH 2000

// WebSocket opcodes
#define OPCODE_DISPATCH 0
#define OPCODE_HEARTBEAT 1
#define OPCODE_IDENTIFY 2
#define OPCODE_PRESENCE_UPDATE 3
#define OPCODE_VOICE_STATE_UPDATE 4
#define OPCODE_RESUME 6
#define OPCODE_RECONNECT 7
#define OPCODE_REQUEST_GUILD_MEMBERS 8
#define OPCODE_INVALID_SESSION 9
#define OPCODE_HELLO 10
#define OPCODE_HEARTBEAT_ACK 11
#define OPCODE_RESUMED 12

// Event types
#define EVENT_READY "READY"
#define EVENT_MESSAGE_CREATE "MESSAGE_CREATE"
#define EVENT_MESSAGE_UPDATE "MESSAGE_UPDATE"
#define EVENT_MESSAGE_DELETE "MESSAGE_DELETE"
#define EVENT_GUILD_CREATE "GUILD_CREATE"
#define EVENT_GUILD_UPDATE "GUILD_UPDATE"
#define EVENT_GUILD_DELETE "GUILD_DELETE"

// Message types
#define MESSAGE_TYPE_DEFAULT 0
#define MESSAGE_TYPE_RECIPIENT_ADD 1
#define MESSAGE_TYPE_RECIPIENT_REMOVE 2
#define MESSAGE_TYPE_CALL 3
#define MESSAGE_TYPE_CHANNEL_NAME_CHANGE 4
#define MESSAGE_TYPE_CHANNEL_ICON_CHANGE 5
#define MESSAGE_TYPE_CHANNEL_PINNED_MESSAGE 6
#define MESSAGE_TYPE_GUILD_MEMBER_JOIN 7
#define MESSAGE_TYPE_USER_PREMIUM_GUILD_SUBSCRIPTION 8
#define MESSAGE_TYPE_USER_PREMIUM_GUILD_SUBSCRIPTION_TIER_1 9
#define MESSAGE_TYPE_USER_PREMIUM_GUILD_SUBSCRIPTION_TIER_2 10
#define MESSAGE_TYPE_USER_PREMIUM_GUILD_SUBSCRIPTION_TIER_3 11
#define MESSAGE_TYPE_CHANNEL_FOLLOW_ADD 12
#define MESSAGE_TYPE_GUILD_DISCOVERY_DISQUALIFIED 14
#define MESSAGE_TYPE_GUILD_DISCOVERY_REQUALIFIED 15
#define MESSAGE_TYPE_GUILD_DISCOVERY_GRACE_PERIOD_INITIAL_WARNING 16
#define MESSAGE_TYPE_GUILD_DISCOVERY_GRACE_PERIOD_FINAL_WARNING 17
#define MESSAGE_TYPE_THREAD_CREATED 18
#define MESSAGE_TYPE_REPLY 19
#define MESSAGE_TYPE_CHAT_INPUT_COMMAND 20
#define MESSAGE_TYPE_THREAD_STARTER_MESSAGE 21
#define MESSAGE_TYPE_GUILD_INVITE_REMINDER 22
#define MESSAGE_TYPE_CONTEXT_MENU_COMMAND 23
#define MESSAGE_TYPE_AUTO_MODERATION_ACTION 24
#define MESSAGE_TYPE_ROLE_SUBSCRIPTION_PURCHASE 25
#define MESSAGE_TYPE_INTERACTION_PREMIUM_UPSELL 26
#define MESSAGE_TYPE_STAGE_START 27
#define MESSAGE_TYPE_STAGE_END 28
#define MESSAGE_TYPE_STAGE_SPEAKER_ADD 29
#define MESSAGE_TYPE_STAGE_SPEAKER_REMOVE 30
#define MESSAGE_TYPE_STAGE_TOPIC_CHANGE 31
#define MESSAGE_TYPE_GUILD_APPLICATION_PREMIUM_SUBSCRIPTION 32

// Channel types
#define CHANNEL_TYPE_GUILD_TEXT 0
#define CHANNEL_TYPE_DM 1
#define CHANNEL_TYPE_GUILD_VOICE 2
#define CHANNEL_TYPE_GROUP_DM 3
#define CHANNEL_TYPE_GUILD_CATEGORY 4
#define CHANNEL_TYPE_GUILD_ANNOUNCEMENT 5
#define CHANNEL_TYPE_ANNOUNCEMENT_THREAD 10
#define CHANNEL_TYPE_PUBLIC_THREAD 11
#define CHANNEL_TYPE_PRIVATE_THREAD 12
#define CHANNEL_TYPE_STAGE_VOICE 13
#define CHANNEL_TYPE_DIRECTORY 14
#define CHANNEL_TYPE_FORUM 15

// User flags
#define USER_FLAG_STAFF 1
#define USER_FLAG_PARTNER 2
#define USER_FLAG_HYPESQUAD 4
#define USER_FLAG_BUG_HUNTER_LEVEL_1 8
#define USER_FLAG_HYPESQUAD_ONLINE_HOUSE_1 64
#define USER_FLAG_HYPESQUAD_ONLINE_HOUSE_2 128
#define USER_FLAG_HYPESQUAD_ONLINE_HOUSE_3 256
#define USER_FLAG_PREMIUM_EARLY_SUPPORTER 512
#define USER_FLAG_TEAM_PSEUDO_USER 1024
#define USER_FLAG_BUG_HUNTER_LEVEL_2 16384
#define USER_FLAG_VERIFIED_BOT 65536
#define USER_FLAG_VERIFIED_DEVELOPER 131072
#define USER_FLAG_CERTIFIED_MODERATOR 262144
#define USER_FLAG_BOT_HTTP_INTERACTIONS 524288
#define USER_FLAG_ACTIVE_DEVELOPER 1073741824

// Presence status
#define PRESENCE_ONLINE "online"
#define PRESENCE_IDLE "idle"
#define PRESENCE_DND "dnd"
#define PRESENCE_INVISIBLE "invisible"
#define PRESENCE_OFFLINE "offline"

// Activity types
#define ACTIVITY_TYPE_PLAYING 0
#define ACTIVITY_TYPE_STREAMING 1
#define ACTIVITY_TYPE_LISTENING 2
#define ACTIVITY_TYPE_WATCHING 3
#define ACTIVITY_TYPE_CUSTOM 4
#define ACTIVITY_TYPE_COMPETING 5

// Debug levels
#define DEBUG_LEVEL_ERROR 0
#define DEBUG_LEVEL_WARNING 1
#define DEBUG_LEVEL_INFO 2
#define DEBUG_LEVEL_VERBOSE 3

// Discord API Response structure
struct DiscordResponse
{
    int statusCode;
    String body;
    bool success;
    String error;
};

// Discord User structure
struct DiscordUser
{
    String id;
    String username;
    String discriminator;
    String global_name;
    String avatar;
    bool bot;
    bool system;
    bool mfa_enabled;
    String banner;
    int accent_color;
    String locale;
    bool verified;
    String email;
    int flags;
    int premium_type;
    int public_flags;
    String avatar_decoration;
};

// Discord Message structure
struct DiscordMessage
{
    String id;
    String channel_id;
    String guild_id;
    DiscordUser author;
    String content;
    String timestamp;
    String edited_timestamp;
    bool tts;
    bool mention_everyone;
    String *mentions;
    int mentions_count;
    String *mention_roles;
    int mention_roles_count;
    String *mention_channels;
    int mention_channels_count;
    String *attachments;
    int attachments_count;
    String *embeds;
    int embeds_count;
    String *reactions;
    int reactions_count;
    String nonce;
    bool pinned;
    String webhook_id;
    int type;
    String *activity;
    String *application;
    String application_id;
    String *message_reference;
    int flags;
    String *referenced_message;
    String *interaction;
    String *thread;
    String *components;
    int components_count;
    String *sticker_items;
    int sticker_items_count;
    String *stickers;
    int stickers_count;
    int position;
    String *role_subscription_data;
};

// Discord Channel structure
struct DiscordChannel
{
    String id;
    int type;
    String guild_id;
    int position;
    String *permission_overwrites;
    int permission_overwrites_count;
    String name;
    String topic;
    bool nsfw;
    String last_message_id;
    int bitrate;
    int user_limit;
    int rate_limit_per_user;
    String *recipients;
    int recipients_count;
    String icon;
    String owner_id;
    String application_id;
    String parent_id;
    String last_pin_timestamp;
    String rtc_region;
    int video_quality_mode;
    int message_count;
    int member_count;
    String *thread_metadata;
    String *member;
    int default_auto_archive_duration;
    String permissions;
    int flags;
    String *total_message_sent;
    String *available_tags;
    int available_tags_count;
    String *applied_tags;
    int applied_tags_count;
    String *default_reaction_emoji;
    int default_thread_rate_limit_per_user;
    String *default_sort_order;
    String *default_forum_layout;
};

// Discord Guild structure
struct DiscordGuild
{
    String id;
    String name;
    String icon;
    String icon_hash;
    String splash;
    String discovery_splash;
    bool owner;
    String owner_id;
    String permissions;
    String region;
    String afk_channel_id;
    int afk_timeout;
    bool widget_enabled;
    String widget_channel_id;
    int verification_level;
    int default_message_notifications;
    int explicit_content_filter;
    String *roles;
    int roles_count;
    String *emojis;
    int emojis_count;
    String *features;
    int features_count;
    int mfa_level;
    String application_id;
    String system_channel_id;
    int system_channel_flags;
    String rules_channel_id;
    int max_presences;
    int max_members;
    String vanity_url_code;
    String description;
    String banner;
    int premium_tier;
    int premium_subscription_count;
    String preferred_locale;
    String public_updates_channel_id;
    int max_video_channel_users;
    int max_stage_video_channel_users;
    String *approximate_member_count;
    String *approximate_presence_count;
    String *welcome_screen;
    int nsfw_level;
    String *stickers;
    int stickers_count;
    bool premium_progress_bar_enabled;
    String safety_alerts_channel_id;
};

// Discord API Client class
class DiscordAPI
{
private:
    String _botToken;
    String _clientId;
    String _clientSecret;
    String _redirectUri;
    WiFiClientSecure _wifiClient;
    HTTPClient _httpClient;
    WebSocketsClient _webSocket;

    // Rate limiting
    unsigned long _lastRequestTime;
    int _requestCount;
    unsigned long _rateLimitReset;

    // WebSocket state
    bool _wsConnected;
    bool _wsAuthenticated;
    int _heartbeatInterval;
    unsigned long _lastHeartbeat;
    int _sequenceNumber;
    String _sessionId;
    String _resumeGatewayUrl;

    // Reconnection management
    unsigned long _lastReconnectAttempt;
    int _reconnectAttempts;
    int _maxReconnectAttempts;
    unsigned long _reconnectDelay;

    // Connection stability
    unsigned long _lastHeartbeatAck;
    unsigned long _connectionStartTime;
    int _heartbeatMissedCount;
    int _maxHeartbeatMissed;

    // Event callbacks
    void (*_onReady)(DiscordUser user);
    void (*_onMessage)(DiscordMessage message);
    void (*_onGuildCreate)(DiscordGuild guild);
    void (*_onError)(String error);
    void (*_onDebug)(String message, int level);
    void (*_onRaw)(String rawMessage);

    // Internal methods
    String _getAuthHeader();
    DiscordResponse _makeRequest(String method, String endpoint, String body = "");
    void _handleWebSocketEvent(JsonDocument &doc);
    void _sendHeartbeat();
    void _identify();
    void _resume();
    void _parseUser(JsonObject userObj, DiscordUser &user);
    void _parseMessage(JsonObject messageObj, DiscordMessage &message);
    void _parseChannel(JsonObject channelObj, DiscordChannel &channel);
    void _parseGuild(JsonObject guildObj, DiscordGuild &guild);
    void _debugLog(String message, int level);
    bool _shouldReconnect();
    void _handleReconnect();
    bool _checkConnectionStability();
    void _handleConnectionTimeout();

public:
    // Constructor
    DiscordAPI();

    // Destructor
    ~DiscordAPI();

    // Authentication methods
    bool setBotToken(String token);
    bool setOAuth2Credentials(String clientId, String clientSecret, String redirectUri);
    String getOAuth2URL(String scope = "identify");
    String exchangeCodeForToken(String code);
    bool testBotToken();

    // REST API methods
    DiscordUser getCurrentUser();
    DiscordUser getUser(String userId);
    DiscordGuild getGuild(String guildId);
    DiscordChannel getChannel(String channelId);
    DiscordMessage getMessage(String channelId, String messageId);
    DiscordMessage *getChannelMessages(String channelId, int limit = 50, String before = "", String after = "", String around = "");
    DiscordResponse sendMessage(String channelId, String content, bool tts = false);
    DiscordResponse editMessage(String channelId, String messageId, String content);
    DiscordResponse deleteMessage(String channelId, String messageId);
    DiscordResponse addReaction(String channelId, String messageId, String emoji);
    DiscordResponse removeReaction(String channelId, String messageId, String emoji, String userId = "@me");
    DiscordResponse removeAllReactions(String channelId, String messageId);
    DiscordResponse removeAllReactionsForEmoji(String channelId, String messageId, String emoji);

    // WebSocket methods
    bool connectWebSocket();
    void disconnectWebSocket();
    void loop();
    bool isWebSocketConnected();
    void resetReconnectionState();
    void resetConnectionState();
    void forceDisconnect();
    void debugConnectionState();

    // Event handlers
    void onReady(void (*callback)(DiscordUser user));
    void onMessage(void (*callback)(DiscordMessage message));
    void onGuildCreate(void (*callback)(DiscordGuild guild));
    void onError(void (*callback)(String error));
    void onDebug(void (*callback)(String message, int level));
    void onRaw(void (*callback)(String rawMessage));

    // Utility methods
    String getBotInviteURL(String permissions = "0");
    String formatUserMention(String userId);
    String formatChannelMention(String channelId);
    String formatRoleMention(String roleId);
    String formatEmoji(String name, String id, bool animated = false);
    String formatTimestamp(String timestamp, String style = "f");
    String formatCodeBlock(String code, String language = "");
    String formatInlineCode(String code);
    String formatBold(String text);
    String formatItalic(String text);
    String formatUnderline(String text);
    String formatStrikethrough(String text);
    String formatSpoiler(String text);
    String formatQuote(String text);
    String formatBlockQuote(String text);

    // Rate limiting
    bool isRateLimited();
    int getRemainingRequests();
    unsigned long getRateLimitReset();

    // Error handling
    String getLastError();
    void clearError();
};

#endif // DISCORD_API_H
