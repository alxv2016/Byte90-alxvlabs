/**
 * @file emotes_module.h
 * @brief Header for animated and static emote definitions
 * 
 * This module provides definitions for all GIF animations and static images
 * used for the device's emotional expression system. It includes different
 * categories of emotes for various device modes and states.
 */

 #ifndef EMOTES_MODULE_H
 #define EMOTES_MODULE_H
 
 #include "common.h"
 
 //------------------------------------------------------------------------------
 // Common Emotes
 //------------------------------------------------------------------------------
 /** @brief GIF Animation Emotes for ALL variations MAC, PC, and BYTE-90 default
  * Animations are optimized GIFs for the ESP32's SPIFFS filesystem
  * GIFs are 128x128 pixels and 16FPS
  */
 #define ANGRY_EMOTE "/gifs/angry.gif"
 #define CRASH01_EMOTE "/gifs/crash_01.gif"
 #define CRASH02_EMOTE "/gifs/crash_02.gif"
 #define CRASH03_EMOTE "/gifs/crash_03.gif"
 #define CRY_EMOTE "/gifs/cry.gif"
 #define DIZZY_EMOTE "/gifs/dizzy.gif"
 #define DOUBTFUL_EMOTE "/gifs/doubtful.gif"
 #define IDLE_EMOTE "/gifs/idle.gif"
 #define LOOK_DOWN_EMOTE "/gifs/look_down.gif"
 #define LOOK_LEFT_RIGHT_EMOTE "/gifs/look_left_right.gif"
 #define LOOK_UP_EMOTE "/gifs/look_up.gif"
 #define REST_EMOTE "/gifs/rest.gif"
 #define SCAN_EMOTE "/gifs/scan.gif"
 #define SHOCK_EMOTE "/gifs/shock.gif"
 #define SLEEP01_EMOTE "/gifs/sleep_01.gif"
 #define SLEEP02_EMOTE "/gifs/sleep_02.gif"
 #define SLEEP03_EMOTE "/gifs/sleep_03.gif"
 #define STARTUP_EMOTE "/gifs/startup.gif"
 #define TALK_EMOTE "/gifs/talk.gif"
 #define TAP_EMOTE "/gifs/tap.gif"
 #define WINK_EMOTE "/gifs/wink.gif"
 #define ZONED_EMOTE "/gifs/zoned.gif"
 #define PIXEL_EMOTE "/gifs/pixelated.gif"
 #define GLEE_EMOTE "/gifs/glee.gif"
 #define BLINK_EMOTE "/gifs/blink.gif"
 #define EXCITED_EMOTE "/gifs/excited.gif"
 #define UWU_EMOTE "/gifs/uwu.gif"
 #define HEARTS_EMOTE "/gifs/hearts.gif"
 #define STARTLED_EMOTE "/gifs/startled.gif"
 
 //------------------------------------------------------------------------------
 // BYTE-90 Specific Emotes
 //------------------------------------------------------------------------------
 /** @brief Special Emotes for BYTE-90 only */
 #define WHISTLE_EMOTE "/gifs/whistle.gif"
 #define MISCHIEF_EMOTE "/gifs/mischief.gif"
 #define HUMSUP_EMOTE "/gifs/humsup.gif"
 #define WINK_02_EMOTE "/gifs/wink_02.gif"
 
 //------------------------------------------------------------------------------
 // ESPNOW Communication Emotes
 //------------------------------------------------------------------------------
 /** @brief Special Emotes for ESPNOW communication mode */
 #define COMS_AGREED_EMOTE "/gifs/coms_agreed.gif"
 #define COMS_CONNECT_EMOTE "/gifs/coms_connect.gif"
 #define COMS_DISCONNECT_EMOTE "/gifs/coms_disconnect.gif"
 #define COMS_DISAGREE_EMOTE "/gifs/coms_disagree.gif"
 #define COMS_HELLO_EMOTE "/gifs/coms_hello.gif"
 #define COMS_LAUGH_EMOTE "/gifs/coms_laugh.gif"
 #define COMS_SHOCK_EMOTE "/gifs/coms_shock.gif"
 #define COMS_TALK_01_EMOTE "/gifs/coms_talk_01.gif"
 #define COMS_TALK_02_EMOTE "/gifs/coms_talk_02.gif"
 #define COMS_TALK_03_EMOTE "/gifs/coms_talk_03.gif"
 #define COMS_WINK_EMOTE "/gifs/coms_wink.gif"
 #define COMS_YELL_EMOTE "/gifs/coms_yell.gif"
 #define COMS_ZONED_EMOTE "/gifs/coms_zoned.gif"
 #define COMS_IDLE_EMOTE "/gifs/coms_idle.gif"
 
 //==============================================================================
 // STATIC IMAGES AND EXTERNAL DECLARATIONS
 //==============================================================================
 
 /** @brief External declaration for device mode */
 extern const char* deviceMode;
 
 /** 
  * @brief Static images are converted to C arrays using the RGB565 image converter
  * RGB565 image converter: https://marlinfw.org/tools/rgb565/converter.html
  */
 /** @brief BYTE Static crash image */
 extern const uint16_t BYTE_CRASH_STATIC[];
 /** @brief Static crash image */
 extern const uint16_t CRASH_STATIC[];
 /** @brief BYTE mode static image */
 extern const uint16_t BYTE_STATIC[];
 /** @brief MAC mode static image */
 extern const uint16_t MAC_STATIC[];
 /** @brief PC mode static image */
 extern const uint16_t PC_STATIC[];
 /** @brief Updater mode static image */
 extern const uint16_t UPDATER_STATIC[];
  /** @brief Startup static image */
 extern const uint16_t STARTUP_STATIC[];
   /** @brief WIFI ON ICON */
 extern const uint8_t WIFI_CONNECT_ICON[];
   /** @brief WIFI OFF ICON */
 extern const uint8_t WIFI_DISCONNECT_ICON[];
 /** @brief BYTE90 LOGO */
 extern const uint8_t BYTE90_LOGO[];

 #endif /* EMOTES_MODULE_H */