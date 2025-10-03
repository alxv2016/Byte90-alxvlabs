/**
 * @file wifi_module.cpp
 * @brief Implementation of WiFi management module
 *
 * Provides functionality for WiFi connection management, Access Point operations,
 * ESP-NOW configuration, and web server handling for the BYTE-90 device.
 * 
 * This module handles:
 * - WiFi station mode connection management
 * - Access Point mode for firmware updates
 * - ESP-NOW communication setup
 * - WiFi event handling and status monitoring
 * - Web server operations for device configuration
 */

#include "wifi_module.h"
#include "preferences_module.h"
#include "wifi_common.h"
#include "flash_module.h"
#include "ota_module.h"
#include "wifi_endpoints.h"
#include "espnow_module.h"
#include <esp_log.h>
#include <esp_wifi.h>
#include <stdarg.h>
 
//==============================================================================
// DEBUG SYSTEM
//==============================================================================

// Debug control - set to true to enable debug logging
static bool wifiDebugEnabled = false;

/**
 * @brief Centralized debug logging function for WiFi operations
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
static void wifiDebug(const char* format, ...) {
  if (!wifiDebugEnabled) {
    return;
  }
  
  va_list args;
  va_start(args, format);
  esp_log_writev(ESP_LOG_INFO, "::WIFI_MODULE [NEW]::", format, args);
  va_end(args);
}

/**
 * @brief Enable or disable debug logging for WiFi operations
 * @param enabled true to enable debug logging, false to disable
 * 
 * @example
 * // Enable debug logging
 * setWifiDebug(true);
 * 
 * // Disable debug logging  
 * setWifiDebug(false);
 */
void setWifiDebug(bool enabled) {
  wifiDebugEnabled = enabled;
  if (enabled) {
    ESP_LOGI("::WIFI_MODULE [NEW]::", "WiFi debug logging enabled");
  } else {
    ESP_LOGI("::WIFI_MODULE [NEW]::", "WiFi debug logging disabled");
  }
}

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

static const char *TAG = "::WIFI_MODULE [NEW]::";

// AP mode configuration
static const char* AP_SSID = "ESP32-Config";
static const char* AP_PASSWORD = "12345678";

// Connection management
static const int MAX_CONNECTION_ATTEMPTS = 3;
static const unsigned long CONNECTION_TIMEOUT = 30000; // 30 seconds

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================
 
bool wifiNetworkConnected = false;
char wifi_ssid[WIFI_SSID_MAX_LEN] = "";
char wifi_password[WIFI_PASSWORD_MAX_LEN] = "";

// Connection management state
static int connectionAttempts = 0;
static unsigned long connectionStartTime = 0;
static bool connectionInProgress = false;
 
//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================
 
/**
 * @brief Converts WiFi disconnect reason code to human-readable string
 * @param reason WiFi disconnect reason code
 * @return String representation of the disconnect reason
 * 
 * Maps WiFi disconnect reason codes to descriptive strings for logging
 * and debugging purposes.
 */
