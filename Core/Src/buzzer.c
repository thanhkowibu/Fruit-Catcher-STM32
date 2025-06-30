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
  * @brief  Test buzzer with different tones
  * @note   This function should be called from a FreeRTOS task
  */
void buzzer_test(void)
{
    // Test với các tần số khác nhau
    buzzer_tone(buzzer_htim, buzzer_channel, 1000, 200);  // 1000Hz - 200ms
    osDelay(100);  // Nghỉ 100ms
    
    buzzer_tone(buzzer_htim, buzzer_channel, 1500, 200);  // 1500Hz - 200ms  
    osDelay(100);
    
    buzzer_tone(buzzer_htim, buzzer_channel, 2000, 200);  // 2000Hz - 200ms
    osDelay(100);
    
    buzzer_tone(buzzer_htim, buzzer_channel, 800, 300);   // 800Hz - 300ms (low tone)
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
  * @brief  Play test sequence without blocking
  */
void buzzer_play_test_sequence(void)
{
    buzzer_play_tone(1000, 200);  // 1000Hz - 200ms
    buzzer_play_tone(1500, 200);  // 1500Hz - 200ms
    buzzer_play_tone(2000, 200);  // 2000Hz - 200ms
    buzzer_play_tone(800, 300);   // 800Hz - 300ms
}

/**
  * @brief  Buzzer Task - handles all buzzer operations
  * @param  argument: Not used
  */
/**
  * @brief  Modified Buzzer Task - handles all buzzer operations with Katyusha loop support
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
  * @brief  Play game over music (trầm hơn một chút)
  */
void buzzer_play_game_over(void)
{
    static MusicNote_t game_over_notes[] = {
        {GS4, 300, 0},    // Trầm hơn: AS5 -> GS4 (415Hz)
        {GS5, 640, 640},  // Trầm hơn: AS6 -> GS5 (831Hz) 
        {G5, 340, 950},   // Trầm hơn: A6 -> G5 (784Hz)
        {AS4, 640, 0},    // Trầm hơn: B5 -> AS4 (466Hz)
        {D5, 950, 0},     // Trầm hơn: FS6 -> D5 (587Hz)
    };
    
    buzzer_play_music_track(game_over_notes, 5);
}

/**
  * @brief  Play intro music khi start game - ngắn và trầm hơn
  */
void buzzer_play_intro_music(void)
{
    static MusicNote_t intro_notes[] = {
        {C4, 250, 100},   // Do trầm
        {E4, 250, 100},   // Mi trầm
        {G4, 400, 0},     // Sol trầm kết thúc
    };
    
    buzzer_play_music_track(intro_notes, 3);
}

/**
  * @brief  Play catch sound effect
  */
void buzzer_play_catch_sound(void)
{
    // Luôn phát sound effect, có thể interrupt music track
    buzzer_play_tone(1500, 100);  // High pitched success sound
}

/**
  * @brief  Play lose HP sound effect (miss fruit hoặc dính bomb) - trầm hơn
  */
void buzzer_play_lose_hp(void)
{
    // Luôn phát sound effect, có thể interrupt music track
    buzzer_play_tone(300, 250);   // Trầm hơn: 400Hz -> 300Hz, dài hơn
}

/**
  * @brief  Play special effect sound (heart và snowflake) - single tone Te (La 4)
  */
void buzzer_play_special_effect(void)
{
    // Single tone Te (La 4) 440Hz
    buzzer_play_tone(A4, 200);
}

/**
  * @brief  Debug function to test buzzer queue - phát 3 beep ngắn
  */
void buzzer_debug_test(void)
{
    buzzer_play_tone(1000, 100);
    buzzer_play_tone(1200, 100); 
    buzzer_play_tone(1400, 100);
} 


/* Private variables - thêm vào phần private variables */
static MusicTrack_t katyushaTrack = {0};

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

/**
  * @brief  Play Katyusha theme in loop mode
  */
void buzzer_play_katyusha_theme(void)
{
    // Stop current music if playing
    buzzer_stop_music_track();

    // Enable Katyusha loop mode
    katyushaLoopEnabled = true;

    // Set up Katyusha track
    katyushaTrack.notes = katyusha_notes;
    katyushaTrack.noteCount = sizeof(katyusha_notes) / sizeof(MusicNote_t);
    katyushaTrack.currentNote = 0;
    katyushaTrack.isPlaying = true;

    // Set as current track
    currentTrack = katyushaTrack;
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
