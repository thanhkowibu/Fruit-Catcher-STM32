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
    uint16_t frequency;
    uint16_t duration;
    uint16_t pause;
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
#define D5  587
#define E5  659
#define G5  784
#define GS5 831
#define A5  880

#define REST 0

/* Function Prototypes */
void buzzer_init(TIM_HandleTypeDef *htim, uint32_t channel, osMessageQueueId_t queue);
void buzzer_tone(TIM_HandleTypeDef *htim, uint32_t channel, uint16_t frequency, uint16_t duration);
void buzzer_test(void);
void buzzer_play_tone(uint16_t frequency, uint16_t duration);
void buzzer_play_test_sequence(void);
void buzzer_task(void *argument);
void buzzer_play_music_track(MusicNote_t* notes, uint16_t noteCount);
void buzzer_stop_music_track(void);
void buzzer_debug_test(void);

#ifdef __cplusplus
}
#endif

#endif /* __BUZZER_H */