const char* getDisconnectReasonString(uint8_t reason) {
    switch (reason) {
        case WIFI_REASON_UNSPECIFIED: return "Unspecified";
        case WIFI_REASON_AUTH_EXPIRE: return "Auth expired";
        case WIFI_REASON_AUTH_LEAVE: return "Auth leave";
        case WIFI_REASON_ASSOC_EXPIRE: return "Assoc expired";
        case WIFI_REASON_ASSOC_TOOMANY: return "Too many assoc";
        case WIFI_REASON_NOT_AUTHED: return "Not authenticated";
        case WIFI_REASON_NOT_ASSOCED: return "Not associated";
        case WIFI_REASON_ASSOC_LEAVE: return "Assoc leave";
        case WIFI_REASON_ASSOC_NOT_AUTHED: return "Assoc not authed";
        case WIFI_REASON_DISASSOC_PWRCAP_BAD: return "Bad power capability";
        case WIFI_REASON_DISASSOC_SUPCHAN_BAD: return "Bad supported channels";
        case WIFI_REASON_IE_INVALID: return "Invalid IE";
        case WIFI_REASON_MIC_FAILURE: return "MIC failure";
        case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT: return "4-way handshake timeout";
        case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT: return "Group key update timeout";
        case WIFI_REASON_IE_IN_4WAY_DIFFERS: return "IE in 4-way differs";
        case WIFI_REASON_GROUP_CIPHER_INVALID: return "Invalid group cipher";
        case WIFI_REASON_PAIRWISE_CIPHER_INVALID: return "Invalid pairwise cipher";
        case WIFI_REASON_AKMP_INVALID: return "Invalid AKMP";
        case WIFI_REASON_UNSUPP_RSN_IE_VERSION: return "Unsupported RSN IE version";
        case WIFI_REASON_INVALID_RSN_IE_CAP: return "Invalid RSN IE capability";
        case WIFI_REASON_802_1X_AUTH_FAILED: return "802.1X auth failed";
        case WIFI_REASON_CIPHER_SUITE_REJECTED: return "Cipher suite rejected";
        case WIFI_REASON_BEACON_TIMEOUT: return "Beacon timeout";
        case WIFI_REASON_NO_AP_FOUND: return "No AP found";
        case WIFI_REASON_AUTH_FAIL: return "Auth failed";
        case WIFI_REASON_ASSOC_FAIL: return "Assoc failed";
        case WIFI_REASON_HANDSHAKE_TIMEOUT: return "Handshake timeout";
        case WIFI_REASON_CONNECTION_FAIL: return "Connection failed";
        case WIFI_REASON_AP_TSF_RESET: return "AP TSF reset";
        default: return "Unknown reason";
    }
}

/**
 * @brief Prints current WiFi status information to serial output
 * 
 * Displays comprehensive WiFi status including connection state,
 * network information, and Access Point details if applicable.
 */
void printWiFiStatus() {
    wifiDebug("=== WiFi Status ===");
    wifiDebug("Mode: %d", WiFi.getMode());
    wifiDebug("Status: %s", wifiNetworkConnected ? "Connected" : "Disconnected");

    if (wifiNetworkConnected) {
        wifiDebug("SSID: %s", WiFi.SSID().c_str());
        wifiDebug("IP: %s", WiFi.localIP().toString().c_str());
        wifiDebug("RSSI: %d dBm", WiFi.RSSI());
    }

    if (WiFi.getMode() == WIFI_MODE_AP) {
        wifiDebug("AP SSID: %s", WiFi.softAPSSID().c_str());
        wifiDebug("AP IP: %s", WiFi.softAPIP().toString().c_str());
        wifiDebug("AP Clients: %d", WiFi.softAPgetStationNum());
    }

    wifiDebug("==================");
}

//==============================================================================
// WIFI EVENT HANDLERS
//==============================================================================

/**
 * @brief Event handler for WiFi station start event
 * @param event WiFi event type
 * @param info WiFi event information structure
 * 
 * Handles WiFi station initialization and triggers connection attempts
 * when in WiFi mode and not already connected.
 */
void onWiFiStationStart(WiFiEvent_t event, WiFiEventInfo_t info) {
    wifiDebug("[WIFI STA START EVENT]::WiFi station started and ready");
    
    // If we're in WIFI_MODE and not connected, attempt connection
    if (!wifiNetworkConnected && !connectionInProgress) {
        wifiDebug("Station ready - attempting connection");
        delay(200);
        connectToWiFiFromPreferences();
    }
}

/**
 * @brief Event handler for WiFi station connected event
 * @param event WiFi event type
 * @param info WiFi event information structure
 * 
 * Handles successful WiFi connection and resets connection state.
 */
void onWiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    wifiDebug("Connected to WiFi network: %s", wifi_ssid);
    connectionInProgress = false;
    connectionAttempts = 0;
}

/**
 * @brief Event handler for WiFi station disconnected event
 * @param event WiFi event type
 * @param info WiFi event information structure
 * 
 * Handles WiFi disconnection events and logs disconnect reasons.
 */
void onWiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    uint8_t reason = info.wifi_sta_disconnected.reason;
    ESP_LOGW(TAG, "Disconnected from WiFi network - Reason: %s (%d)", 
             getDisconnectReasonString(reason), reason);

    wifiNetworkConnected = false;
    connectionInProgress = false;
    // Don't auto-reconnect - let state machine handle reconnection logic
}

