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
      isBombActive(false), bombSpawnChance(3), isBlastActive(false),
      blastTimer(0), blastX(0), blastY(0), activeFruitCount(0),
      lastFruitSpawnTick(0), minSpawnInterval(30), maxSpawnInterval(90),
      maxActiveFruits(2), isHeartActive(false), isSnowflakeActive(false),
      snowflakeTimer(0), originalFallSpeed(2), isDamageBgActive(false),
      damageBgTimer(0)
{
    tickCount = 0;

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

bool Screen2View::checkCollisionWithBomb()
{
    if (!isBombActive) return false;

    int16_t x1 = basket.getX();
    int16_t y1 = basket.getY();
    int16_t w1 = basket.getWidth();
    int16_t h1 = basket.getHeight();

    int16_t x2 = bomb.getX();
    int16_t y2 = bomb.getY();
    int16_t w2 = bomb.getWidth();
    int16_t h2 = bomb.getHeight();

    return !(x1 + w1 < x2 || x2 + w2 < x1 ||
             y1 + h1 < y2 || y2 + h2 < y1);
}

bool Screen2View::checkCollisionWithHeart()
{
    if (!isHeartActive) return false;

    int16_t x1 = basket.getX();
    int16_t y1 = basket.getY();
    int16_t w1 = basket.getWidth();
    int16_t h1 = basket.getHeight();

    int16_t x2 = heart.getX();
    int16_t y2 = heart.getY();
    int16_t w2 = heart.getWidth();
    int16_t h2 = heart.getHeight();

    return !(x1 + w1 < x2 || x2 + w2 < x1 ||
             y1 + h1 < y2 || y2 + h2 < y1);
}

bool Screen2View::checkCollisionWithSnowflake()
{
    if (!isSnowflakeActive) return false;

    int16_t x1 = basket.getX();
    int16_t y1 = basket.getY();
    int16_t w1 = basket.getWidth();
    int16_t h1 = basket.getHeight();

    int16_t x2 = snowflake.getX();
    int16_t y2 = snowflake.getY();
    int16_t w2 = snowflake.getWidth();
    int16_t h2 = snowflake.getHeight();

    return !(x1 + w1 < x2 || x2 + w2 < x1 ||
             y1 + h1 < y2 || y2 + h2 < y1);
}

void Screen2View::spawnSpecialItem()
{
    // Chọn ngẫu nhiên heart hoặc snowflake
    SpecialItemType itemType = static_cast<SpecialItemType>(xorshift32() % 2);

    int index = xorshift32() % 6;
    int newX = index * 40 + 15;

    if (itemType == SpecialItemType::HEART) {
        isHeartActive = true;
        heart.setX(newX);
        heart.setY(0);
        heart.setVisible(true);
        heart.invalidate();
    } else {
        isSnowflakeActive = true;
        snowflake.setX(newX);
        snowflake.setY(0);
        snowflake.setVisible(true);
        snowflake.invalidate();
    }
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

                // Phát âm thanh khi miss fruit
                play_lose_hp();

                // Hiển thị damage bg effect
                showDamageBg();

                // Không còn combo system

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

                    // Phát music game over
                    play_game_over();

                    // Hiện button restart khi game over
                    flexButton2.setVisible(true);

                    // Ẩn tất cả fruit và reset
                    for (int j = 0; j < MAX_FRUITS; j++) {
                        activeFruits[j].setVisible(false);
                        activeFruits[j] = FruitObject();
                    }
                    activeFruitCount = 0;

                    bomb.setVisible(false);
                    textGameOver.invalidate();
                    flexButton2.invalidate();
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

            // Phát âm thanh khi bắt được fruit
            play_catch_sound();

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

void Screen2View::updateBgEffects()
{
    // Xử lý damage bg timer
    if (isDamageBgActive) {
        damageBgTimer--;
        if (damageBgTimer <= 0) {
            isDamageBgActive = false;
            resetToNormalBg();
        }
    }
}

void Screen2View::showDamageBg()
{
    // Ẩn bg bình thường và bg frozen
    bg.setVisible(false);
    bgBlue.setVisible(false);

    // Hiện bg damage
    bgRed.setVisible(true);

    // Invalidate để update hiển thị
    bg.invalidate();
    bgRed.invalidate();
    bgBlue.invalidate();

    // Set timer cho 0.25 giây (15 ticks @ 60Hz)
    isDamageBgActive = true;
    damageBgTimer = 15;
}

void Screen2View::resetToNormalBg()
{
    if (snowflakeTimer > 0) {
        // Nếu đang có frozen effect, hiện bgBlue
        bg.setVisible(false);
        bgRed.setVisible(false);
        bgBlue.setVisible(true);
    } else {
        // Không có effect nào, hiện bg bình thường
        bgRed.setVisible(false);
        bgBlue.setVisible(false);
        bg.setVisible(true);
    }

    // Invalidate để update hiển thị
    bg.invalidate();
    bgRed.invalidate();
    bgBlue.invalidate();
}

void Screen2View::restartGame()
{
    // Reset tất cả trạng thái game về ban đầu
    score = 0;
    hp = 10;
    isBombActive = false;
    activeFruitCount = 0;
    lastFruitSpawnTick = 0;
    maxActiveFruits = 2;

    // Reset game progression
    tickCount = 0;
    fallSpeed = 2;
    speedLevel = 0;
    localImageX = 100;
    basketSpeedStep = 4;
    minSpawnInterval = 30;
    maxSpawnInterval = 90;
    bombSpawnChance = 3;

    // Reset special items
    isHeartActive = false;
    isSnowflakeActive = false;
    snowflakeTimer = 0;
    originalFallSpeed = 2;

    // Reset blast effect
    isBlastActive = false;
    blastTimer = 0;

    // Reset bg effects
    isDamageBgActive = false;
    damageBgTimer = 0;

    // Reset game state
    isGameOver = false;

    // Ẩn button restart khi game đang chạy
    flexButton2.setVisible(false);

    // Cập nhật hiển thị
    Unicode::snprintf(scoreBuffer, sizeof(scoreBuffer), "%d", score);
    textScore.setWildcard(scoreBuffer);
    textScore.invalidate();

    Unicode::snprintf(hpBuffer, sizeof(hpBuffer), "%d", hp);
    hpScore.setWildcard(hpBuffer);
    hpScore.invalidate();

    // Reset vị trí xe
    basket.setX(localImageX);
    basket.invalidate();

    // Ẩn tất cả objects
    bomb.setVisible(false);
    blast.setVisible(false);
    heart.setVisible(false);
    snowflake.setVisible(false);
    apple.setVisible(false);
    banana.setVisible(false);
    grape.setVisible(false);
    textGameOver.setVisible(false);

    // Reset bg về bình thường
    bg.setVisible(true);
    bgRed.setVisible(false);
    bgBlue.setVisible(false);

    // Invalidate tất cả
    bomb.invalidate();
    blast.invalidate();
    heart.invalidate();
    snowflake.invalidate();
    apple.invalidate();
    banana.invalidate();
    grape.invalidate();
    textGameOver.invalidate();
    bg.invalidate();
    bgRed.invalidate();
    bgBlue.invalidate();
    flexButton2.invalidate();

    // Reset fruit array
    for (int i = 0; i < MAX_FRUITS; i++) {
        activeFruits[i] = FruitObject();
    }

    // Spawn fruit đầu tiên
    spawnNewFruit();
}

void Screen2View::forceSpawnSpecialItem()
{
    // Debug function: Force spawn special item ngay lập tức để test
    if (!isHeartActive && !isSnowflakeActive) {
        spawnSpecialItem();
    }
}

void Screen2View::setupScreen()
{
    stop_katyusha_theme();

    localImageX = presenter->GetImageX();
    score = 0;
    highScore = presenter->GetHighScore();
    hp = 10;
    isBombActive = false;
    activeFruitCount = 0;
    lastFruitSpawnTick = 0;
    maxActiveFruits = 2; // Bắt đầu với 2 fruit

    // Gọi base setup TRƯỚC
    Screen2ViewBase::setupScreen();

    // SAU ĐÓ mới cập nhật các buffer để overwrite giá trị mặc định
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

    // Ẩn bomb, blast và special items khi bắt đầu
    bomb.setVisible(false);
    blast.setVisible(false);
    heart.setVisible(false);
    snowflake.setVisible(false);
    isBlastActive = false;
    blastTimer = 0;
    isHeartActive = false;
    isSnowflakeActive = false;
    snowflakeTimer = 0;

    // Khởi tạo bg effects - hiện bg bình thường
    bg.setVisible(true);
    bgRed.setVisible(false);
    bgBlue.setVisible(false);
    isDamageBgActive = false;
    damageBgTimer = 0;

    // Ẩn tất cả fruit và button restart
    apple.setVisible(false);
    banana.setVisible(false);
    grape.setVisible(false);
    flexButton2.setVisible(false);

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
        // Chờ người dùng nhấn nút PG3 để chơi lại
        uint8_t msg;
        if (osMessageQueueGet(Queue1Handle, &msg, NULL, 0) == osOK && msg == 'A') {
            // Reset lại trạng thái game
            score = 0;
            hp = 3;
            isBombActive = false;
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

            bomb.setVisible(false);
            blast.setVisible(false);
            heart.setVisible(false);
            snowflake.setVisible(false);
            isBlastActive = false;
            blastTimer = 0;
            isHeartActive = false;
                isSnowflakeActive = false;
    snowflakeTimer = 0;
    originalFallSpeed = 2;
    isDamageBgActive = false;
    damageBgTimer = 0;
            basket.setX(localImageX);
            textGameOver.setVisible(false);

                // Hiện lại đường bình thường
    bg.setVisible(true);
    bgRed.setVisible(false);
    bgBlue.setVisible(false);

            // Reset fruit array
            for (int i = 0; i < MAX_FRUITS; i++) {
                activeFruits[i] = FruitObject();
            }

            // Spawn fruit đầu tiên
            spawnNewFruit();

            basket.invalidate();
            bomb.invalidate();
            blast.invalidate();
            textGameOver.invalidate();
        }

        return;
    }

    // 1. Xử lý nút điều khiển xe
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

    // Xử lý blast effect timer
    if (isBlastActive) {
        blastTimer++;
        if (blastTimer >= 60) {
            isBlastActive = false;
            blast.setVisible(false);
            blast.invalidate();
            blastTimer = 0;
        }
    }

    // 3. Xử lý tất cả fruits
    updateAllFruits();
    checkAllFruitCollisions();
    spawnFruitIfNeeded();

    // 3b. Xử lý bg effects
    updateBgEffects();

    // 4. Xử lý bomb rơi (nếu có)
    if (isBombActive) {
        int newBombY = bomb.getY() + fallSpeed;
        if (newBombY >= 320) {
            isBombActive = false;
            bomb.setVisible(false);
            bomb.invalidate();
        } else {
            bomb.setY(newBombY);
        }
    }

    // 5. Kiểm tra va chạm với bomb
    if (checkCollisionWithBomb()) {
        blastX = bomb.getX();
        blastY = bomb.getY();

        isBombActive = false;
        bomb.setVisible(false);
        bomb.invalidate();

        isBlastActive = true;
        blastTimer = 0;
        blast.setX(blastX);
        blast.setY(blastY);
        blast.setVisible(true);
        blast.invalidate();

        hp--;

        play_lose_hp();

        // Hiển thị damage bg effect
        showDamageBg();

        Unicode::snprintf(hpBuffer, sizeof(hpBuffer), "%d", hp);
        hpScore.setWildcard(hpBuffer);
        hpScore.invalidate();

        if (hp <= 0) {
            isGameOver = true;
            textGameOver.setVisible(true);

            // Phát music game over
            play_game_over();

            // Hiện button restart khi game over (cần thêm flexButton2 trong TouchGFX Designer)
            flexButton2.setVisible(true);

            for (int i = 0; i < MAX_FRUITS; i++) {
                activeFruits[i].setVisible(false);
            }
            activeFruitCount = 0;

            textGameOver.invalidate();
            flexButton2.invalidate();
            return;
        }
    }

    // 5b. Xử lý special items
    // Xử lý heart rơi
    if (isHeartActive) {
        int newHeartY = heart.getY() + fallSpeed;
        if (newHeartY >= 320) {
            isHeartActive = false;
            heart.setVisible(false);
            heart.invalidate();
        } else {
            heart.setY(newHeartY);
        }
    }

    // Xử lý snowflake rơi
    if (isSnowflakeActive) {
        int newSnowflakeY = snowflake.getY() + fallSpeed;
        if (newSnowflakeY >= 320) {
            isSnowflakeActive = false;
            snowflake.setVisible(false);
            snowflake.invalidate();
        } else {
            snowflake.setY(newSnowflakeY);
        }
    }

    // Kiểm tra va chạm với heart
    if (checkCollisionWithHeart()) {
        hp++;
        isHeartActive = false;
        heart.setVisible(false);
        heart.invalidate();

        // Phát âm thanh khi ăn heart (dùng cùng âm thanh như ăn quả)
        play_catch_sound();

        Unicode::snprintf(hpBuffer, sizeof(hpBuffer), "%d", hp);
        hpScore.setWildcard(hpBuffer);
        hpScore.invalidate();
    }

    // Kiểm tra va chạm với snowflake
    if (checkCollisionWithSnowflake()) {
        // Kích hoạt slowdown effect trong 5 giây (300 ticks)
        originalFallSpeed = fallSpeed;
        fallSpeed = fallSpeed / 2; // Giảm tốc độ xuống một nửa
        if (fallSpeed < 1) fallSpeed = 1;

        snowflakeTimer = 300; // 5 giây @ 60Hz
        isSnowflakeActive = false;
        snowflake.setVisible(false);
        snowflake.invalidate();

        // Phát âm thanh khi ăn snowflake (dùng cùng âm thanh như ăn quả)
        play_catch_sound();

        // Hiển thị frozen bg effect (bgBlue) nếu không đang có damage effect
        if (!isDamageBgActive) {
            bg.setVisible(false);
            bgRed.setVisible(false);
            bgBlue.setVisible(true);
            bg.invalidate();
            bgRed.invalidate();
            bgBlue.invalidate();
        }
    }

    // Xử lý snowflake timer
    if (snowflakeTimer > 0) {
        snowflakeTimer--;
        if (snowflakeTimer == 0) {
            // Khôi phục tốc độ ban đầu
            fallSpeed = originalFallSpeed;

            // Reset về bg bình thường nếu không có damage effect
            if (!isDamageBgActive) {
                resetToNormalBg();
            }
        }
    }

    // 6. Tạo bomb ngẫu nhiên (6 làn)
    if (!isBombActive && (tickCount % 60 == 0) && (xorshift32() % bombSpawnChance == 0)) {
        isBombActive = true;
        bomb.setY(0);
        int index = xorshift32() % 6;
        int newX = index * 40 + 15;
        bomb.setX(newX);
        bomb.setVisible(true);
        bomb.invalidate();
    }

    // 6b. Tạo special items (heart/snowflake) - sử dụng biến từ main.c để dễ test
    if (!isHeartActive && !isSnowflakeActive &&
        (tickCount % specialEffectSpawnInterval == 0) &&
        (xorshift32() % specialEffectSpawnChance == 0)) {
        spawnSpecialItem();
    }

    // 7. Tăng độ khó theo thời gian
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

        // Tăng tỷ lệ bomb xuất hiện (giảm bombSpawnChance = tăng tỷ lệ)
        if (bombSpawnChance > 50) {
            bombSpawnChance -= 5;
                }
      }

      invalidate();
}
