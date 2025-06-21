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
    : isGameOver(false), fallSpeed(2), speedLevel(0), basketSpeedStep(4), hp(10),
      activeFruitCount(0), lastFruitSpawnTick(0), minSpawnInterval(30), maxSpawnInterval(90),
      maxActiveFruits(2)
{
    tickCount = 0;
    localImageX = 100;

    // Khởi tạo array fruit rỗng
    for (int i = 0; i < MAX_FRUITS; i++) {
        activeFruits[i] = FruitObject();
    }
}

bool Screen2View::checkCollisionWithFruit(FruitObject& fruit)
{
    int16_t x1 = basket.getX();
    int16_t y1 = basket.getY();
    int16_t w1 = basket.getWidth();
    int16_t h1 = basket.getHeight();

    return fruit.checkCollision(x1, y1, w1, h1);
}

void Screen2View::spawnNewFruit()
{
    // Tìm slot trống trong array
    int emptySlot = -1;
    for (int i = 0; i < MAX_FRUITS; i++) {
        if (!activeFruits[i].isActive) {
            emptySlot = i;
            break;
        }
    }

    if (emptySlot == -1) return; // Không có slot trống
    
    // Chọn loại trái cây ngẫu nhiên
    FruitType fruitType = static_cast<FruitType>(xorshift32() % 3);
    
    // Tìm widget trái cây phù hợp mà CHƯA được sử dụng
    touchgfx::Image* widget = nullptr;
    bool widgetFound = false;
    
    // Kiểm tra xem widget này đã được dùng chưa
    for (int attempt = 0; attempt < 10 && !widgetFound; attempt++) {
        switch(fruitType) {
            case FruitType::APPLE:
                widget = &apple;
                break;
            case FruitType::BANANA:
                widget = &banana;
                break;
            case FruitType::GRAPE:
                widget = &grape;
                break;
        }
        
        // Kiểm tra xem widget này có đang được dùng bởi fruit khác không
        bool isUsed = false;
        for (int j = 0; j < MAX_FRUITS; j++) {
            if (j != emptySlot && activeFruits[j].isActive && activeFruits[j].imageWidget == widget) {
                isUsed = true;
                break;
            }
        }
        
        if (!isUsed) {
            widgetFound = true;
        } else {
            // Thử loại fruit khác
            fruitType = static_cast<FruitType>((static_cast<int>(fruitType) + 1) % 3);
        }
    }
    
    if (!widgetFound) return; // Không tìm được widget phù hợp
    
    // Tạo FruitObject mới
    activeFruits[emptySlot] = FruitObject(fruitType, widget);
    
    // Đặt vị trí ngẫu nhiên trong 6 làn
    int index = xorshift32() % 6;
    int newX = index * 40 + 15;
    activeFruits[emptySlot].setPosition(newX, 0);
    activeFruits[emptySlot].setVisible(true);

    activeFruitCount++;
}

void Screen2View::updateFruitPosition(FruitObject& fruit)
{
    if (!fruit.isActive) return;
    
    int newY = fruit.y + fallSpeed;
    fruit.setPosition(fruit.x, newY);
}

void Screen2View::updateAllFruits()
{
    for (int i = 0; i < MAX_FRUITS; i++) {
        if (activeFruits[i].isActive) {
            updateFruitPosition(activeFruits[i]);

            // Kiểm tra fruit có ra khỏi màn hình không  
            if (activeFruits[i].y >= 320) {
                // Fruit ra khỏi màn hình -> trừ 1 HP
                hp--;

                // Ẩn fruit này và reset trạng thái
                activeFruits[i].setVisible(false);
                activeFruits[i] = FruitObject(); // Reset hoàn toàn object
                activeFruitCount--;
                
                // Đảm bảo activeFruitCount không âm
                if (activeFruitCount < 0) {
                    activeFruitCount = 0;
                }

                // Cập nhật hiển thị HP
                Unicode::snprintf(hpBuffer, sizeof(hpBuffer), "%d", hp);
                hpScore.setWildcard(hpBuffer);
                hpScore.invalidate();

                // Kiểm tra game over
                if (hp <= 0) {
                    isGameOver = true;
                    textGameOver.setVisible(true);

                    // Ẩn tất cả fruit và reset
                    for (int j = 0; j < MAX_FRUITS; j++) {
                        activeFruits[j].setVisible(false);
                        activeFruits[j] = FruitObject();
                    }
                    activeFruitCount = 0;

                    textGameOver.invalidate();
                    return;
                }
            }
        }
    }
}

