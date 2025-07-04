#ifndef __BUZZER_H
#define __BUZZER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include <stdint.h>
#include <stdbool.h>

/* Type Definitions ----------------------------------------------------------*/

/**
  * @brief  Buzzer Command Structure for sound effects
  */
typedef struct {
    uint16_t frequency;
    uint16_t duration;
} BuzzerCommand_t;

/**
  * @brief  Music Note Structure
  */
typedef struct {
    uint16_t frequency;  // Frequency in Hz (0 = REST)
    uint16_t duration;   // Duration in ms
    uint16_t pause;      // Pause after note in ms
} MusicNote_t;

/**
  * @brief  Music Track Structure - unified for all music types
  */
typedef struct {
    MusicNote_t* notes;
    uint16_t noteCount;
    uint16_t currentNote;
    bool isPlaying;
    bool shouldLoop;     // Unified loop control
} MusicTrack_t;

/**
  * @brief  Sound Effect Types
  */
typedef enum {
    SFX_CATCH = 0,
    SFX_LOSE_HP,
    SFX_COUNT        // Used for validation
} SfxType_t;

/**
  * @brief  Background Music Types
  */
typedef enum {
    BG_KATYUSHA = 0,
    BG_GAME_OVER,
    BG_COUNT         // Used for validation
} BgMusicType_t;

/**
  * @brief  Sound Effect Configuration
  */
typedef struct {
    uint16_t frequency;
    uint16_t duration;
} SfxConfig_t;

/**
  * @brief  Background Music Configuration
  */
typedef struct {
    MusicNote_t* notes;
    uint16_t noteCount;
    bool shouldLoop;
} BgMusicConfig_t;

/* Note frequency definitions - standard musical notes */
#define C4  262
#define CS4 277
#define D4  294
#define DS4 311
#define E4  330
#define F4  349
#define FS4 370
#define G4  392
#define GS4 415
#define A4  440
#define AS4 466
#define B4  494
#define C5  523
#define CS5 554
#define D5  587
#define DS5 622
#define E5  659
#define F5  698
#define FS5 740
#define G5  784
#define GS5 831
#define A5  880
#define AS5 932
#define B5  988
#define A3  220
#define BB4 466
#define C6  1047
#define CS6 1109
#define D6  1175
#define DS6 1245
#define E6  1319
#define F6  1397
#define FS6 1480
#define G6  1568
#define GS6 1661
#define A6  1760
#define AS6 1865
#define B6  1976
#define REST 0      // Rest (no sound)

/* Public Function Prototypes -----------------------------------------------*/

/**
  * @brief  Initialize buzzer system
  * @param  htim: Timer handle for PWM
  * @param  channel: Timer channel
  * @param  queue: Message queue handle
  */
void buzzer_init(TIM_HandleTypeDef *htim, uint32_t channel, osMessageQueueId_t queue);

/**
  * @brief  Non-blocking function to play a tone
  * @param  frequency: Frequency in Hz
  * @param  duration: Duration in ms
  */
void buzzer_play_tone(uint16_t frequency, uint16_t duration);

/**
  * @brief  Buzzer Task - handles all buzzer operations
  * @param  argument: Not used
  */
void buzzer_task(void *argument);

/**
  * @brief  Play a music track (unified interface)
  * @param  notes: Array of music notes
  * @param  noteCount: Number of notes in the track
  * @param  shouldLoop: Whether the track should loop
  */
void buzzer_play_music_track(MusicNote_t* notes, uint16_t noteCount, bool shouldLoop);

/**
  * @brief  Stop current music track
  */
void buzzer_stop_music_track(void);

/**
  * @brief  Play sound effects (type-safe)
  * @param  type: Sound effect type
  */
void buzzer_play_sfx(SfxType_t type);

/**
  * @brief  Play background music (type-safe)
  * @param  type: Background music type
  */
void buzzer_play_bg(BgMusicType_t type);

/**
  * @brief  Check if any music is currently playing
  * @retval true if music is playing, false otherwise
  */
bool buzzer_is_music_playing(void);

/**
  * @brief  Check if current track is looping
  * @retval true if current track is looping, false otherwise
  */
bool buzzer_is_looping(void);

/* Backward Compatibility Macros (deprecated) -------------------------------*/
#define SFX_CATCH_OLD    SFX_CATCH
#define SFX_LOSE_HP_OLD  SFX_LOSE_HP
#define BG_KATYUSHA_OLD  BG_KATYUSHA
#define BG_GAME_OVER_OLD BG_GAME_OVER

/* Legacy function aliases (deprecated - use new APIs) */
#define buzzer_stop_katyusha_theme() buzzer_stop_music_track()
#define buzzer_is_katyusha_playing() buzzer_is_looping()

#ifdef __cplusplus
}
#endif

#endif /* __BUZZER_H */
