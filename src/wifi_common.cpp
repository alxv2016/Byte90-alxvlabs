/**
 * @file wifi_common.cpp
 * @brief Implementation of shared WiFi system utilities
 */

#include "wifi_common.h"
#include "esp_log.h"

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

/**
 * @brief Convert encryption type to human-readable string
 * @param encType WiFi encryption type
 * @return String description of security type
 */
String getNetworkSecurityType(wifi_auth_mode_t encType) {
    switch (encType) {
        case WIFI_AUTH_OPEN:
            return "Open";
        case WIFI_AUTH_WEP:
            return "WEP";
        case WIFI_AUTH_WPA_PSK:
            return "WPA";
        case WIFI_AUTH_WPA2_PSK:
            return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK:
            return "WPA/WPA2";
        case WIFI_AUTH_WPA2_ENTERPRISE:
            return "WPA2 Enterprise";
        case WIFI_AUTH_WPA3_PSK:
            return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK:
            return "WPA2/WPA3";
        case WIFI_AUTH_WAPI_PSK:
            return "WAPI";
        default:
            return "Unknown";
    }
}

/**
 * @brief Convert RSSI value to a human-readable signal strength description
 * @param rssi RSSI value from WiFi
 * @return String description of signal strength
 */
String getNetworkSignalStrength(int rssi) {
  if (rssi == 0) {
    return "NOT_CONNECTED";
  } else if (rssi >= -30) {
    return "Excellent";
  } else if (rssi >= -50) {
    return "Good";
  } else if (rssi >= -70) {
    return "Fair";
  } else if (rssi >= -90) {
    return "Poor";
  } else {
    return "Very Poor";
  }
}

/**
 * @brief Get default Access Point IP address
 * @return IPAddress for the default AP configuration
 */
IPAddress getDefaultAPIP() {
  return IPAddress(AP_LOCAL_IP_1, AP_LOCAL_IP_2, AP_LOCAL_IP_3, AP_LOCAL_IP_4);
}

/**
 * @brief Get default Access Point gateway
 * @return IPAddress for the default AP gateway
 */
IPAddress getDefaultAPGateway() {
  return IPAddress(AP_GATEWAY_1, AP_GATEWAY_2, AP_GATEWAY_3, AP_GATEWAY_4);
}

/**
 * @brief Get default Access Point network mask
 * @return IPAddress for the default AP network mask
 */
IPAddress getDefaultAPNetworkMask() {
  return IPAddress(AP_NETWORK_MASK_1, AP_NETWORK_MASK_2, AP_NETWORK_MASK_3,
                   AP_NETWORK_MASK_4);
}

/**
 * @brief Get current WiFi status information
 * @return String representation of current WiFi status
 */
String getCurrentWiFiStatus() {
    wl_status_t status = WiFi.status();
    
    switch (status) {
        case WL_CONNECTED:
            return "CONNECTED";
        case WL_DISCONNECTED:
            return "DISCONNECTED";
        case WL_IDLE_STATUS:
            return "IDLE";
        case WL_NO_SSID_AVAIL:
            return "NO SSID";
        case WL_SCAN_COMPLETED:
            return "SCANNING";
        case WL_CONNECT_FAILED:
            return "FAILED";
        case WL_CONNECTION_LOST:
            return "LOST";
        default:
            return "--";
    }
}

/**
 * @brief Get current WiFi network name
 * @return String with current network SSID or "--" if not connected
 */
String getCurrentWiFiNetwork() {
    if (WiFi.status() == WL_CONNECTED) {
        String ssid = WiFi.SSID();
        
        if (ssid.length() > 0 && ssid != "null") {
            return ssid;
        } else {
            return "--";
        }
    } else {
        return "--";
    }
}

/**
 * @brief Get current WiFi signal strength
 * @return String with signal strength description or "--" if not connected
 */
String getCurrentWiFiSignalStrength() {
    if (WiFi.status() == WL_CONNECTED) {
        int rssi = WiFi.RSSI();
        
        if (rssi != 0 && rssi > -100) { // Valid RSSI value (not 0 and reasonable range)
            return getNetworkSignalStrength(rssi);
        } else {
            return "--";
        }
    } else {
        return "--";
    }
}

/**
 * @brief Get current WiFi IP address
 * @return String with current IP address or "--" if not connected
 */
String getCurrentWiFiIP() {
    if (WiFi.status() == WL_CONNECTED) {
        IPAddress ip = WiFi.localIP();
        if (ip.toString() != "0.0.0.0") { // Valid IP address
            return ip.toString();
        } else {
            return "--";
        }
    } else {
        return "--";
    }
}
