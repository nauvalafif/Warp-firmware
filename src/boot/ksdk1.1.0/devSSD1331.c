// Credit to Bailey Brookes: https://github.com/BaileyBrookes/Weatherstation

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

/*
 *	config.h needs to come first
 */
#include "config.h"

#include "fsl_spi_master_driver.h"
#include "fsl_port_hal.h"

#include "SEGGER_RTT.h"
#include "gpio_pins.h"
#include "warp.h"
#include "devSSD1331.h"

volatile uint8_t	inBuffer[1];
volatile uint8_t	payloadBytes[1];

/*
 *	Override Warp firmware's use of these pins and define new aliases.
 */
enum
{
	kSSD1331PinMOSI		= GPIO_MAKE_PIN(HW_GPIOA, 8),
	kSSD1331PinSCK		= GPIO_MAKE_PIN(HW_GPIOA, 9),
	kSSD1331PinCSn		= GPIO_MAKE_PIN(HW_GPIOB, 10),
	kSSD1331PinDC		= GPIO_MAKE_PIN(HW_GPIOA, 12),
	kSSD1331PinRST		= GPIO_MAKE_PIN(HW_GPIOB, 1),
};

static int writeCommand(uint8_t commandByte)
{
	spi_status_t status;

	/*
	 *	Drive /CS low.
	 *
	 *	Make sure there is a high-to-low transition by first driving high, delay, then drive low.
	 */
	GPIO_DRV_SetPinOutput(kSSD1331PinCSn);
	OSA_TimeDelay(10);
	GPIO_DRV_ClearPinOutput(kSSD1331PinCSn);

	/*
	 *	Drive DC low (command).
	 */
	GPIO_DRV_ClearPinOutput(kSSD1331PinDC);

	payloadBytes[0] = commandByte;
	status = SPI_DRV_MasterTransferBlocking(0	/* master instance */,
					NULL		/* spi_master_user_config_t */,
					(const uint8_t * restrict)&payloadBytes[0],
					(uint8_t * restrict)&inBuffer[0],
					1		/* transfer size */,
					1000		/* timeout in microseconds (unlike I2C which is ms) */);

	/*
	 *	Drive /CS high
	 */
	GPIO_DRV_SetPinOutput(kSSD1331PinCSn);

	return status;
}

