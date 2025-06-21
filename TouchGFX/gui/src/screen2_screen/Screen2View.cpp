#include <gui/screen2_screen/Screen2View.hpp>
#include <stdint.h>
#include "cmsis_os.h"
#include "main.h"
#include "stm32f4xx_hal.h"

extern osMessageQueueId_t Queue1Handle;

uint32_t xorshift32()
{
    static uint32_t seed = 0;

    if (seed == 0)
        seed = HAL_GetTick();

    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;

    return seed;
}

Screen2View::Screen2View()
    : isGameOver(false), fallSpeed(2), hp(10), score(0), highScore(0)
{
    tickCount = 0;
    localImageX = 100;
}

bool Screen2View::checkCollisionWithFruit()
{
    if (!apple.isVisible()) return false;
    
    int16_t x1 = basket.getX();
    int16_t y1 = basket.getY();
    int16_t w1 = basket.getWidth();
    int16_t h1 = basket.getHeight();

    int16_t x2 = apple.getX();
    int16_t y2 = apple.getY();
    int16_t w2 = apple.getWidth();
    int16_t h2 = apple.getHeight();

    return !(x1 + w1 < x2 || x2 + w2 < x1 ||
             y1 + h1 < y2 || y2 + h2 < y1);
}

void Screen2View::spawnNewFruit()
{
    // Spawn apple tại vị trí ngẫu nhiên trên đầu màn hình
    int index = xorshift32() % 6;
    int newX = index * 40 + 15;
    
    apple.setX(newX);
    apple.setY(0);
    apple.setVisible(true);
    apple.invalidate();
}

void Screen2View::setupScreen()
{
    localImageX = presenter->GetImageX();
    score = 0;
    highScore = presenter->GetHighScore();
    hp = 10;
    
    // Gọi base setup TRƯỚC
    Screen2ViewBase::setupScreen();
    
    // SAU ĐÓ mới cập nhật các buffer
    basket.setX(localImageX);
    
    // Khởi tạo hiển thị HP
    Unicode::snprintf(hpBuffer, sizeof(hpBuffer), "%d", hp);
    hpScore.setWildcard(hpBuffer);
    hpScore.invalidate();
    
    // Khởi tạo hiển thị Score
    Unicode::snprintf(scoreBuffer, sizeof(scoreBuffer), "%d", score);
    textScore.setWildcard(scoreBuffer);
    textScore.invalidate();
    
    // Khởi tạo hiển thị HighScore
    Unicode::snprintf(highScoreBuffer, sizeof(highScoreBuffer), "%d", highScore);
    textHighScore.setWildcard(highScoreBuffer);
    textHighScore.invalidate();
    
    // Ẩn apple ban đầu
    apple.setVisible(false);
    
    // Spawn fruit đầu tiên
    spawnNewFruit();
}

void Screen2View::tearDownScreen()
{
    Screen2ViewBase::tearDownScreen();
    presenter->UpdateImageX(localImageX);
}

void Screen2View::handleTickEvent()
{
    Screen2ViewBase::handleTickEvent();

    if (isGameOver) {
        // Chờ người dùng nhấn nút để chơi lại
        uint8_t msg;
        if (osMessageQueueGet(Queue1Handle, &msg, NULL, 0) == osOK && msg == 'A') {
            // Reset lại trạng thái game
            score = 0;
            hp = 10;
            isGameOver = false;
            tickCount = 0;
            fallSpeed = 2;
            localImageX = 100;

            Unicode::snprintf(scoreBuffer, sizeof(scoreBuffer), "%d", score);
            textScore.setWildcard(scoreBuffer);
            textScore.invalidate();

            Unicode::snprintf(hpBuffer, sizeof(hpBuffer), "%d", hp);
            hpScore.setWildcard(hpBuffer);
            hpScore.invalidate();

            basket.setX(localImageX);
            textGameOver.setVisible(false);

            // Spawn fruit mới
            spawnNewFruit();

            basket.invalidate();
            textGameOver.invalidate();
        }
        return;
    }

    // 1. Xử lý nút điều khiển basket
    uint8_t msg;
    if (osMessageQueueGet(Queue1Handle, &msg, NULL, 0) == osOK) {
        const int16_t minX = 0;
        const int16_t maxX = 240 - basket.getWidth();
        const int16_t basketSpeedStep = 4;

        if (msg == 'L' && localImageX > minX) {
            localImageX -= basketSpeedStep;
        } else if (msg == 'R' && localImageX < maxX) {
            localImageX += basketSpeedStep;
        }

        basket.setX(localImageX);
        basket.invalidate();
    }

    // 2. Animation và logic game
    tickCount++;

    // 3. Xử lý fruit rơi
    if (apple.isVisible()) {
        int newY = apple.getY() + fallSpeed;
        if (newY >= 320) {
            // Fruit ra khỏi màn hình -> trừ 1 HP
            hp--;
            
            // Cập nhật hiển thị HP
            Unicode::snprintf(hpBuffer, sizeof(hpBuffer), "%d", hp);
            hpScore.setWildcard(hpBuffer);
            hpScore.invalidate();

            // Kiểm tra game over
            if (hp <= 0) {
                isGameOver = true;
                textGameOver.setVisible(true);
                apple.setVisible(false);
                textGameOver.invalidate();
                return;
            }
            
            // Spawn fruit mới
            spawnNewFruit();
        } else {
            apple.setY(newY);
            apple.invalidate();
        }
    }

    // 4. Kiểm tra va chạm với fruit
    if (checkCollisionWithFruit()) {
        // Chạm vào fruit -> cộng điểm
        score += 1;  // Apple = 1 điểm
        if (score > highScore) {
            highScore = score;
            presenter->SetHighScore(highScore);
        }

        // Cập nhật hiển thị điểm
        Unicode::snprintf(scoreBuffer, sizeof(scoreBuffer), "%d", score);
        textScore.setWildcard(scoreBuffer);
        textScore.invalidate();

        Unicode::snprintf(highScoreBuffer, sizeof(highScoreBuffer), "%d", highScore);
        textHighScore.setWildcard(highScoreBuffer);
        textHighScore.invalidate();

        // Spawn fruit mới
        spawnNewFruit();
    }

    // 5. Tăng độ khó theo thời gian
    if (tickCount % 300 == 0) { // Mỗi 5 giây
        if (fallSpeed < 5) {
            fallSpeed++;
        }
    }

    invalidate();
}