void Screen2View::checkAllFruitCollisions()
{
    for (int i = 0; i < MAX_FRUITS; i++) {
        if (activeFruits[i].isActive && checkCollisionWithFruit(activeFruits[i])) {
            // Chạm vào fruit -> cộng điểm
            score += activeFruits[i].points;
            if (score > highScore) {
                highScore = score;
                presenter->SetHighScore(highScore);  // Save to model via presenter
            }

            // Cập nhật hiển thị điểm
            Unicode::snprintf(scoreBuffer, sizeof(scoreBuffer), "%d", score);
            textScore.setWildcard(scoreBuffer);
            textScore.invalidate();

            Unicode::snprintf(highScoreBuffer, sizeof(highScoreBuffer), "%d", highScore);
            textHighScore.setWildcard(highScoreBuffer);
            textHighScore.invalidate();

            // Ẩn fruit này và reset trạng thái
            activeFruits[i].setVisible(false);
            activeFruits[i] = FruitObject(); // Reset hoàn toàn object
            activeFruitCount--;
            
            // Đảm bảo activeFruitCount không âm
            if (activeFruitCount < 0) {
                activeFruitCount = 0;
            }
        }
    }
}

void Screen2View::spawnFruitIfNeeded()
{
    // Kiểm tra có nên spawn fruit mới không
    bool shouldSpawn = false;

    // Điều kiện 1: Chưa đạt số fruit tối đa
    if (activeFruitCount < maxActiveFruits) {
        // Điều kiện 2: Đã đủ thời gian từ lần spawn cuối
        auto timeSinceLastSpawn = tickCount - lastFruitSpawnTick;
        auto randomInterval = minSpawnInterval + (xorshift32() % (maxSpawnInterval - minSpawnInterval));

        if (timeSinceLastSpawn >= randomInterval) {
            shouldSpawn = true;
        }
    }

    if (shouldSpawn) {
        spawnNewFruit();
        lastFruitSpawnTick = tickCount;
    }
}

void Screen2View::setupScreen()
{
    localImageX = presenter->GetImageX();
    score = 0;
    highScore = presenter->GetHighScore();
    hp = 10;
    activeFruitCount = 0;
    lastFruitSpawnTick = 0;
    maxActiveFruits = 2; // Bắt đầu với 2 fruit
    
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
    
    // Ẩn tất cả fruit
    apple.setVisible(false);
    banana.setVisible(false);
    grape.setVisible(false);

    // Khởi tạo array fruit
    for (int i = 0; i < MAX_FRUITS; i++) {
        activeFruits[i] = FruitObject();
    }

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
            activeFruitCount = 0;
            lastFruitSpawnTick = 0;
            maxActiveFruits = 2;

            Unicode::snprintf(scoreBuffer, sizeof(scoreBuffer), "%d", score);
            textScore.setWildcard(scoreBuffer);
            textScore.invalidate();

            Unicode::snprintf(hpBuffer, sizeof(hpBuffer), "%d", hp);
            hpScore.setWildcard(hpBuffer);
            hpScore.invalidate();

            isGameOver = false;
            tickCount = 0;
            fallSpeed = 2;
            speedLevel = 0;
            localImageX = 100;
            basketSpeedStep = 4;
            minSpawnInterval = 30;
            maxSpawnInterval = 90;

            basket.setX(localImageX);
            textGameOver.setVisible(false);

            // Reset fruit array
            for (int i = 0; i < MAX_FRUITS; i++) {
                activeFruits[i] = FruitObject();
            }

            // Spawn fruit đầu tiên
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

    // 3. Xử lý tất cả fruits
    updateAllFruits();
    checkAllFruitCollisions();
    spawnFruitIfNeeded();

    // 4. Tăng độ khó theo thời gian
    if (tickCount % 300 == 0) { // Mỗi 5 giây
        // Tăng tốc độ
        if (fallSpeed < 5) {
            fallSpeed++;
            speedLevel++;
            basketSpeedStep = fallSpeed;
        }

        // Tăng số lượng fruit tối đa (tối đa 4 fruit cùng lúc)
        if (maxActiveFruits < 4 && speedLevel % 2 == 0) {
            maxActiveFruits++;
        }

        // Giảm thời gian spawn (tăng tần suất)
        if (minSpawnInterval > 15) {
            minSpawnInterval -= 2;
        }
        if (maxSpawnInterval > 45) {
            maxSpawnInterval -= 3;
        }
    }

    invalidate();
} 