#ifndef MODEL_HPP
#define MODEL_HPP

#include <stdint.h>
class ModelListener;

class Model
{
public:
    Model();
    long modelTickCount = 0;
    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();
    void update(int16_t x);
    int16_t GetImageX();
    
    // HighScore storage
    void setHighScore(int score);
    int getHighScore();

    int16_t ImageX;
    int highScore = 0;  // Persistent highScore storage
protected:
    ModelListener* modelListener;
};

#endif // MODEL_HPP
