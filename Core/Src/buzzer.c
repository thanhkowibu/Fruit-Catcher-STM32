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

/* Katyusha loop control variable */
static bool katyushaLoopEnabled = false;

/* Private variables for music tracks */
static MusicTrack_t katyushaTrack = {0};

/* Music track constants */
#define GAME_OVER_NOTE_COUNT 5
#define KATYUSHA_NOTE_COUNT 80

/* Forward declarations of music data arrays */
static MusicNote_t game_over_notes[];
static MusicNote_t katyusha_notes[];

/* Private function prototypes -----------------------------------------------*/

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
    // Tính toán Period và Pulse với timer clock = 1MHz (180MHz/(179+1))
    uint32_t period = 1000000 / frequency; // Tính giá trị cho ARR
    uint32_t pulse = period / 2;          // Tính giá trị cho CCR (duty cycle 50%)

    // Cấu hình lại Timer
    __HAL_TIM_SET_AUTORELOAD(htim, period - 1); // Cập nhật chu kỳ
    __HAL_TIM_SET_COMPARE(htim, channel, pulse); // Cập nhật độ rộng xung

    // Bắt đầu phát âm
    HAL_TIM_PWM_Start(htim, channel);
    osDelay(duration); // Sử dụng osDelay thay vì HAL_Delay để không block FreeRTOS

    // Dừng phát âm
    HAL_TIM_PWM_Stop(htim, channel);
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

    // Gửi command đến buzzer queue (non-blocking)
    osMessageQueuePut(buzzer_queue, &cmd, 0, 0);
}

/**
  * @brief  Buzzer Task - handles all buzzer operations with Katyusha loop support
  * @param  argument: Not used
  */