// initiates the SSD1331 display
int devSSD1331init(void)
{
	/*
	 *	Override Warp firmware's use of these pins.
	 *
	 *	Re-configure SPI to be on PTA8 and PTA9 for MOSI and SCK respectively.
	 */
	PORT_HAL_SetMuxMode(PORTA_BASE, 8u, kPortMuxAlt3);
	PORT_HAL_SetMuxMode(PORTA_BASE, 9u, kPortMuxAlt3);

	warpEnableSPIpins();

	/*
	 *	Override Warp firmware's use of these pins.
	 *
	 *	Reconfigure to use as GPIO.
	 */
	PORT_HAL_SetMuxMode(PORTB_BASE, 10u, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTA_BASE, 12u, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTB_BASE, 1u, kPortMuxAsGpio);


	/*
	 *	RST high->low->high.
	 */
	GPIO_DRV_SetPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);
	GPIO_DRV_ClearPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);
	GPIO_DRV_SetPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);

	/*
	 *	Initialisation sequence, borrowed from https://github.com/adafruit/Adafruit-SSD1331-OLED-Driver-Library-for-Arduino
	 */
	writeCommand(kSSD1331CommandDISPLAYOFF);	// 0xAE
	writeCommand(kSSD1331CommandSETREMAP);		// 0xA0
	writeCommand(0x72);				// RGB Color
	writeCommand(kSSD1331CommandSTARTLINE);		// 0xA1
	writeCommand(0x0);
	writeCommand(kSSD1331CommandDISPLAYOFFSET);	// 0xA2
	writeCommand(0x0);
	writeCommand(kSSD1331CommandNORMALDISPLAY);	// 0xA4
	writeCommand(kSSD1331CommandSETMULTIPLEX);	// 0xA8
	writeCommand(0x3F);				// 0x3F 1/64 duty
	writeCommand(kSSD1331CommandSETMASTER);		// 0xAD
	writeCommand(0x8E);
	writeCommand(kSSD1331CommandPOWERMODE);		// 0xB0
	writeCommand(0x0B);
	writeCommand(kSSD1331CommandPRECHARGE);		// 0xB1
	writeCommand(0x31);
	writeCommand(kSSD1331CommandCLOCKDIV);		// 0xB3
	writeCommand(0xF0);				// 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
	writeCommand(kSSD1331CommandPRECHARGEA);	// 0x8A
	writeCommand(0x64);
	writeCommand(kSSD1331CommandPRECHARGEB);	// 0x8B
	writeCommand(0x78);
	writeCommand(kSSD1331CommandPRECHARGEA);	// 0x8C
	writeCommand(0x64);
	writeCommand(kSSD1331CommandPRECHARGELEVEL);	// 0xBB
	writeCommand(0x3A);
	writeCommand(kSSD1331CommandVCOMH);		// 0xBE
	writeCommand(0x3E);
	writeCommand(kSSD1331CommandMASTERCURRENT);	// 0x87
	writeCommand(0x06);
	writeCommand(kSSD1331CommandCONTRASTA);		// 0x81
	writeCommand(0x91);
	writeCommand(kSSD1331CommandCONTRASTB);		// 0x82
	writeCommand(0x50);
	writeCommand(kSSD1331CommandCONTRASTC);		// 0x83
	writeCommand(0x7D);
	writeCommand(kSSD1331CommandDISPLAYON);		// Turn on oled panel

	/*
	 *	To use fill commands, you will have to issue a command to the display to enable them. See the manual.
	 */
	writeCommand(kSSD1331CommandFILL);
	writeCommand(0x01);

	/*
	 *	Clear Screen
	 */
	writeCommand(kSSD1331CommandCLEAR);
	writeCommand(0x00);
	writeCommand(0x00);
	writeCommand(0x5F);
	writeCommand(0x3F);

	/*
	 *	Any post-initialization drawing commands go here.
	 */
    return 0;
}

// clears the screen
void clearScreen()
{
    writeCommand(0x25);
    writeCommand(0x00);
    writeCommand(0x00);
    writeCommand(0x5F);
    writeCommand(0x3F);
}

// draws a line from the start to end coordinates in the given colour
void drawLine(coord_t start, coord_t end, colour_t colour) //
{
    writeCommand(0x21);
    writeCommand(start.column);
    writeCommand(start.row);
    writeCommand(end.column);
    writeCommand(end.row);
    writeCommand(colour.red);
    writeCommand(colour.green);
    writeCommand(colour.blue);
}

// draws a square with the side length that is not filled. The top left corner of the square is (column ,row)
void drawSquare(int length, uint8_t column, uint8_t row, colour_t colour)
{
    writeCommand(0x22);
    writeCommand(column);
    writeCommand(row);
    writeCommand(column + length);
    writeCommand(row + length);
    writeCommand(colour.red);
    writeCommand(colour.green);
    writeCommand(colour.blue);
    writeCommand(0x00d);
    writeCommand(0x00d);
    writeCommand(0x00d);
}

// draws one of the four types of line in a particular colour and length
void drawLineShape(char type, uint8_t column, uint8_t row, int length, colour_t colour)
{
    coord_t start;
    start.column = column;
    start.row = row;
    coord_t end;
    switch (type)
    {
        case '-': // Horizontal line going from left to right
            end.column = column + length - 1;
            end.row = row;
            drawLine(start, end, colour);
            break;

        case '|': // Vertical line
            end.column = column;
            end.row = row + length - 1;
            drawLine(start, end, colour);
            break;

        case '/': // Diagonal line left to right upwards
            end.column = column + length - 0x1;
            end.row = row - length + 0x1;
            drawLine(start, end, colour);
            break;

        case 'Y': // Diagonal line left to right downwards
            end.column = column + length - 0x1;
            end.row = row + length - 0x1;
            drawLine(start, end, colour);
            break;
    }
}