/**
 * @brief Event handler for WiFi station got IP address event
 * @param event WiFi event type
 * @param info WiFi event information structure
 * 
 * Handles successful IP address assignment and logs network configuration.
 */
void onWiFiStationGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
    wifiDebug("WiFi connected successfully");
    wifiDebug("IP address: %s", WiFi.localIP().toString().c_str());
    wifiDebug("Subnet mask: %s", WiFi.subnetMask().toString().c_str());
    wifiDebug("Gateway: %s", WiFi.gatewayIP().toString().c_str());
    wifiDebug("DNS: %s", WiFi.dnsIP().toString().c_str());
    wifiDebug("RSSI: %d dBm", WiFi.RSSI());

    wifiNetworkConnected = true;
    connectionInProgress = false;
    connectionAttempts = 0;
}

/**
 * @brief Event handler for WiFi Access Point started event
 * @param event WiFi event type
 * @param info WiFi event information structure
 * 
 * Handles successful Access Point startup and logs AP configuration.
 */
void onWiFiAPStarted(WiFiEvent_t event, WiFiEventInfo_t info) {
    wifiDebug("Access Point started");
    wifiDebug("AP SSID: %s", WiFi.softAPSSID().c_str());
    wifiDebug("AP IP: %s", WiFi.softAPIP().toString().c_str());
}

/**
 * @brief Event handler for WiFi Access Point stopped event
 * @param event WiFi event type
 * @param info WiFi event information structure
 * 
 * Handles Access Point shutdown events.
 */
void onWiFiAPStopped(WiFiEvent_t event, WiFiEventInfo_t info) {
    wifiDebug("Access Point stopped");
 }
 
//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================
 
/**
 * @brief Initializes the WiFi manager and sets up event handlers
 * 
 * Configures WiFi for manual control, registers event handlers, and loads
 * stored WiFi credentials from preferences. Sets up the WiFi system for
 * both station mode and access point operations.
 */
