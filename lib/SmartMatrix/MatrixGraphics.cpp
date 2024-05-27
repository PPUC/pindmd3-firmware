/*
 * SmartMatrix Library - Methods for interacting with background layer
 *
 * Copyright (c) 2014 Louis Beaudoin (Pixelmatix)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdlib.h>
#include "SmartMatrix.h"

rgb24 backgroundBuffer[2][MATRIX_HEIGHT][MATRIX_WIDTH];
unsigned char SmartMatrix::currentDrawBuffer = 0;
unsigned char SmartMatrix::currentRefreshBuffer = 1;
volatile bool SmartMatrix::swapPending = false;
bitmap_font *font = (bitmap_font *) &apple3x5;

// coordinates based on screen position, which is between 0-localWidth/localHeight
void SmartMatrix::getPixel(uint8_t x, uint8_t y, rgb24 *xyPixel) {
    copyRgb24(xyPixel, &backgroundBuffer[currentRefreshBuffer][y][x]);
}

// reads pixel from drawing buffer, not refresh buffer
rgb24 SmartMatrix::readPixel(int16_t x, int16_t y) {
    int hwx, hwy;

    // check for out of bounds coordinates
    if (x < 0 || y < 0 || x >= screenConfig.localWidth || y >= screenConfig.localHeight)
        return {0, 0, 0};

    // map pixel into hardware buffer before writing
    if (screenConfig.rotation == rotation0) {
        hwx = x;
        hwy = y;
    } else if (screenConfig.rotation == rotation180) {
        hwx = (DRAWING_WIDTH - 1) - x;
        hwy = (DRAWING_HEIGHT - 1) - y;
    } else if (screenConfig.rotation == rotation90) {
        hwx = (DRAWING_WIDTH - 1) - y;
        hwy = x;
    } else { /* if (screenConfig.rotation == rotation270)*/
        hwx = y;
        hwy = (DRAWING_HEIGHT - 1) - x;
    }

    return backgroundBuffer[currentDrawBuffer][hwy][hwx];
}

void SmartMatrix::drawPixel(int16_t x, int16_t y, rgb24 color) {
    int16_t rtx, rty, hwx, hwy;

    // check for out of bounds coordinates
    if (x < 0 || y < 0 || x >= screenConfig.localWidth || y >= screenConfig.localHeight)
        return;

    // map pixel into hardware buffer before writing
    if (screenConfig.rotation == rotation0) {
        rtx = x;
        rty = y;
    } else if (screenConfig.rotation == rotation180) {
        rtx = (DRAWING_WIDTH - 1) - x;
        rty = (DRAWING_HEIGHT - 1) - y;
    } else if (screenConfig.rotation == rotation90) {
        rtx = (DRAWING_WIDTH - 1) - y;
        rty = x;
    } else { /* if (screenConfig.rotation == rotation270)*/
        rtx = y;
        rty = (DRAWING_HEIGHT - 1) - x;
    }

    convertToHardwareXY(rtx, rty, &hwx, &hwy);
    copyRgb24(&backgroundBuffer[currentDrawBuffer][hwy][hwx], color);
}

#define SWAPint(X,Y) { \
        int temp = X ; \
        X = Y ; \
        Y = temp ; \
    }

// x0, x1, and y must be in bounds (0-localWidth/Height), x1 > x0
void SmartMatrix::drawHardwareHLine(int16_t x0, int16_t x1, int16_t y, rgb24 color) {
	int16_t i, hwx, hwy;

	for (i = x0; i <= x1; i++) {
		convertToHardwareXY(i, y, &hwx, &hwy);
		copyRgb24(&backgroundBuffer[currentDrawBuffer][hwy][hwx], color);
	}
}

// x, y0, and y1 must be in bounds (0-localWidth/Height), y1 > y0
void SmartMatrix::drawHardwareVLine(int16_t x, int16_t y0, int16_t y1, rgb24 color) {
	int16_t i, hwx, hwy;

	for (i = y0; i <= y1; i++) {
		convertToHardwareXY(x, i, &hwx, &hwy);
		copyRgb24(&backgroundBuffer[currentDrawBuffer][hwy][hwx], color);
	}
}

void SmartMatrix::drawFastHLine(int16_t x0, int16_t x1, int16_t y, rgb24 color) {
	// make sure line goes from x0 to x1
	if (x1 < x0)
		SWAPint(x1, x0);

	// check for completely out of bounds line
	if (x1 < 0 || x0 >= screenConfig.localWidth || y < 0 || y >= screenConfig.localHeight)
		return;

	// truncate if partially out of bounds
	if (x0 < 0)
		x0 = 0;

	if (x1 >= screenConfig.localWidth)
		x1 = screenConfig.localWidth - 1;

	// map to hardware drawline function
	if (screenConfig.rotation == rotation0) {
		drawHardwareHLine(x0, x1, y, color);
	}
	else if (screenConfig.rotation == rotation180) {
		drawHardwareHLine((DRAWING_WIDTH - 1) - x1, (DRAWING_WIDTH - 1) - x0, (DRAWING_HEIGHT - 1) - y, color);
	}
	else if (screenConfig.rotation == rotation90) {
		drawHardwareVLine((DRAWING_WIDTH - 1) - y, x0, x1, color);
	}
	else { /* if (screenConfig.rotation == rotation270)*/
		drawHardwareVLine(y, (DRAWING_HEIGHT - 1) - x1, (DRAWING_HEIGHT - 1) - x0, color);
	}
}

