#ifndef SCREEN2VIEW_HPP
#define SCREEN2VIEW_HPP

#include <gui_generated/screen2_screen/Screen2ViewBase.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>
#include "main.h"

// Enum cho các loại trái cây và item đặc biệt
enum class FruitType {
    APPLE = 0,   // 1 điểm
    BANANA = 1,  // 2 điểm  
    GRAPE = 2    // 3 điểm
};

enum class SpecialItemType {
    HEART = 0,      // +1 HP
    SNOWFLAKE = 1   // Slow down effect
};

// Class quản lý trái cây
class FruitObject {
public:
    FruitType type;
    int16_t x, y;
    bool isActive;
    int points;
    touchgfx::Image* imageWidget;
    
    FruitObject() : type(FruitType::APPLE), x(0), y(0), isActive(false), points(1), imageWidget(nullptr) {}
    
    FruitObject(FruitType fruitType, touchgfx::Image* widget) 
        : type(fruitType), x(0), y(0), isActive(true), imageWidget(widget) {
        switch(fruitType) {
            case FruitType::APPLE:  points = 1; break;
            case FruitType::BANANA: points = 2; break;
            case FruitType::GRAPE:  points = 3; break;
        }
    }
    
    void setPosition(int16_t newX, int16_t newY) {
        x = newX;
        y = newY;
        if (imageWidget && isActive) {
            imageWidget->setX(x);
            imageWidget->setY(y);
        }
    }
    
    void setVisible(bool visible) {
        if (imageWidget) {
            imageWidget->setVisible(visible);
            imageWidget->invalidate();
        }
        isActive = visible;
    }
    
    bool checkCollision(int16_t targetX, int16_t targetY, int16_t targetW, int16_t targetH) {
        if (!isActive || !imageWidget) return false;
        
        int16_t w = imageWidget->getWidth();
        int16_t h = imageWidget->getHeight();
        
        return !(targetX + targetW < x || x + w < targetX ||
                 targetY + targetH < y || y + h < targetY);
    }
};

class Screen2View : public Screen2ViewBase
{
public:
    Screen2View();
    virtual ~Screen2View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual bool checkCollisionWithFruit(FruitObject& fruit);
    virtual bool checkCollisionWithBomb();
    virtual bool checkCollisionWithHeart();
    virtual bool checkCollisionWithSnowflake();
    virtual void spawnNewFruit();
    virtual void spawnSpecialItem();
    virtual void updateFruitPosition(FruitObject& fruit);
    virtual void updateAllFruits();
    virtual void checkAllFruitCollisions();
    virtual void spawnFruitIfNeeded();
    virtual void updateBgEffects();
    virtual void showDamageBg();
    virtual void resetToNormalBg();
    virtual void restartGame();
    
    // Debug functions for testing
    virtual void forceSpawnSpecialItem();
    
protected:
    Unicode::UnicodeChar scoreBuffer[30];
    Unicode::UnicodeChar highScoreBuffer[30];
    Unicode::UnicodeChar hpBuffer[20];
    // comboBuffer được định nghĩa trong base class rồi
    int score;
    int highScore;  // Regular variable, will be synced with model
    int hp;
    int16_t localImageX;
    int tickCount;
    bool isGameOver;
    int16_t fallSpeed;
    int16_t speedLevel;
    int16_t basketSpeedStep;
    bool seeded = false;
    bool isBombActive;
    int16_t bombSpawnChance;
    
    // Combo system đã được xóa
    
    
    // Quản lý blast effect
    bool isBlastActive;
    int blastTimer;
    int16_t blastX, blastY;
    
    // Quản lý nhiều trái cây - THAY ĐỔI CHÍNH
    static const int MAX_FRUITS = 6;  // Tối đa 6 fruit cùng lúc
    FruitObject activeFruits[MAX_FRUITS];
    int activeFruitCount;
    
    // Timing control cho spawn
    int lastFruitSpawnTick;
    int16_t minSpawnInterval;     // Khoảng cách tối thiểu giữa các lần spawn
    int16_t maxSpawnInterval;     // Khoảng cách tối đa giữa các lần spawn
    
    // Game progression
    int16_t maxActiveFruits;      // Số fruit tối đa có thể active (tăng theo level)
    
    // Special items
    bool isHeartActive;
    bool isSnowflakeActive;
    int snowflakeTimer;      // Timer cho snowflake effect
    int16_t originalFallSpeed;    // Lưu tốc độ gốc trước khi bị snowflake ảnh hưởng
    
    // Background effects
    bool isDamageBgActive;     // Hiện bg đỏ khi bị trừ HP
    int damageBgTimer;         // Timer cho bg đỏ (15 ticks = 0.25 giây)
};

#endif // SCREEN2VIEW_HPP