void initWiFiManager() {
    wifiDebug("Initializing WiFi Manager...");

    // Configure WiFi for manual control
    WiFi.persistent(false);        // Don't save credentials to flash
    WiFi.setAutoConnect(false);    // No automatic connection
    WiFi.setAutoReconnect(false);  // Manual reconnection control
    WiFi.mode(WIFI_MODE_NULL);     // Start completely disabled
    WiFi.setTxPower(WIFI_POWER_19_5dBm); 

    // Register WiFi event handlers
    WiFi.onEvent(onWiFiStationStart, ARDUINO_EVENT_WIFI_STA_START);
    WiFi.onEvent(onWiFiStationConnected, ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(onWiFiStationDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.onEvent(onWiFiStationGotIP, ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(onWiFiAPStarted, ARDUINO_EVENT_WIFI_AP_START);
    WiFi.onEvent(onWiFiAPStopped, ARDUINO_EVENT_WIFI_AP_STOP);

    // Load WiFi credentials from preferences (preferences module handles defaults)
    if (!loadWiFiCredentials(wifi_ssid, wifi_password)) {
        ESP_LOGW(TAG, "No WiFi credentials found - will need configuration");
        wifi_ssid[0] = '\0';
        wifi_password[0] = '\0';
     } else {
        wifiDebug("Loaded WiFi credentials - SSID: %s", wifi_ssid);
    }

    wifiDebug("WiFi Manager initialized");
}

/**
 * @brief Enables WiFi in station mode
 * 
 * Initializes WiFi hardware in station mode for connecting to existing
 * WiFi networks. This function prepares the WiFi system for network
 * connection operations.
 */
void enableWiFi() {
    if (WiFi.getMode() != WIFI_MODE_NULL) {
        ESP_LOGD(TAG, "WiFi already enabled");
        return;
    }

    wifiDebug("Enabling WiFi hardware...");
    WiFi.mode(WIFI_MODE_STA);
    delay(100); // Allow hardware to initialize

    wifiDebug("WiFi hardware enabled");
}

/**
 * @brief Enables WiFi in ESP-NOW compatible mode
 * 
 * Configures WiFi hardware for ESP-NOW communication. Sets up WiFi in
 * station mode without connecting to any network, optimized for ESP-NOW
 * peer-to-peer communication.
 */
void enableESPNowWiFi() {
    wifiDebug("Enabling WiFi hardware for ESP-NOW...");

    // ESP-NOW works in STA mode but doesn't connect to any AP
    WiFi.mode(WIFI_MODE_STA);
    delay(500);
    WiFi.disconnect(false);

    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    delay(100);

    wifiDebug("WiFi channel set to 1 for ESP-NOW");

    if (!initializeESPNOW()) {
        ESP_LOGE(TAG, "Failed to initialize ESP-NOW");
        return;
    }

    if (!restartCommunication()) {
        ESP_LOGE(TAG, "Failed to start ESP-NOW communication");
        return;
    }

    uint8_t primary;
    wifi_second_chan_t second;
    esp_wifi_get_channel(&primary, &second);
    
    wifiDebug("Verified WiFi channel: %d", primary);
    wifiDebug("WiFi hardware enabled for ESP-NOW on channel 1");
    wifiDebug("MAC Address: %s", WiFi.macAddress().c_str());
}

/**
 * @brief Disables WiFi completely
 * 
 * Completely shuts down WiFi hardware for power savings. Resets all
 * connection state and prepares the system for low-power operation.
 */
void disableWiFi() {
    if (WiFi.getMode() == WIFI_MODE_NULL) {
        ESP_LOGD(TAG, "WiFi already disabled");
        return;
    }

    wifiDebug("Disabling WiFi hardware...");

    // Disable hardware completely
    WiFi.mode(WIFI_MODE_NULL);

    // Reset connection state
    wifiNetworkConnected = false;
    connectionInProgress = false;
    connectionAttempts = 0;

    wifiDebug("WiFi hardware disabled - power savings active");
}

/**
 * @brief Connects to a WiFi network with specified credentials
 * @param ssid WiFi network SSID to connect to
 * @param password WiFi network password
 * @return true if connection successful, false otherwise
 * 
 * Attempts to connect to the specified WiFi network with retry logic.
 * Validates input parameters and manages connection state.
 */
bool connectToWiFi(const char* ssid, const char* password) {
    // Validate input parameters
    if (!ssid || strlen(ssid) == 0) {
        ESP_LOGE(TAG, "Invalid SSID provided - cannot connect");
        return false;
    }
    
    if (strlen(ssid) >= WIFI_SSID_MAX_LEN) {
        ESP_LOGE(TAG, "SSID too long (%d chars, max %d) - cannot connect", 
                 strlen(ssid), WIFI_SSID_MAX_LEN - 1);
        return false;
    }
    
    if (password && strlen(password) >= WIFI_PASSWORD_MAX_LEN) {
        ESP_LOGE(TAG, "Password too long (%d chars, max %d) - cannot connect", 
                 strlen(password), WIFI_PASSWORD_MAX_LEN - 1);
        return false;
    }

    // Update local credentials safely
    strncpy(wifi_ssid, ssid, WIFI_SSID_MAX_LEN - 1);
    wifi_ssid[WIFI_SSID_MAX_LEN - 1] = '\0';
    
    if (password) {
        strncpy(wifi_password, password, WIFI_PASSWORD_MAX_LEN - 1);
        wifi_password[WIFI_PASSWORD_MAX_LEN - 1] = '\0';
         } else {
        wifi_password[0] = '\0';
    }

    if (wifiNetworkConnected) {
        // Check if we're already connected to the requested network
        if (WiFi.SSID().equals(ssid)) {
            ESP_LOGD(TAG, "Already connected to requested WiFi network");
         return true;
     } else {
            wifiDebug("Disconnecting from current network to connect to new one");
            disconnectFromWiFi();
        }
    }

    if (connectionInProgress) {
        // Check for timeout
        if (millis() - connectionStartTime > CONNECTION_TIMEOUT) {
            ESP_LOGW(TAG, "Connection timeout - attempt %d/%d", 
                     connectionAttempts + 1, MAX_CONNECTION_ATTEMPTS);
            connectionInProgress = false;
            connectionAttempts++;

            if (connectionAttempts >= MAX_CONNECTION_ATTEMPTS) {
                ESP_LOGE(TAG, "Maximum connection attempts reached - giving up");
                connectionAttempts = 0;
                return false;
            }
        } else {
            // Still waiting for connection
         return false;
     }
 }
 
    // Ensure WiFi is enabled
    enableWiFi();

    // Start new connection attempt
    wifiDebug("Connecting to WiFi - SSID: %s (attempt %d/%d)", 
             wifi_ssid, connectionAttempts + 1, MAX_CONNECTION_ATTEMPTS);

    WiFi.disconnect(false);
    delay(100);

    WiFi.begin(wifi_ssid, wifi_password);
    connectionInProgress = true;
    connectionStartTime = millis();

    return false; // Connection in progress
}

/**
 * @brief Connects to WiFi using credentials stored in preferences
 * @return true if connection successful, false otherwise
 * 
 * Attempts to connect using previously saved WiFi credentials.
 * Returns false if no credentials are available.
 */
bool connectToWiFiFromPreferences() {
  // Load fresh credentials from preferences and connect
  char currentSSID[WIFI_SSID_MAX_LEN];
  char currentPassword[WIFI_PASSWORD_MAX_LEN];
  
  if (!loadWiFiCredentials(currentSSID, currentPassword)) {
      ESP_LOGW(TAG, "No WiFi credentials found - cannot connect");
      return false;
  }
  
  return connectToWiFi(currentSSID, currentPassword);
}

/**
 * @brief Disconnects from the current WiFi network
 * 
 * Disconnects from the currently connected WiFi network and resets
 * connection state variables.
 */
void disconnectFromWiFi() {
    if (!wifiNetworkConnected && !connectionInProgress) {
        ESP_LOGD(TAG, "WiFi not connected");
        return;
    }

    wifiDebug("Disconnecting from WiFi");
    WiFi.disconnect();

    wifiNetworkConnected = false;
    connectionInProgress = false;
    connectionAttempts = 0;
}

static void setupUpdateModeServices() {
    if (!getFSStatus()) {
        ESP_LOGE(TAG, "Failed to initialize file system for update mode");
        return;
    }

    // Setup web endpoints using the new endpoints module
    if (!initWiFiEndpoints()) {
        ESP_LOGE(TAG, "Failed to initialize WiFi endpoints");
        return;
    }

    if (!initOTA()) {
        ESP_LOGE(TAG, "Failed to initialize OTA for update mode");
        return;
    }
}

/**
 * @brief Starts WiFi Access Point mode
 * 
 * Configures and starts WiFi Access Point mode for device configuration
 * and firmware updates. Automatically selects appropriate mode (AP-only
 * or AP+STA) based on current connection status.
 */
void startWiFiAP() {
    wifiDebug("Starting WiFi Access Point - SSID: %s", WIFI_AP_SSID);

    // Check current WiFi status and choose appropriate mode
    wifi_mode_t targetMode = WIFI_MODE_AP;  // Default to AP only

    // NOTE: when we previous checked if wifi is connected plus if we are in STA mode this would sometimes default to AP mode only
    if (isWifiNetworkConnected()) {
        // We're in STA mode and connected - use APSTA to keep connection
        targetMode = WIFI_MODE_APSTA;
        wifiDebug("WiFi connected - starting AP+STA mode to preserve connection");
    } else {
        // Not connected or not in STA mode - disconnect and use AP only
        wifiDebug("WiFi not connected - starting AP-only mode");
        disconnectFromWiFi();  // Only disconnect if we're going AP-only
    }

    // Configure WiFi mode
    WiFi.mode(targetMode);
    delay(100);

    // Configure AP network settings
    IPAddress apIP = getDefaultAPIP();
    IPAddress apGateway = getDefaultAPGateway();
    IPAddress apMask = getDefaultAPNetworkMask();
    WiFi.softAPConfig(apIP, apGateway, apMask);

    // Start the Access Point
    bool success = WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, 1, 0, 4);

    if (success) {
        wifiDebug("Access Point started successfully");
        wifiDebug("WiFi Mode: %s", targetMode == WIFI_MODE_APSTA ? "AP+STA" : "AP");
        wifiDebug("AP IP address: %s", WiFi.softAPIP().toString().c_str());

        if (targetMode == WIFI_MODE_APSTA) {
            wifiDebug("STA IP address: %s", WiFi.localIP().toString().c_str());
            wifiDebug("Connected to: %s", WiFi.SSID().c_str());
        }

        setupUpdateModeServices();
    } else {
        ESP_LOGE(TAG, "Failed to start Access Point");
    }
}

/**
 * @brief Stops WiFi Access Point mode
 * @param restart If true, restarts WiFi after stopping AP mode
 * 
 * Stops the WiFi Access Point and optionally restarts the device.
 * Cleans up web server endpoints and resets WiFi configuration.
 */
void stopWiFiAP(bool restart) {
    if (WiFi.getMode() != WIFI_MODE_AP) {
        ESP_LOGD(TAG, "Access Point not running");
        return;
    }

    wifiDebug("Stopping WiFi Access Point");
    stopWiFiEndpoints();
    WiFi.softAPdisconnect(true);
    delay(100);
    if (restart) {
        wifiDebug("Restarting device after stopping WiFi Access Point");
        ESP.restart();
     } else {
        WiFi.mode(WIFI_MODE_STA);
    }
}

/**
 * @brief Checks if WiFi network is currently connected
 * @return true if connected to WiFi network, false otherwise
 * 
 * Returns the current connection status of the WiFi network.
 */
bool isWifiNetworkConnected() {
    // Always check actual WiFi connection status
    wifi_mode_t mode = WiFi.getMode();
    wl_status_t status = WiFi.status();
    
    // We're connected if status is WL_CONNECTED and we're in STA or APSTA mode
    bool actuallyConnected = (status == WL_CONNECTED) && 
                            (mode == WIFI_MODE_STA || mode == WIFI_MODE_APSTA);
    
       // DEBUG LOGGING
    ESP_LOGE(TAG, "=== isWifiNetworkConnected DEBUG ===");
    ESP_LOGE(TAG, "WiFi.getMode() = %d (NULL=0, STA=1, AP=2, APSTA=3)", mode);
    ESP_LOGE(TAG, "WiFi.status() = %d (WL_CONNECTED=3)", status);
    ESP_LOGE(TAG, "actuallyConnected = %s", actuallyConnected ? "TRUE" : "FALSE");
    ESP_LOGE(TAG, "wifiNetworkConnected flag before = %s", wifiNetworkConnected ? "TRUE" : "FALSE");
    
    // Sync our flag with the actual state
    wifiNetworkConnected = actuallyConnected;
    
    ESP_LOGE(TAG, "wifiNetworkConnected flag after = %s", wifiNetworkConnected ? "TRUE" : "FALSE");
    ESP_LOGE(TAG, "===================================");
    
    return actuallyConnected;
}

/**
 * @brief Handles web server requests and responses
 * 
 * Processes incoming web server requests for device configuration
 * and firmware updates. Manages client connections and request routing.
 */
void handleWebServer() {

    getWiFiWebServer().handleClient();
    // Check connected clients every 30 seconds
    static unsigned long lastClientCheck = 0;
    static int lastClientCount = -1; // Track previous count to detect changes
  
    if (millis() - lastClientCheck > 30000) {
      lastClientCheck = millis();
      // Only log if we're in AP mode (UPDATE_MODE or when AP is active)
      if (WiFi.getMode() == WIFI_MODE_AP || WiFi.getMode() == WIFI_MODE_APSTA) {
        int currentClientCount = WiFi.softAPgetStationNum();
  
        // Log when client count changes
        if (currentClientCount != lastClientCount) {
          if (currentClientCount > lastClientCount) {
            wifiDebug("Client connected - Total clients: %d",
                     currentClientCount);
          } else if (currentClientCount < lastClientCount) {
            wifiDebug("Client disconnected - Total clients: %d",
                     currentClientCount);
          }
          lastClientCount = currentClientCount;
        }
  
        // Also log periodically even if no change (every 5 minutes)
        static unsigned long lastPeriodicLog = 0;
        if (millis() - lastPeriodicLog > 300000) { // 5 minutes
          lastPeriodicLog = millis();
          wifiDebug("AP Status - Connected clients: %d, IP: %s",
                   currentClientCount, WiFi.softAPIP().toString().c_str());
        }
      }
    }
 }