void SmartMatrix::drawFastVLine(int16_t x, int16_t y0, int16_t y1, rgb24 color) {
	// make sure line goes from y0 to y1
	if (y1 < y0)
		SWAPint(y1, y0);

	// check for completely out of bounds line
	if (y1 < 0 || y0 >= screenConfig.localHeight || x < 0 || x >= screenConfig.localWidth)
		return;

	// truncate if partially out of bounds
	if (y0 < 0)
		y0 = 0;

	if (y1 >= screenConfig.localHeight)
		y1 = screenConfig.localHeight - 1;

	// map to hardware drawline function
	if (screenConfig.rotation == rotation0) {
		drawHardwareVLine(x, y0, y1, color);
	}
	else if (screenConfig.rotation == rotation180) {
		drawHardwareVLine((DRAWING_WIDTH - 1) - x, (DRAWING_HEIGHT - 1) - y1, (DRAWING_HEIGHT - 1) - y0, color);
	}
	else if (screenConfig.rotation == rotation90) {
		drawHardwareHLine((DRAWING_WIDTH - 1) - y1, (DRAWING_WIDTH - 1) - y0, x, color);
	}
	else { /* if (screenConfig.rotation == rotation270)*/
		drawHardwareHLine(y0, y1, (DRAWING_HEIGHT - 1) - x, color);
	}
}

void SmartMatrix::drawRectangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, rgb24 color) {
    drawFastHLine(x0, x1, y0, color);
    drawFastHLine(x0, x1, y1, color);
    drawFastVLine(x0, y0, y1, color);
    drawFastVLine(x1, y0, y1, color);
}

void SmartMatrix::fillRectangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, rgb24 color) {
    int i;
    for (i = y0; i <= y1; i++) {
        drawFastHLine(x0, x1, i, color);
    }
}

void SmartMatrix::fillScreen(rgb24 color) {
    fillRectangle(0, 0, screenConfig.localWidth, screenConfig.localHeight, color);
}

bool getBitmapPixelAtXY(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t *bitmap) {
    int cell = (y * ((width / 8) + 1)) + (x / 8);

    uint8_t mask = 0x80 >> (x % 8);
    return (mask & bitmap[cell]);
}

void SmartMatrix::setFont(fontChoices newFont) {
    font = (bitmap_font *)fontLookup(newFont);
}

void SmartMatrix::drawChar(int16_t x, int16_t y, rgb24 charColor, char character) {
    int xcnt, ycnt;

    for (ycnt = 0; ycnt < font->Height; ycnt++) {
        for (xcnt = 0; xcnt < font->Width; xcnt++) {
            if (getBitmapFontPixelAtXY(character, xcnt, ycnt, font)) {
                drawPixel(x + xcnt, y + ycnt, charColor);
            }
        }
    }
}

void SmartMatrix::drawString(int16_t x, int16_t y, rgb24 charColor, const char text[]) {
    int xcnt, ycnt, i = 0, offset = 0;
    char character;

    // limit text to 10 chars, why?
    for (i = 0; i < 50; i++) {
        character = text[offset++];
        if (character == '\0')
            return;

        for (ycnt = 0; ycnt < font->Height; ycnt++) {
            for (xcnt = 0; xcnt < font->Width; xcnt++) {
                if (getBitmapFontPixelAtXY(character, xcnt, ycnt, font)) {
                    drawPixel(x + xcnt, y + ycnt, charColor);
                }
            }
        }
        x += font->Width;
    }
}

void SmartMatrix::drawMonoBitmap(int16_t x, int16_t y, uint8_t width, uint8_t height, rgb24 bitmapColor, uint8_t *bitmap) {
    int xcnt, ycnt;

    for (ycnt = 0; ycnt < height; ycnt++) {
        for (xcnt = 0; xcnt < width; xcnt++) {
            if (getBitmapPixelAtXY(xcnt, ycnt, width, height, bitmap)) {
                drawPixel(x + xcnt, y + ycnt, bitmapColor);
            }
        }
    }
}

void SmartMatrix::handleBufferSwap(void) {
    if (!swapPending)
        return;

    unsigned char newDrawBuffer = currentRefreshBuffer;

    currentRefreshBuffer = currentDrawBuffer;
    currentDrawBuffer = newDrawBuffer;

    swapPending = false;
}

// waits until swap is complete before returning
void SmartMatrix::swapBuffers(bool copy) {
    swapPending = true;

    while (swapPending);

    if (copy)
        memcpy(&backgroundBuffer[currentDrawBuffer], &backgroundBuffer[currentRefreshBuffer], sizeof(backgroundBuffer[0]));
}

// return pointer to start of currentDrawBuffer, so application can do efficient loading of bitmaps
rgb24 *SmartMatrix::backBuffer() {
	return &backgroundBuffer[currentDrawBuffer][0][0];
}

