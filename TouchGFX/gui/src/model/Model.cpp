#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

Model::Model() : ImageX(95), modelListener(0), highScore(0)
{
	modelTickCount = 0;
}

void Model::tick()
{
	modelTickCount++;
}

void Model::update(int16_t x)
{
	ImageX = x;
}

int16_t Model::GetImageX()
{
	return ImageX;
}

void Model::setHighScore(int score)
{
	if (score > highScore) {
		highScore = score;
	}
}

int Model::getHighScore()
{
	return highScore;
}
