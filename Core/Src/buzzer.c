/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : buzzer.c
  * @brief          : Refactored buzzer system implementation for FruitCatcher game
  *                   Following SOLID principles and DRY pattern
  ******************************************************************************
  */
/* USER CODE END Header */

#include "buzzer.h"

/* Private variables ---------------------------------------------------------*/
static TIM_HandleTypeDef *buzzer_htim;
static uint32_t buzzer_channel;
static osMessageQueueId_t buzzer_queue;

/* Unified music system - no more separate Katyusha logic */
static MusicTrack_t currentTrack = {0};

/* Music track constants */
#define GAME_OVER_NOTE_COUNT 5
#define KATYUSHA_NOTE_COUNT 80

/* Forward declarations of music data arrays */
static MusicNote_t game_over_notes[];
static MusicNote_t katyusha_notes[];

/* Private function prototypes -----------------------------------------------*/
static void buzzer_play_note(uint16_t frequency, uint16_t duration);
static void buzzer_handle_track_end(void);
static void buzzer_process_current_note(void);
static void buzzer_configure_pwm(uint16_t frequency);
static void buzzer_start_pwm(void);
static void buzzer_stop_pwm(void);

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

    // Initialize track state
    currentTrack.isPlaying = false;
    currentTrack.currentNote = 0;
    currentTrack.shouldLoop = false;
}

/**
  * @brief  Non-blocking function to play a tone
  * @param  frequency: Frequency in Hz
  * @param  duration: Duration in ms
  */
void buzzer_play_tone(uint16_t frequency, uint16_t duration)
{
    BuzzerCommand_t cmd = {
        .frequency = frequency,
        .duration = duration
    };

    osMessageQueuePut(buzzer_queue, &cmd, 0, 0);
}

/**
  * @brief  Unified buzzer task - handles all buzzer operations
  * @param  argument: Not used
  */
void buzzer_task(void *argument)
{
    BuzzerCommand_t cmd;
    const uint32_t QUEUE_TIMEOUT_MS = 5;
    const uint32_t IDLE_DELAY_MS = 20;

    for(;;)
    {
        // Check for sound effects (highest priority)
        if (osMessageQueueGet(buzzer_queue, &cmd, NULL, QUEUE_TIMEOUT_MS) == osOK)
        {
            // Sound effect interrupts any playing music
            buzzer_play_note(cmd.frequency, cmd.duration);
            osDelay(IDLE_DELAY_MS); // Brief pause after sound effect
        }
        else if (currentTrack.isPlaying && currentTrack.notes != NULL)
        {
            // Process current music track
            buzzer_process_current_note();
        }
        else
        {
            // Nothing to do
            osDelay(IDLE_DELAY_MS);
        }
    }
}

/**
  * @brief  Play a music track (unified interface)
  * @param  notes: Array of music notes
  * @param  noteCount: Number of notes in the track
  * @param  shouldLoop: Whether the track should loop
  */
void buzzer_play_music_track(MusicNote_t* notes, uint16_t noteCount, bool shouldLoop)
{
    // Stop current track if playing
    buzzer_stop_music_track();

    // Configure new track
    currentTrack.notes = notes;
    currentTrack.noteCount = noteCount;
    currentTrack.currentNote = 0;
    currentTrack.isPlaying = true;
    currentTrack.shouldLoop = shouldLoop;
}

/**
  * @brief  Stop current music track
  */
void buzzer_stop_music_track(void)
{
    currentTrack.isPlaying = false;
    currentTrack.currentNote = 0;
    currentTrack.shouldLoop = false;
    buzzer_stop_pwm();
}

/**
  * @brief  Play sound effects
  * @param  type: Sound effect type
  */
void buzzer_play_sfx(SfxType_t type)
{
    const SfxConfig_t sfx_configs[] = {
        [SFX_CATCH] = {1500, 100},      // High pitched success sound
        [SFX_LOSE_HP] = {300, 250},     // Low pitched fail sound
    };

    if (type < SFX_COUNT) {
        const SfxConfig_t* config = &sfx_configs[type];
        buzzer_play_tone(config->frequency, config->duration);
    }
}

/**
  * @brief  Play background music
  * @param  type: Background music type
  */
void buzzer_play_bg(BgMusicType_t type)
{
    const BgMusicConfig_t bg_configs[] = {
        [BG_KATYUSHA] = {katyusha_notes, KATYUSHA_NOTE_COUNT, true},    // Looping
        [BG_GAME_OVER] = {game_over_notes, GAME_OVER_NOTE_COUNT, false} // One-shot
    };

    if (type < BG_COUNT) {
        const BgMusicConfig_t* config = &bg_configs[type];
        buzzer_play_music_track(config->notes, config->noteCount, config->shouldLoop);
    }
}

/**
  * @brief  Check if any music is currently playing
  * @retval true if music is playing, false otherwise
  */
bool buzzer_is_music_playing(void)
{
    return currentTrack.isPlaying;
}

/**
  * @brief  Check if current track is looping
  * @retval true if current track is looping, false otherwise
  */
bool buzzer_is_looping(void)
{
    return currentTrack.shouldLoop && currentTrack.isPlaying;
}

/* Private Functions ---------------------------------------------------------*/

/**
  * @brief  Play a single note with PWM
  * @param  frequency: Frequency in Hz
  * @param  duration: Duration in ms
  */
static void buzzer_play_note(uint16_t frequency, uint16_t duration)
{
    if (frequency > 0) {
        buzzer_configure_pwm(frequency);
        buzzer_start_pwm();
        osDelay(duration);
        buzzer_stop_pwm();
    } else {
        // REST note
        osDelay(duration);
    }
}

/**
  * @brief  Process the current note in the playing track
  */
static void buzzer_process_current_note(void)
{
    MusicNote_t* currentNote = &currentTrack.notes[currentTrack.currentNote];

    // Play the note
    buzzer_play_note(currentNote->frequency, currentNote->duration);

    // Add pause if specified
    if (currentNote->pause > 0) {
        osDelay(currentNote->pause);
    }

    // Advance to next note
    currentTrack.currentNote++;

    // Check if track ended
    if (currentTrack.currentNote >= currentTrack.noteCount) {
        buzzer_handle_track_end();
    }
}

/**
  * @brief  Handle track end logic (loop or stop)
  */
static void buzzer_handle_track_end(void)
{
    if (currentTrack.shouldLoop) {
        // Reset to beginning for loop
        currentTrack.currentNote = 0;
        osDelay(1000); // Pause between loops
    } else {
        // Stop the track
        buzzer_stop_music_track();
    }
}

/**
  * @brief  Configure PWM parameters for given frequency
  * @param  frequency: Frequency in Hz
  */
static void buzzer_configure_pwm(uint16_t frequency)
{
    // Calculate Period and Pulse with timer clock = 1MHz (180MHz/(179+1))
    uint32_t period = 1000000 / frequency;
    uint32_t pulse = period / 2; // 50% duty cycle

    __HAL_TIM_SET_AUTORELOAD(buzzer_htim, period - 1);
    __HAL_TIM_SET_COMPARE(buzzer_htim, buzzer_channel, pulse);
}

/**
  * @brief  Start PWM output
  */
static void buzzer_start_pwm(void)
{
    HAL_TIM_PWM_Start(buzzer_htim, buzzer_channel);
}

/**
  * @brief  Stop PWM output
  */
static void buzzer_stop_pwm(void)
{
    HAL_TIM_PWM_Stop(buzzer_htim, buzzer_channel);
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
