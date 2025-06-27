/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : buzzer.c
  * @brief          : Buzzer system implementation for FruitCatcher game
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "buzzer.h"

/* Private variables ---------------------------------------------------------*/
static TIM_HandleTypeDef *buzzer_htim;
static uint32_t buzzer_channel;
static osMessageQueueId_t buzzer_queue;

/* Global music track variable */
static MusicTrack_t currentTrack = {0};

/* Public Functions ----------------------------------------------------------*/

/**
  * @brief  Initialize buzzer system
  * @param  htim: Timer handle for PWM
  * @param  channel: Timer channel
  * @param  queue: Message queue handle
  */
void buzzer_init(TIM_HandleTypeDef *htim, uint32_t channel, osMessageQueueId_t queue)
{
    buzzer_htim = htim;
    buzzer_channel = channel;
    buzzer_queue = queue;
}

/**
  * @brief  Generate tone using PWM timer
  * @param  htim: Timer handle
  * @param  channel: Timer channel
  * @param  frequency: Frequency in Hz
  * @param  duration: Duration in ms
  */
void buzzer_tone(TIM_HandleTypeDef *htim, uint32_t channel, uint16_t frequency, uint16_t duration)
{
    uint32_t period = 1000000 / frequency;
    uint32_t pulse = period / 2;

    __HAL_TIM_SET_AUTORELOAD(htim, period - 1);
    __HAL_TIM_SET_COMPARE(htim, channel, pulse);

    HAL_TIM_PWM_Start(htim, channel);
    osDelay(duration);
    HAL_TIM_PWM_Stop(htim, channel);
}

/**
  * @brief  Test buzzer with different tones
  * @note   This function should be called from a FreeRTOS task
  */
void buzzer_test(void)
{
    buzzer_tone(buzzer_htim, buzzer_channel, C4, 200);
    osDelay(100);
    buzzer_tone(buzzer_htim, buzzer_channel, E4, 200);
    osDelay(100);
    buzzer_tone(buzzer_htim, buzzer_channel, G4, 200);
}

/**
  * @brief  Non-blocking function to play a tone
  * @param  frequency: Frequency in Hz
  * @param  duration: Duration in ms
  */
void buzzer_play_tone(uint16_t frequency, uint16_t duration)
{
    BuzzerCommand_t cmd;
    cmd.frequency = frequency;
    cmd.duration = duration;
    
    osMessageQueuePut(buzzer_queue, &cmd, 0, 0);
}

/**
  * @brief  Play test sequence without blocking
  */
void buzzer_play_test_sequence(void)
{
    buzzer_play_tone(C4, 200);
    buzzer_play_tone(E4, 200);
    buzzer_play_tone(G4, 200);
}

/**
  * @brief  Buzzer Task - handles all buzzer operations
  * @param  argument: Not used
  */
void buzzer_task(void *argument)
{
    BuzzerCommand_t cmd;
    osStatus_t status;
    
    for(;;)
    {
        status = osMessageQueueGet(buzzer_queue, &cmd, NULL, 5);
        
        if (status == osOK)
        {
            uint32_t period = 1000000 / cmd.frequency;
            uint32_t pulse = period / 2;

            __HAL_TIM_SET_AUTORELOAD(buzzer_htim, period - 1);
            __HAL_TIM_SET_COMPARE(buzzer_htim, buzzer_channel, pulse);

            HAL_TIM_PWM_Start(buzzer_htim, buzzer_channel);
            osDelay(cmd.duration);
            HAL_TIM_PWM_Stop(buzzer_htim, buzzer_channel);
            
            osDelay(20);
        }
        else if (currentTrack.isPlaying && currentTrack.notes != NULL)
        {
            MusicNote_t* currentNote = &currentTrack.notes[currentTrack.currentNote];
            
            if (currentNote->frequency > 0)
            {
                uint32_t period = 1000000 / currentNote->frequency;
                uint32_t pulse = period / 2;

                __HAL_TIM_SET_AUTORELOAD(buzzer_htim, period - 1);
                __HAL_TIM_SET_COMPARE(buzzer_htim, buzzer_channel, pulse);

                HAL_TIM_PWM_Start(buzzer_htim, buzzer_channel);
                osDelay(currentNote->duration);
                HAL_TIM_PWM_Stop(buzzer_htim, buzzer_channel);
            }
            else
            {
                osDelay(currentNote->duration);
            }
            
            if (currentNote->pause > 0)
            {
                osDelay(currentNote->pause);
            }
            
            currentTrack.currentNote++;
            
            if (currentTrack.currentNote >= currentTrack.noteCount)
            {
                currentTrack.isPlaying = false;
                currentTrack.currentNote = 0;
            }
        }
        else
        {
            osDelay(20);
        }
    }
}

/**
  * @brief  Play a music track (non-blocking)
  * @param  notes: Array of music notes
  * @param  noteCount: Number of notes in the track
  */
void buzzer_play_music_track(MusicNote_t* notes, uint16_t noteCount)
{
    buzzer_stop_music_track();
    
    currentTrack.notes = notes;
    currentTrack.noteCount = noteCount;
    currentTrack.currentNote = 0;
    currentTrack.isPlaying = true;
}

/**
  * @brief  Stop current music track
  */
void buzzer_stop_music_track(void)
{
    currentTrack.isPlaying = false;
    currentTrack.currentNote = 0;
    HAL_TIM_PWM_Stop(buzzer_htim, buzzer_channel);
}

/**
  * @brief  Debug function to test buzzer queue
  */
void buzzer_debug_test(void)
{
    buzzer_play_tone(C4, 100);
    buzzer_play_tone(E4, 100);
    buzzer_play_tone(G4, 100);
}