void buzzer_task(void *argument)
{
    BuzzerCommand_t cmd;
    osStatus_t status;

    for(;;)
    {
        // Kiểm tra sound effects với timeout ngắn hơn
        status = osMessageQueueGet(buzzer_queue, &cmd, NULL, 5);  // 5ms timeout

        if (status == osOK)
        {
            // Có sound effect -> phát ngay (interrupt music nếu cần)
            uint32_t period = 1000000 / cmd.frequency;
            uint32_t pulse = period / 2;

            __HAL_TIM_SET_AUTORELOAD(buzzer_htim, period - 1);
            __HAL_TIM_SET_COMPARE(buzzer_htim, buzzer_channel, pulse);

            HAL_TIM_PWM_Start(buzzer_htim, buzzer_channel);
            osDelay(cmd.duration);
            HAL_TIM_PWM_Stop(buzzer_htim, buzzer_channel);

            osDelay(20); // Nghỉ ngắn sau sound effect
        }
        else if (currentTrack.isPlaying && currentTrack.notes != NULL) {
            // Không có sound effect -> phát music track
            MusicNote_t* currentNote = &currentTrack.notes[currentTrack.currentNote];

            if (currentNote->frequency > 0) {  // Không phải REST
                uint32_t period = 1000000 / currentNote->frequency;
                uint32_t pulse = period / 2;

                __HAL_TIM_SET_AUTORELOAD(buzzer_htim, period - 1);
                __HAL_TIM_SET_COMPARE(buzzer_htim, buzzer_channel, pulse);

                HAL_TIM_PWM_Start(buzzer_htim, buzzer_channel);
                osDelay(currentNote->duration);
                HAL_TIM_PWM_Stop(buzzer_htim, buzzer_channel);
            } else {
                // REST note
                osDelay(currentNote->duration);
            }

            // Pause giữa các notes
            if (currentNote->pause > 0) {
                osDelay(currentNote->pause);
            }

            // Chuyển sang note tiếp theo
            currentTrack.currentNote++;

            // Kiểm tra xem đã hết track chưa
            if (currentTrack.currentNote >= currentTrack.noteCount) {
                // Nếu đang trong chế độ Katyusha loop
                if (katyushaLoopEnabled) {
                    // Reset về đầu để loop
                    currentTrack.currentNote = 0;
                    osDelay(1000); // Nghỉ 1 giây giữa các lần loop
                } else {
                    // Track bình thường -> dừng
                    currentTrack.isPlaying = false;
                    currentTrack.currentNote = 0;
                }
            }
        } else {
            // Không có gì để phát
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
    // Stop current track if playing
    buzzer_stop_music_track();

    // Set new track
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
  * @brief  Play sound effects
  * @param  type: Sound effect type (SFX_CATCH or SFX_LOSE_HP)
  */
void buzzer_play_sfx(int type)
{
    switch(type) {
        case SFX_CATCH:
            // High pitched success sound (catch fruit)
            buzzer_play_tone(1500, 100);
            break;

        case SFX_LOSE_HP:
            // Low pitched fail sound (lose HP)
            buzzer_play_tone(300, 250);
            break;

        default:
            // Invalid type, do nothing
            break;
    }
}

/**
  * @brief  Play background music
  * @param  type: Background music type (BG_KATYUSHA or BG_GAME_OVER)
  */
void buzzer_play_bg(int type)
{
    switch(type) {
        case BG_KATYUSHA:
            // Play Katyusha theme in loop mode
            buzzer_stop_music_track();
            katyushaLoopEnabled = true;
            katyushaTrack.notes = katyusha_notes;
            katyushaTrack.noteCount = KATYUSHA_NOTE_COUNT;
            katyushaTrack.currentNote = 0;
            katyushaTrack.isPlaying = true;
            currentTrack = katyushaTrack;
            break;

        case BG_GAME_OVER:
            // Play game over music (one-shot)
            buzzer_stop_music_track();
            katyushaLoopEnabled = false;
            buzzer_play_music_track(game_over_notes, GAME_OVER_NOTE_COUNT);
            break;

        default:
            // Invalid type, do nothing
            break;
    }
}

/**
  * @brief  Stop Katyusha theme
  */
void buzzer_stop_katyusha_theme(void)
{
    katyushaLoopEnabled = false;
    buzzer_stop_music_track();
}

/**
  * @brief  Check if Katyusha is currently playing
  * @retval true if Katyusha is playing, false otherwise
  */
bool buzzer_is_katyusha_playing(void)
{
    return katyushaLoopEnabled && currentTrack.isPlaying;
}

/* Music Data Arrays ---------------------------------------------------------*/

/**
  * @brief  Game over music notes
  */
static MusicNote_t game_over_notes[] = {
    {GS4, 300, 0},    // Trầm hơn: AS5 -> GS4 (415Hz)
    {GS5, 640, 640},  // Trầm hơn: AS6 -> GS5 (831Hz)
    {G5, 340, 950},   // Trầm hơn: A6 -> G5 (784Hz)
    {AS4, 640, 0},    // Trầm hơn: B5 -> AS4 (466Hz)
    {D5, 950, 0},     // Trầm hơn: FS6 -> D5 (587Hz)
};

/**
  * @brief  Katyusha melody notes array
  */
static MusicNote_t katyusha_notes[] = {
    // Phrase 1 - MODERATE TEMPO (1.5x speed)
    {D4, 270, 70},   // D4
    {REST, 130, 0},   // rest
    {E4, 270, 70},   // E4
    {F4, 270, 70},   // F4
    {REST, 130, 0},   // rest
    {D4, 270, 70},   // D4
    {F4, 270, 70},   // F4
    {F4, 270, 70},   // F4
    {E4, 270, 70},   // E4
    {D4, 270, 70},   // D4
    {E4, 270, 70},   // E4
    {A3, 270, 70},   // A3
    {E4, 270, 70},   // E4
    {REST, 130, 0},   // rest
    {F4, 270, 70},   // F4
    {G4, 270, 70},   // G4
    {REST, 130, 0},   // rest
    {E4, 270, 70},   // E4
    {G4, 270, 70},   // G4
    {G4, 270, 70},   // G4
    {F4, 270, 70},   // F4
    {E4, 270, 70},   // E4
    {D4, 270, 70},   // D4
    {REST, 270, 0},   // rest
    {REST, 270, 0},   // rest

    // Phrase 2 - MODERATE TEMPO (1.5x speed)
    {A4, 270, 70},   // A4
    {REST, 130, 0},   // rest
    {D5, 270, 70},   // D5
    {REST, 130, 0},   // rest
    {C5, 270, 70},   // C5
    {REST, 130, 0},   // rest
    {D5, 270, 70},   // D5
    {BB4, 270, 70},  // Bb4
    {BB4, 270, 70},  // Bb4
    {A4, 270, 70},   // A4
    {G4, 270, 70},   // G4
    {A4, 270, 70},   // A4
    {D4, 270, 70},   // D4
    {REST, 270, 0},   // rest
    {REST, 270, 0},   // rest
    {BB4, 270, 70},  // Bb4
    {G4, 270, 70},   // G4
    {A4, 270, 70},   // A4
    {REST, 130, 0},   // rest
    {E4, 270, 70},   // E4
    {G4, 270, 70},   // G4
    {G4, 270, 70},   // G4
    {F4, 270, 70},   // F4
    {E4, 270, 70},   // E4
    {D4, 270, 70},   // D4
    {REST, 270, 0},   // rest
    {REST, 270, 0},   // rest

    // Phrase 3 (repeat of phrase 2) - MODERATE TEMPO (1.5x speed)
    {A4, 270, 70},   // A4
    {REST, 130, 0},   // rest
    {D5, 270, 70},   // D5
    {REST, 130, 0},   // rest
    {C5, 270, 70},   // C5
    {REST, 130, 0},   // rest
    {D5, 270, 70},   // D5
    {BB4, 270, 70},  // Bb4
    {BB4, 270, 70},  // Bb4
    {A4, 270, 70},   // A4
    {G4, 270, 70},   // G4
    {A4, 270, 70},   // A4
    {D4, 270, 70},   // D4
    {REST, 270, 0},   // rest
    {REST, 270, 0},   // rest
    {BB4, 270, 70},  // Bb4
    {G4, 270, 70},   // G4
    {A4, 270, 70},   // A4
    {REST, 130, 0},   // rest
    {E4, 270, 70},   // E4
    {G4, 270, 70},   // G4
    {G4, 270, 70},   // G4
    {F4, 270, 70},   // F4
    {E4, 270, 70},   // E4
    {D4, 270, 70},   // D4
    {REST, 270, 0},   // rest
    {REST, 270, 0},   // rest
    {REST, 530, 0},   // ending pause
};
