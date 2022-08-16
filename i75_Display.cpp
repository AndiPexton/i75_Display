#include <stdio.h>
#include <cstdint>

#include "pico/stdlib.h"
#include  "hardware/vreg.h"

#include "common/pimoroni_common.hpp"
#include "hub75.hpp"
#include "font_6x8.hpp"

using namespace pimoroni;

// Display size in pixels
const uint8_t WIDTH = 64;
const uint8_t HEIGHT = 64;

static const int blinkInterval = 300;
Hub75 hub75(WIDTH, HEIGHT, nullptr, PANEL_GENERIC, true);

void __isr dma_complete() {
    hub75.dma_complete();
}

char screenText [10][8];
Pixel screenColour [10][8];
Pixel screenGraphics [WIDTH][HEIGHT];
int cursorX = 0;
int cursorY = 0;
int pointX = 0;
int pointY = 0;
Pixel currentColour = Pixel(127,127,127);
absolute_time_t blink = make_timeout_time_ms(300);
bool on = false;
bool escape = false;
bool graphics = false;


void processCursor();

void checkBlinkTimeout();

void renderTextChar(int row, int column);

void renderTextChars();

void ResetTextScreen(const Pixel &colour);

void processEscapeCommand(int key);

void processBackSpace();

void processTextToDisplay(int key);

void scrollUpIfNeeded(){
    if (cursorY > 7)
    {
        for(auto row=0; row < 7; row++)
        {
            for(auto column=0; column < 10; column++)
            {
                screenText[column][row] = screenText[column][row+1];
                screenColour[column][row] = screenColour[column][row+1];
            }
        }

        for(auto column=0; column < 10; column++)
        {
            screenText[column][7] = ' ';
            screenColour[column][7] = 0;
        }
        cursorY --;
    }
}

void writeChar(char c)
{
    screenText[cursorX][cursorY] = c;
    screenColour[cursorX][cursorY] = currentColour;

    cursorX ++;
    if (cursorX > 9) {
        cursorX = 0;
        cursorY ++;
        scrollUpIfNeeded();

    }
}

void renderChar(char c,int column, int row, Pixel color){
    uint x = column * letter_width + 2;
    uint y = row * letter_height;
    for(auto pixel = 0u; pixel < letter_width; pixel ++) {
        uint32_t col = font[c - 32][pixel];
        for (auto s_y = 0u; s_y < letter_height; s_y++) {
            if(col & (1 << s_y)) {
                hub75.set_color(x+pixel, s_y + y, color);
            }
        }
    }
}

void renderScreenText() {
    renderTextChars();
    processCursor();
}

void renderTextChars() {
    for (auto row = 0; row < 8; row++)
        for (auto column = 0; column < 10; column++)
            renderTextChar(row, column);
}

void renderTextChar(int row, int column) {
    char c = screenText[column][row];
    Pixel color = screenColour[column][row];
    renderChar(c, column, row, color);
}

void processCursor() {
    checkBlinkTimeout();

    if(on)
        renderChar('_', cursorX, cursorY, currentColour);
}

void checkBlinkTimeout() {
    if ( !time_reached(blink)) return;

    on = !on;
    blink = make_timeout_time_ms(blinkInterval);
}

void renderScreenGraphics()
{
    for(auto x=0; x < WIDTH; x++)
    {
        for(auto y=0; y < HEIGHT; y++)
        {
            hub75.set_color(x,y, screenGraphics[x][y]);
        }
    }
}

void moveUp()
{
    cursorY--;
    if (cursorY<0)
        cursorY = 7;
}

void moveDown()
{
    cursorY++;
    if (cursorY>7)
        cursorY = 0;
}

void moveLeft()
{
    cursorX--;
    if (cursorX<0) {
        cursorX = 9;
        moveUp();
    }
}

void newLine(){
    cursorY ++;
    cursorX =0;
    scrollUpIfNeeded();
}

void moveTo(int row, int col)
{
    cursorY = row;
    cursorX = col;
    if (cursorX>9) cursorX = 9;
    if (cursorY>7) cursorY = 7;
}

void moveRight()
{
    cursorX++;
    if (cursorX>9) {
        cursorX = 0;
        moveDown();
    }
}

int BitsToColourValue(int bits) {
    switch (bits)
    {
        case 1 :
            return 63;
        case 2 :
            return 127;
        case 3 :
            return 255;
        default :
            return 0;
    }
}

Pixel To64Colour(int input)
{
    int redBits = input & 0b00000011;
    int greenBits = (input & 0b00001100) >> 2;
    int blueBits = (input & 0b00110000) >> 4;

    int r = BitsToColourValue(redBits);
    int g = BitsToColourValue(greenBits);
    int b = BitsToColourValue(blueBits);

    return Pixel(r, g, b);
}

void processKey(int key)
{
    if(escape)
    {
        escape = false;
        processEscapeCommand(key);
        return;
    }

    if(key == 27) {
        escape = true;
        return;
    }

    if (key > 0 && key <=255) {
        
        processTextToDisplay(key);
        return;
        
        
    }
}

void processTextToDisplay(int key) {
    if (key == 13) {
        newLine();
        return;
    }

    if (key == 127 ) {
        processBackSpace();
        return;
    }

    if(key >=32) {
        writeChar(key);
        return;
    }
}

void processBackSpace() {
    moveLeft();
    writeChar(' ');
    moveLeft();
}

void processEscapeCommand(int key) {
    if(key == 0) {
        ResetTextScreen(Pixel(127, 127, 127));
        return;
    }

    if(key == 1) {
        ResetTextScreen(Pixel(0, 255, 0));
        return;
    }

    if((key & 128) == 128 )
    {
        currentColour = To64Colour(key);
        return;
    }

    if((key & 64) == 64 )
    {
        graphics=true;
        return;
    }
}

void ResetTextScreen(const Pixel &colour) {
    cursorX = 0;
    cursorY = 0;
    currentColour = colour;

    for (auto row = 0; row < 8; row++) {
        for (auto column = 0; column < 10; column++) {
            screenText[column][row] = 0;
            screenColour[column][row] = Pixel(0, 0, 0);
        }
    }
}

void processGraphics(int value)
{
    if ((value & 192) == 192) {
        graphics = false;
        return;
    }

    if ((value & 128) == 128)
    {
        pointX = value & 0b00111111;
        return;
    }

    if ((value & 64) == 64)
    {
        pointY = value & 0b00111111;
        return;
    }

    screenGraphics[pointX][pointY] = To64Colour(value);
    pointX ++;

    if (pointX > 63) {
        pointX = 0;
        pointY ++;
    }

    if (pointY >63) pointY = 0;
}

void processInput(int input)
{
    if (graphics)
        processGraphics(input);
    else
        processKey(input);
}

int main() {
    stdio_init_all();
    vreg_set_voltage(VREG_VOLTAGE_1_20);
    sleep_us(100);
    set_sys_clock_khz(266000, true);

    hub75.start(dma_complete);

    while (true) {
        hub75.background = 0;

        int c = getchar_timeout_us(0);
        processInput(c);

        if (graphics)
            renderScreenGraphics();
        else
            renderScreenText();

        hub75.flip(false); // Flip and clear to the background colour
        sleep_ms(1000 / 60);
    }
}
