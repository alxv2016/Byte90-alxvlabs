/**
 * @file soundsfx_communication.cpp
 * @brief Implementation of communication and vocal sound effects for retro computer audio
 *
 * Provides retro computer-style vocal and communication sound effects including
 * talking, alternative talking, laughing, angry, and shock sounds for the BYTE-90 device.
 * 
 * This module handles:
 * - Talking sound effects with speech-like modulation
 * - Alternative talking sounds for variety
 * - Laughing effects with rhythmic patterns
 * - Angry sounds with harsh, low-frequency characteristics
 * - Shock sounds with sharp, high-pitched characteristics
 * - Audio state validation and volume control
 */

 #include "soundsfx_communication.h"
 #include "soundsfx_core.h"
 #include "speaker_module.h"
 #include <stdlib.h>
 
 //==============================================================================
 // COMMUNICATION SOUND FUNCTIONS
 //==============================================================================
 
 /**
  * @brief Play talking sound effect
  * 
  * Generates expressive beeps that mimic natural speech rhythm
  * with varied pitch, timing, and intonation patterns.
  */
 void sfxPlayTalking(void) {
     audio_state_t audioState = getAudioState();
     if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
     
     // Extended conversational pattern with natural speech rhythm
     // Pattern: "Hello there, how are you doing today? I hope you're well!"
     sfxPlayTone(380, 45, SFX_BASE_VOLUME * 0.5);  // "Hel-"
     sfxDelay(25);
     sfxPlayTone(420, 35, SFX_BASE_VOLUME * 0.6);  // "-lo"
     sfxDelay(60);                                  // Word break
     sfxPlayTone(450, 50, SFX_BASE_VOLUME * 0.5);  // "there,"
     sfxDelay(80);                                  // Comma pause
     sfxPlayTone(400, 30, SFX_BASE_VOLUME * 0.4);  // "how"
     sfxDelay(35);
     sfxPlayTone(460, 40, SFX_BASE_VOLUME * 0.5);  // "are"
     sfxDelay(30);
     sfxPlayTone(420, 45, SFX_BASE_VOLUME * 0.5);  // "you"
     sfxDelay(35);
     sfxPlayTone(390, 40, SFX_BASE_VOLUME * 0.5);  // "do-"
     sfxDelay(25);
     sfxPlayTone(350, 50, SFX_BASE_VOLUME * 0.6);  // "-ing"
     sfxDelay(40);
     sfxPlayTone(430, 35, SFX_BASE_VOLUME * 0.5);  // "to-"
     sfxDelay(25);
     sfxPlayTone(380, 40, SFX_BASE_VOLUME * 0.5);  // "-day?"
     sfxDelay(90);                                  // Sentence break
     sfxPlayTone(410, 30, SFX_BASE_VOLUME * 0.4);  // "I"
     sfxDelay(40);
     sfxPlayTone(450, 45, SFX_BASE_VOLUME * 0.5);  // "hope"
     sfxDelay(35);
     sfxPlayTone(420, 40, SFX_BASE_VOLUME * 0.5);  // "you're"
     sfxDelay(30);
     sfxPlayTone(380, 55, SFX_BASE_VOLUME * 0.6);  // "well!" (falling)
 }
 
 /**
  * @brief Play alternative talking sound effect
  * 
  * Generates a more animated, higher-pitched conversation pattern
  * with questioning intonation and excitement.
  */
 void sfxPlayAltTalking(void) {
     audio_state_t audioState = getAudioState();
     if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
     
     // Extended excited/questioning pattern with rising intonation
     // Pattern: "Really? That's absolutely amazing! Tell me more about it!" 
     sfxPlayTone(520, 40, SFX_BASE_VOLUME * 0.4);  // "Real-"
     sfxDelay(20);
     sfxPlayTone(580, 35, SFX_BASE_VOLUME * 0.5);  // "-ly?" (rising)
     sfxDelay(70);                                  // Question pause
     sfxPlayTone(480, 30, SFX_BASE_VOLUME * 0.4);  // "That's"
     sfxDelay(30);
     sfxPlayTone(540, 35, SFX_BASE_VOLUME * 0.5);  // "ab-"
     sfxDelay(20);
     sfxPlayTone(590, 30, SFX_BASE_VOLUME * 0.5);  // "-so-"
     sfxDelay(25);
     sfxPlayTone(620, 45, SFX_BASE_VOLUME * 0.6);  // "-lute-"
     sfxDelay(25);
     sfxPlayTone(680, 40, SFX_BASE_VOLUME * 0.6);  // "-ly"
     sfxDelay(20);
     sfxPlayTone(580, 55, SFX_BASE_VOLUME * 0.5);  // "a-ma-zing!" (falling)
     sfxDelay(80);                                  // Exclamation pause
     sfxPlayTone(500, 35, SFX_BASE_VOLUME * 0.5);  // "Tell"
     sfxDelay(30);
     sfxPlayTone(560, 40, SFX_BASE_VOLUME * 0.5);  // "me"
     sfxDelay(35);
     sfxPlayTone(520, 45, SFX_BASE_VOLUME * 0.5);  // "more"
     sfxDelay(30);
     sfxPlayTone(480, 35, SFX_BASE_VOLUME * 0.4);  // "a-"
     sfxDelay(25);
     sfxPlayTone(520, 50, SFX_BASE_VOLUME * 0.5);  // "-bout"
     sfxDelay(30);
     sfxPlayTone(560, 45, SFX_BASE_VOLUME * 0.5);  // "it!" (excited ending)
 }
 
 /**
  * @brief Play laughing sound effect
  * 
  * Generates an expressive laughing pattern with building intensity
  * and natural rhythm like genuine laughter.
  */
 void sfxPlayLaughing(void) {
     audio_state_t audioState = getAudioState();
     if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
     
     // Natural laughter pattern: "heh-heh-ha-HA-ha-hehe"
     sfxPlayTone(450, 30, SFX_BASE_VOLUME * 0.4);  // "heh" (quiet start)
     sfxDelay(40);
     sfxPlayTone(480, 35, SFX_BASE_VOLUME * 0.5);  // "heh" (building)
     sfxDelay(45);
     sfxPlayTone(550, 45, SFX_BASE_VOLUME * 0.6);  // "ha" (louder)
     sfxDelay(35);
     sfxPlayTone(620, 55, SFX_BASE_VOLUME * 0.8);  // "HA!" (peak)
     sfxDelay(40);
     sfxPlayTone(580, 40, SFX_BASE_VOLUME * 0.6);  // "ha" (coming down)
     sfxDelay(30);
     sfxPlayTone(500, 25, SFX_BASE_VOLUME * 0.4);  // "he-"
     sfxDelay(20);
     sfxPlayTone(520, 30, SFX_BASE_VOLUME * 0.4);  // "-he" (trailing off)
 }
 
 /**
  * @brief Play angry sound effect
  * 
  * Generates harsh, aggressive beeps with growling rhythm
  * and frustrated intonation patterns.
  */
 void sfxPlayAngry(void) {
     audio_state_t audioState = getAudioState();
     if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
     
     // Angry outburst pattern: building frustration and harsh tones
     // Pattern: "Grr... NO! That's wrong!"
     sfxPlayTone(200, 60, SFX_BASE_VOLUME * 0.6);  // "Grr..." (growl)
     sfxDelay(40);
     sfxPlayTone(180, 70, SFX_BASE_VOLUME * 0.7);  // Deeper growl
     sfxDelay(50);                                  // Tension pause
     sfxPlayTone(280, 45, SFX_BASE_VOLUME * 0.8);  // "NO!" (sharp)
     sfxDelay(30);
     sfxPlayTone(220, 55, SFX_BASE_VOLUME * 0.7);  // "That's"
     sfxDelay(25);
     sfxPlayTone(160, 80, SFX_BASE_VOLUME * 0.9);  // "WRONG!" (harsh finale)
 }
 
 /**
  * @brief Play shock sound effect
  * 
  * Generates a sharp gasp pattern that conveys sudden surprise
  * with quick inhalation and trailing off sounds.
  */
 void sfxPlayShock(void) {
     audio_state_t audioState = getAudioState();
     if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
     
     // Surprised gasp pattern: "Oh! Wow... what?!"
     sfxPlayTone(850, 25, SFX_BASE_VOLUME * 0.7);  // "Oh!" (sharp intake)
     sfxDelay(30);                                  // Brief pause
     sfxPlayTone(600, 40, SFX_BASE_VOLUME * 0.6);  // "Wow..." (processing)
     sfxDelay(60);                                  // Longer pause (thinking)
     sfxPlayTone(480, 30, SFX_BASE_VOLUME * 0.5);  // "what" (confused)
     sfxDelay(20);
     sfxPlayTone(520, 35, SFX_BASE_VOLUME * 0.5);  // "?!" (questioning)
 }