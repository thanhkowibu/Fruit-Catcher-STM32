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

/* Buzzer Command Structure */
typedef struct {
    uint16_t frequency;
    uint16_t duration;
} BuzzerCommand_t;

/* Music System - MIDI-like format but non-blocking */
typedef struct {
    uint16_t frequency;  // Tần số (Hz)
    uint16_t duration;   // Thời gian phát (ms)
    uint16_t pause;      // Thời gian nghỉ sau note (ms)
} MusicNote_t;

typedef struct {
    MusicNote_t* notes;
    uint16_t noteCount;
    uint16_t currentNote;
    bool isPlaying;
} MusicTrack_t;

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
#define C5  523

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

#define REST 0  // Nghỉ (không phát âm)

/* Function Prototypes */
void buzzer_init(TIM_HandleTypeDef *htim, uint32_t channel, osMessageQueueId_t queue);
void buzzer_tone(TIM_HandleTypeDef *htim, uint32_t channel, uint16_t frequency, uint16_t duration);
void buzzer_test(void);
void buzzer_play_tone(uint16_t frequency, uint16_t duration);
void buzzer_play_test_sequence(void);
void buzzer_task(void *argument);
void buzzer_play_music_track(MusicNote_t* notes, uint16_t noteCount);
void buzzer_stop_music_track(void);
void buzzer_play_game_over(void);
void buzzer_play_intro_music(void);
void buzzer_play_catch_sound(void);
void buzzer_play_lose_hp(void);
void buzzer_play_special_effect(void);

/* SIUUUUUUUUUUUUUUU */
void buzzer_play_katyusha_theme(void);
void buzzer_stop_katyusha_theme(void);
bool buzzer_is_katyusha_playing(void);

/* Debug function */
void buzzer_debug_test(void);

#ifdef __cplusplus
}
#endif

#endif /* __BUZZER_H */ 