// draws a circle of two fixed sizes, either small or big
void drawCircle(char size, uint8_t originColumn, uint8_t originRow, colour_t colour)
{
    switch (size)
    {
        case 'S': // Small circle, origin is the center of the circle
            drawLineShape('/', originColumn - 2, originRow    , 3, colour);
            drawLineShape('/', originColumn    , originRow + 2, 3, colour);
            drawLineShape('Y', originColumn - 2, originRow    , 3, colour);
            drawLineShape('Y', originColumn    , originRow - 2, 3, colour);
            break;

        case 'B': // Big circle, origin is the top left of the circle
            drawLineShape('/', originColumn    , originRow + 3, 4, colour);
            drawLineShape('/', originColumn + 4, originRow + 7, 4, colour);
            drawLineShape('Y', originColumn    , originRow + 4, 4, colour);
            drawLineShape('Y', originColumn + 4, originRow    , 4, colour);
            break;
    }
}

// draws a character
void drawCharacter(char character, uint8_t originColumn, uint8_t originRow, colour_t colour)
{
    switch (character)
    {
        case 'a':
        case 'A':
            drawLineShape('|', originColumn, originRow + 2, 5, colour);
            drawLineShape('|', originColumn + 4, originRow + 2, 5, colour);
            drawLineShape('-', originColumn, originRow + 3, 5, colour);
            drawLineShape('/', originColumn, originRow + 2, 3, colour);
            drawLineShape('Y', originColumn + 2, originRow    , 3, colour);
            break;

        case 'b':
        case 'B':
            drawLineShape('|', originColumn, originRow, 7, colour);
            drawLineShape('-', originColumn + 1, originRow, 3, colour);
            drawLineShape('-', originColumn + 1, originRow+3, 3, colour);
            drawLineShape('-', originColumn + 1, originRow+6, 3, colour);
            drawLineShape('|', originColumn+4, originRow+1, 2, colour);
            drawLineShape('|', originColumn+4, originRow+4, 2, colour);
            break;

        case 'c':
        case 'C':
            drawLineShape('|', originColumn, originRow+1, 5, colour);
            drawLineShape('-', originColumn + 1, originRow, 4, colour);
            drawLineShape('-', originColumn + 1, originRow+6, 4, colour);
            break;

        case 'd':
        case 'D':
            drawLineShape('|', originColumn    , originRow    , 6, colour);
            drawLineShape('Y', originColumn + 2, originRow    , 3, colour);
            drawLineShape('/', originColumn + 2, originRow + 6, 3, colour);
            drawLineShape('|', originColumn + 4, originRow + 2, 2, colour);
            drawLineShape('-', originColumn + 1, originRow, 1, colour);
            drawLineShape('-', originColumn + 1, originRow + 6, 1, colour);
            break;

        case 'e':
        case 'E':
            drawLineShape('|', originColumn, originRow    , 6, colour);
            drawLineShape('-', originColumn, originRow    , 5, colour);
            drawLineShape('-', originColumn, originRow + 3, 5, colour);
            drawLineShape('-', originColumn, originRow + 6, 5, colour);
            break;

        case 'f':
        case 'F':
            drawLineShape('|', originColumn, originRow    , 7, colour);
            drawLineShape('-', originColumn, originRow    , 5, colour);
            drawLineShape('-', originColumn, originRow + 3, 5, colour);
            break;

        case 'g':
        case 'G':
            drawLineShape('|', originColumn, originRow + 1, 5, colour);
            drawLineShape('-', originColumn + 1, originRow    , 4, colour);
            drawLineShape('-', originColumn + 1, originRow + 6, 4, colour);
            drawLineShape('-', originColumn + 3, originRow + 4, 2, colour);
            drawLineShape('|', originColumn + 4, originRow + + 5, 1, colour);
            break;

        case 'h':
        case 'H':
            drawLineShape('|', originColumn    , originRow    , 7, colour);
            drawLineShape('|', originColumn + 4, originRow    , 7, colour);
            drawLineShape('-', originColumn    , originRow + 3, 4, colour);
            break;

        case 'i':
        case 'I':
            drawLineShape('|', originColumn + 2, originRow, 7, colour);
            drawLineShape('-', originColumn, originRow    , 5, colour);
            drawLineShape('-', originColumn, originRow + 6    , 5, colour);
            break;

        case 'j':
        case 'J':
            drawLineShape('-', originColumn + 1, originRow    , 4, colour);
            drawLineShape('|', originColumn + 3    , originRow + 1   , 5, colour);
            drawLineShape('-', originColumn + 1, originRow + 6    , 2, colour);
            drawLineShape('|', originColumn    , originRow + 4   , 2, colour);
            break;

        case 'k':
        case 'K':
            drawLineShape('|', originColumn, originRow    , 7, colour);
            drawLineShape('Y', originColumn + 1, originRow + 3    , 4, colour);
            drawLineShape('/', originColumn + 1, originRow + 3, 4, colour);
            break;

        case 'l':
        case 'L':
            drawLineShape('|', originColumn, originRow    , 7, colour);
            drawLineShape('-', originColumn, originRow+6    , 5, colour);
            break;

        case 'm':
        case 'M':
            drawLineShape('|', originColumn    , originRow    , 7, colour);
            drawLineShape('Y', originColumn    , originRow    , 3, colour);
            drawLineShape('/', originColumn + 2, originRow + 2, 3, colour);
            drawLineShape('|', originColumn + 4, originRow    , 7, colour);
            break;

        case 'n':
        case 'N':
            drawLineShape('|', originColumn    , originRow    , 7, colour);
            drawLineShape('|', originColumn + 4, originRow    , 7, colour);
            drawLineShape('-', originColumn + 1, originRow + 1, 1, colour);
            drawLineShape('-', originColumn + 3, originRow + 5, 1, colour);
            drawLineShape('Y', originColumn + 1, originRow + 2, 3, colour);
            break;

        case 'o':
        case 'O':
            drawLineShape('|', originColumn, originRow + 1, 5, colour);
            drawLineShape('|', originColumn + 4, originRow + 1, 5, colour);
            drawLineShape('-', originColumn + 1, originRow, 3, colour);
            drawLineShape('-', originColumn + 1, originRow + 6, 3, colour);
            break;

        case 'p':
        case 'P':
            drawLineShape('|', originColumn, originRow, 7, colour);
            drawLineShape('|', originColumn + 4, originRow + 1, 2, colour);
            drawLineShape('-', originColumn + 1, originRow, 3, colour);
            drawLineShape('-', originColumn + 1, originRow + 3, 3, colour);
            break;

        case 'q':
        case 'Q':
            drawLineShape('|', originColumn, originRow + 1, 3, colour);
            drawLineShape('|', originColumn + 4, originRow + 1, 3, colour);
            drawLineShape('-', originColumn + 1, originRow, 3, colour);
            drawLineShape('-', originColumn + 1, originRow + 4, 3, colour);
            drawLineShape('|', originColumn + 2, originRow + 5, 2, colour);
            drawLineShape('-', originColumn + 3, originRow + 6, 2, colour);
            break;

        case 'r':
        case 'R':
            drawLineShape('|', originColumn, originRow, 7, colour);
            drawLineShape('|', originColumn + 4, originRow + 1, 2, colour);
            drawLineShape('-', originColumn + 1, originRow, 3, colour);
            drawLineShape('-', originColumn + 1, originRow + 3, 3, colour);
            drawLineShape('-', originColumn + 3, originRow + 6, 2, colour);
            drawLineShape('Y', originColumn + 1, originRow + 4, 2, colour);
            break;

        case 's':
        case 'S':
            drawLineShape('-', originColumn + 1, originRow, 4, colour);
            drawLineShape('-', originColumn + 1, originRow + 3, 3, colour);
            drawLineShape('-', originColumn, originRow + 6, 4, colour);
            drawLineShape('|', originColumn, originRow + 1, 2, colour);
            drawLineShape('|', originColumn + 4, originRow + 4, 2, colour);
            break;

        case 't':
        case 'T':
            drawLineShape('-', originColumn, originRow, 5, colour);
            drawLineShape('|', originColumn + 2, originRow + 1, 6, colour);
            break;

        case 'u':
        case 'U':
            drawLineShape('|', originColumn, originRow, 6, colour);
            drawLineShape('-', originColumn + 1, originRow + 6, 3, colour);
            drawLineShape('|', originColumn + 4, originRow, 6, colour);
            break;

        case 'v':
        case 'V':
            drawLineShape('|', originColumn, originRow, 3, colour);
            drawLineShape('|', originColumn + 4, originRow, 3, colour);
            drawLineShape('|', originColumn + 1, originRow + 3, 3, colour);
            drawLineShape('|', originColumn + 3, originRow + 3, 3, colour);
            drawLineShape('|', originColumn + 2, originRow + 6, 1, colour);
            break;

        case 'w':
        case 'W':
            drawLineShape('|', originColumn, originRow, 6, colour);
            drawLineShape('|', originColumn + 2, originRow, 6, colour);
            drawLineShape('|', originColumn + 4, originRow, 6, colour);
            drawLineShape('|', originColumn + 1, originRow + 6, 1, colour);
            drawLineShape('|', originColumn + 3, originRow + 6, 1, colour);
            break;

        case 'x':
        case 'X':
            drawLineShape('|', originColumn, originRow, 2, colour);
            drawLineShape('|', originColumn + 4, originRow, 2, colour);
            drawLineShape('|', originColumn, originRow + 5, 2, colour);
            drawLineShape('|', originColumn + 4, originRow + 5, 2, colour);
            drawLineShape('/', originColumn + 1, originRow + 4, 3, colour);
            drawLineShape('Y', originColumn + 1, originRow + 2, 3, colour);
            break;

        case 'y':
        case 'Y':
            drawLineShape('Y', originColumn, originRow + 1, 3, colour);
            drawLineShape('/', originColumn + 2, originRow + 3, 3, colour);
            drawLineShape('|', originColumn + 2, originRow + 3, 4, colour);
            drawLineShape('|', originColumn, originRow, 1, colour);
            drawLineShape('|', originColumn + 4, originRow, 1, colour);
            break;

        case 'z':
        case 'Z':
            drawLineShape('-', originColumn, originRow, 5, colour);
            drawLineShape('-', originColumn, originRow + 6, 5, colour);
            drawLineShape('/', originColumn, originRow + 5, 5, colour);
            break;

        case ':':
            drawSquare(3,originColumn + 1, originRow, colour);
            drawSquare(3,originColumn + 1, originRow + 4, colour);
            break;
    }
}

void printText(char *text) {
    colour_t text_colour; 		// For the description text
    text_colour.red   = 255;
    text_colour.green = 255;
    text_colour.blue  = 255;

    uint8_t c = 1;
    uint8_t r = 1;

    clearScreen();
    warpEnableSPIpins();

    for (int i = 0; i < strlen(text); i++) {
        if (text[i] == ' ') {
                c += 6;
        } else {
                if (c > 91) {
                    c = 1;
                    r += 8;
                }
                drawCharacter(text[i], c, r, text_colour);
                c += 6;
        }
    }

    warpDisableSPIpins();
}