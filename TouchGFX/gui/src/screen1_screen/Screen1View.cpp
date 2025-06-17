#include <gui/screen1_screen/Screen1View.hpp>
#include "main.h"

Screen1View::Screen1View()
{

}

void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();
    
    // Phát intro music khi screen1 xuất hiện (màn hình menu chính)
    play_intro_music();
}

void Screen1View::tearDownScreen()
{
    Screen1ViewBase::tearDownScreen();
}

void Screen1View::handleTickEvent()
{
	tickCounter += 1;

}
