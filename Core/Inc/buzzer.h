#ifndef __BUZZER_H
#define __BUZZER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include <stdint.h>

/* Buzzer Command Structure */
typedef struct {
    uint16_t frequency;
    uint16_t duration;
} BuzzerCommand_t;

/* Note frequency definitions - standard musical notes */
#define C4  262
#define D4  294
#define E4  330
#define G4  392
#define A4  440

/* Function Prototypes */
void buzzer_init(TIM_HandleTypeDef *htim, uint32_t channel, osMessageQueueId_t queue);
void buzzer_tone(TIM_HandleTypeDef *htim, uint32_t channel, uint16_t frequency, uint16_t duration);
void buzzer_test(void);
void buzzer_play_tone(uint16_t frequency, uint16_t duration);
void buzzer_play_test_sequence(void);
void buzzer_task(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* __BUZZER_H */