/**
 * @file soundsfx_communication.h
 * @brief Communication and vocal sound effects for retro computer audio
 */

 #ifndef SOUNDSFX_COMMUNICATION_H
 #define SOUNDSFX_COMMUNICATION_H
 
 #include <stdint.h>
 #include <stdbool.h>
 
 #define SFX_DEFAULT_TALKING_DURATION 400
 #define SFX_DEFAULT_ALT_TALKING_DURATION 350
 #define SFX_DEFAULT_LAUGHING_DURATION 600
 #define SFX_DEFAULT_ANGRY_DURATION 500
 #define SFX_DEFAULT_SHOCK_DURATION 300
 
 #define SFX_TALKING_BASE_FREQ 300
 #define SFX_ALT_TALKING_BASE_FREQ 450
 #define SFX_LAUGHING_BASE_FREQ 500
 #define SFX_ANGRY_BASE_FREQ 200
 #define SFX_SHOCK_BASE_FREQ 800
 
 //==============================================================================
 // COMMUNICATION SOUND FUNCTIONS
 //==============================================================================
 
 /**
  * @brief Play talking sound effect
  * 
  * Generates a modulated low-frequency sound that mimics speech patterns
  * with random frequency variations and amplitude modulation.
  * Default duration: ~400ms
  */
 void sfxPlayTalking(void);
 
 /**
  * @brief Play alternative talking sound effect
  * 
  * Generates a higher-pitched talking sound with different modulation
  * pattern, suitable for different characters or emphasis.
  * Default duration: ~350ms
  */
 void sfxPlayAltTalking(void);
 
 /**
  * @brief Play laughing sound effect
  * 
  * Generates a rhythmic, bouncing sound pattern that mimics laughter
  * with ascending frequency bursts and varying intervals.
  * Default duration: ~600ms
  */
 void sfxPlayLaughing(void);
 
 /**
  * @brief Play angry sound effect
  * 
  * Generates a harsh, growling sound with low frequencies and
  * aggressive modulation patterns to convey anger or frustration.
  * Default duration: ~500ms
  */
 void sfxPlayAngry(void);
 
 /**
  * @brief Play shock sound effect
  * 
  * Generates a sharp, high-pitched sound that starts high and
  * drops quickly, mimicking a gasp or surprised exclamation.
  * Default duration: ~300ms
  */
 void sfxPlayShock(void);
 
 #endif /* SOUNDSFX_COMMUNICATION_H */