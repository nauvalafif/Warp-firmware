// devSD1331.c -----------------------------------------------------------------
//
// Purpose: Driver for the SSD1331 OLED screen, writeen purposefully for this
//          project. This means it only has the letters written reuqired for the
//		    4 measurements so this driver isn't suitable for other uses. However
//          it can be extended on to include  all other letters.
//
// Written by: B Brookes for the 4B25 weatherstation project
//
// Note: When screen co-ordinates are used the top left pixel when the pins are
//       at the top of the device are (0,0) written (column, row),
//       and the coordinates are in hexidecimal i.e. the screen is 96x64, that
//       is 5Fx3F in hex. Hex values are denoted as 0X
//
// -----------------------------------------------------------------------------

#include <stdint.h>
#include <unistd.h>

/*
 *	config.h needs to come first
 */
#include "config.h"

//  API incldues
#include "fsl_spi_master_driver.h"
#include "fsl_port_hal.h"
// Includes for warp firmware
#include "SEGGER_RTT.h"
#include "gpio_pins.h"
#include "warp.h"
#include "devSSD1331.h"

volatile uint8_t	inBuffer[32];
volatile uint8_t	payloadBytes[32];

/*
 *	Override Warp firmware's use of these pins and define new aliases.
 */
enum
{
    kSSD1331PinMOSI		= GPIO_MAKE_PIN(HW_GPIOA, 8),
    kSSD1331PinSCK		= GPIO_MAKE_PIN(HW_GPIOA, 9),
    kSSD1331PinCSn		= GPIO_MAKE_PIN(HW_GPIOB, 10),
    kSSD1331PinDC		= GPIO_MAKE_PIN(HW_GPIOA, 12),
    kSSD1331PinRST		= GPIO_MAKE_PIN(HW_GPIOB, 0),
};

static int
writeCommand(uint8_t commandByte)
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

// clearScreen(): Clears whatever is on the screen
void clearScreen()
{
    writeCommand(0x25);
    writeCommand(0x00);
    writeCommand(0x00);
    writeCommand(0x5F);
    writeCommand(0x3F);
}

// drawLine(start,end,colour): Draws a line from the start co-ordinate to end in
// the given colour
void drawLine(coord_t start, coord_t end, colour_t colour) //
{
    writeCommand(0x21);
    writeCommand(start.col);
    writeCommand(start.row);
    writeCommand(end.col);
    writeCommand(end.row);
    writeCommand(colour.red);
    writeCommand(colour.green);
    writeCommand(colour.blue);
}
// drawSquare(side_length, col, row, colour): draws a square with side length
// side_length that is not filled. The top left corner of the square is
// (col,row)
void drawSquare(int side_length, uint8_t col, uint8_t row, colour_t colour)
{
    writeCommand(0x22);
    writeCommand(col);
    writeCommand(row);
    writeCommand(col + side_length);
    writeCommand(row + side_length);
    writeCommand(colour.red);
    writeCommand(colour.green);
    writeCommand(colour.blue);
    writeCommand(0x00d);
    writeCommand(0x00d);
    writeCommand(0x00d);

}

// horizontalSegment(start,colour):
void horizontalSegment(coord_t start, colour_t colour)
{
    writeCommand(0x21);
    writeCommand(start.col);
    writeCommand(start.row);
    writeCommand(start.col + 7);
    writeCommand(start.row);
    writeCommand(colour.red);
    writeCommand(colour.green);
    writeCommand(colour.blue);
}

// verticalSegment(start, colour):
void verticalSegment(coord_t start, colour_t colour)
{
    writeCommand(0x21);
    writeCommand(start.col);
    writeCommand(start.row);
    writeCommand(start.col);
    writeCommand(start.row + 7);
    writeCommand(colour.red);
    writeCommand(colour.green);
    writeCommand(colour.blue);
}

// drawSegment(segment, col, row, colour): Numbers on the screen are written as
// a 7 segment display. This function writes the segments to the screen.
// The ASCI art on the right shows what each case in the switch statement
// writes one segment. The orgin of the number is the top left.
void drawSegment(int segment, uint8_t col, uint8_t row, colour_t colour)        //       3
{																				//    --------		ASCI art of the 7 segment display
    coord_t start;																//   |        |
    switch (segment)															// 1 |        | 2
    {																			//   |   0    |
        case 0:																	//    --------
            start.col = col + 1;												//   |        |
            start.row = row + 9;												// 4 |        | 5
            horizontalSegment(start, colour);									//   |        |
            break;																//    --------
            //		 6
        case 1:
            start.col = col + 0;
            start.row = row + 1;
            verticalSegment(start,colour);
            break;

        case 2:
            start.col = col + 9;
            start.row = row + 1;
            verticalSegment(start,colour);
            break;

        case 3:
            start.col = col + 1;
            start.row = row + 0;
            horizontalSegment(start, colour);
            break;

        case 4:
            start.col = col + 0;
            start.row = row + 0x0A;
            verticalSegment(start,colour);
            break;

        case 5:
            start.col = col + 9;
            start.row = row + 0x0A;
            verticalSegment(start,colour);
            break;

        case 6:
            start.col = col + 1;
            start.row = row + 0x12;
            horizontalSegment(start, colour);
            break;
    }
}

// drawNumber(number, col, row, colour): Draws the number using a 7 segment
// display in a colour. (col, row) is the top left of the number.
void drawNumber(char number, uint8_t col, uint8_t row, colour_t colour)
{
    switch (number) {
        case '0':
            drawSegment(1, col, row, colour);
            drawSegment(2, col, row, colour);
            drawSegment(3, col, row, colour);
            drawSegment(4, col, row, colour);
            drawSegment(5, col, row, colour);
            drawSegment(6, col, row, colour);
            break;

        case '1':
            drawSegment(2, col, row, colour);
            drawSegment(5, col, row, colour);
            break;

        case '2':
            drawSegment(3, col, row, colour);
            drawSegment(2, col, row, colour);
            drawSegment(0, col, row, colour);
            drawSegment(4, col, row, colour);
            drawSegment(6, col, row, colour);
            break;

        case '3':
            drawSegment(3, col, row, colour);
            drawSegment(2, col, row, colour);
            drawSegment(0, col, row, colour);
            drawSegment(5, col, row, colour);
            drawSegment(6, col, row, colour);
            break;

        case '4':
            drawSegment(0, col, row, colour);
            drawSegment(1, col, row, colour);
            drawSegment(2, col, row, colour);
            drawSegment(5, col, row, colour);
            break;

        case '5':
            drawSegment(0, col, row, colour);
            drawSegment(1, col, row, colour);
            drawSegment(3, col, row, colour);
            drawSegment(5, col, row, colour);
            drawSegment(6, col, row, colour);
            break;

        case '6':
            drawSegment(0, col, row, colour);
            drawSegment(1, col, row, colour);
            drawSegment(3, col, row, colour);
            drawSegment(4, col, row, colour);
            drawSegment(5, col, row, colour);
            drawSegment(6, col, row, colour);
            break;

        case '7':
            drawSegment(3, col, row, colour);
            drawSegment(2, col, row, colour);
            drawSegment(5, col, row, colour);
            drawSegment(6, col, row, colour);
            break;

        case '8':
            drawSegment(0, col, row, colour);
            drawSegment(1, col, row, colour);
            drawSegment(2, col, row, colour);
            drawSegment(3, col, row, colour);
            drawSegment(4, col, row, colour);
            drawSegment(5, col, row, colour);
            drawSegment(6, col, row, colour);
            break;

        case '9':
            drawSegment(0, col, row, colour);
            drawSegment(1, col, row, colour);
            drawSegment(2, col, row, colour);
            drawSegment(3, col, row, colour);
            drawSegment(5, col, row, colour);
            drawSegment(6, col, row, colour);
            break;

        case 'l': // Denote backward one for pressure reading
            drawSegment(1, col, row, colour);
            drawSegment(4, col, row, colour);
            break;

        case '-': // Denote minus sign for temperature
            drawSegment(0, col, row, colour);
            break;

    }
}

// drawSeperator(): Draws a horizontal line across the center of the screen to
// serpeate the two readings
void drawSeperator(void)
{
    // Draw green line across middle of screen
    colour_t SeperatorColour;
    SeperatorColour.red   = 0x00;
    SeperatorColour.green = 0x35d;
    SeperatorColour.blue  = 0x00d;

    coord_t SeperatorStart;
    SeperatorStart.row = 0x1F;
    SeperatorStart.col = 0x01;

    coord_t SeperatorEnd;
    SeperatorEnd.row = 0x1F;
    SeperatorEnd.col = 0x5F;

    drawLine(SeperatorStart, SeperatorEnd, SeperatorColour);
}

// drawLineShape(type, col, row, length, colour): drawsone of 4 types of line in
// the colour and of a particluar length.
void drawLineShape(char type, uint8_t col, uint8_t row, int length, colour_t colour)
{
    coord_t start;
    start.col = col;
    start.row = row;
    coord_t end;
    switch (type)
    {
        case '-': // Hoizontal line going from left to right
            end.col = col + length;
            end.row = row;
            drawLine(start, end, colour);
            break;

        case '|': // Vertical line
            end.col = col;
            end.row = row + length;
            drawLine(start, end, colour);
            break;

        case '/': // Diagonal line left to right upwards
            end.col = col + length - 0x1;
            end.row = row - length + 0x1;
            drawLine(start, end, colour);
            break;

        case 'Y': // Diagonal line right to left downwards
            end.col = col + length - 0x1;
            end.row = row + length - 0x1;
            drawLine(start, end, colour);
            break;

    }
}

// drawCricle(size, origin_col, origin_row, colour): draws a circle of two
// fixed sizes, either small for use in text or nig in used for units
void drawCircle(char size, uint8_t origin_col, uint8_t origin_row, colour_t colour)
{
    switch (size)
    {
        case 'S': // Small for text, orign is the center of the cricle
            drawLineShape('/', origin_col - 2, origin_row    , 3, colour);
            drawLineShape('/', origin_col    , origin_row + 2, 3, colour);
            drawLineShape('Y', origin_col - 2, origin_row    , 3, colour);
            drawLineShape('Y', origin_col    , origin_row - 2, 3, colour);
            break;

        case 'B': //Big for units, origin is the top left of the cirlce
            drawLineShape('/', origin_col    , origin_row + 3, 4, colour);
            drawLineShape('/', origin_col + 4, origin_row + 7, 4, colour);
            drawLineShape('Y', origin_col    , origin_row + 4, 4, colour);
            drawLineShape('Y', origin_col + 4, origin_row    , 4, colour);
            break;
    }
}

// drawLetter(letter, origin_col, origin_row, colour): Draws letter in order to
// spell a description of the reading being displayed on the screen. the origin
// is the top left of the letter if a rectangel was drawn to contain the letter.
void drawLetter(char letter, uint8_t origin_col, uint8_t origin_row, colour_t colour)
{
    coord_t LineStart;
    coord_t LineEnd;
    switch (letter)
    {
        case 'T':
            drawLineShape('-', origin_col    , origin_row, 6, colour);
            drawLineShape('|', origin_col + 3, origin_row, 6, colour);
            break;

        case 'E':
            drawLineShape('|', origin_col, origin_row    , 6, colour);
            drawLineShape('-', origin_col, origin_row    , 4, colour);
            drawLineShape('-', origin_col, origin_row + 3, 3, colour);
            drawLineShape('-', origin_col, origin_row + 6, 4, colour);
            break;

        case 'M':
            drawLineShape('|', origin_col    , origin_row    , 6, colour);
            drawLineShape('Y', origin_col    , origin_row    , 3, colour);
            drawLineShape('/', origin_col + 2, origin_row + 2, 3, colour);
            drawLineShape('|', origin_col + 4, origin_row    , 6, colour);
            break;

        case 'A':
            drawLineShape('|', origin_col    , origin_row + 2, 4, colour);
            drawLineShape('|', origin_col + 5, origin_row + 2, 4, colour);
            drawLineShape('-', origin_col    , origin_row + 4, 5, colour);
            drawLineShape('/', origin_col    , origin_row + 2, 3, colour);
            drawLineShape('Y', origin_col + 3, origin_row    , 3, colour);
            break;

        case 'U':
            drawLineShape('|', origin_col    , origin_row    , 5, colour);
            drawLineShape('-', origin_col + 1, origin_row + 6, 2, colour);
            drawLineShape('|', origin_col + 4, origin_row    , 5, colour);
            break;

        case 'S': // This ones a tricky one
            drawLineShape('-', origin_col + 1, origin_row    , 3, colour);
            drawLineShape('-', origin_col + 1, origin_row + 3, 2, colour);
            drawLineShape('-', origin_col    , origin_row + 6, 3, colour);
            drawLineShape('|', origin_col    , origin_row + 1, 1, colour);
            drawLineShape('|', origin_col + 4, origin_row + 4, 1, colour);
            break;

        case 'H':
            drawLineShape('|', origin_col    , origin_row    , 6, colour);
            drawLineShape('|', origin_col + 4, origin_row    , 6, colour);
            drawLineShape('-', origin_col    , origin_row + 4, 4, colour);
            break;

        case 'I':
            drawLineShape('|', origin_col    , origin_row    , 6, colour);
            break;

        case 'D':
            drawLineShape('|', origin_col    , origin_row    , 6, colour);
            drawLineShape('Y', origin_col + 1, origin_row    , 3, colour);
            drawLineShape('/', origin_col + 1, origin_row + 6, 3, colour);
            drawLineShape('|', origin_col + 3, origin_row + 2, 2, colour);
            break;

        case 'Y':
            drawLineShape('Y', origin_col    , origin_row    , 4, colour);
            drawLineShape('/', origin_col + 3, origin_row + 3, 4, colour);
            drawLineShape('|', origin_col + 3, origin_row + 3, 3, colour);
            break;

        case 'W':
            drawLineShape('|', origin_col    , origin_row    , 6, colour);
            drawLineShape('Y', origin_col + 2, origin_row + 4, 3, colour);
            drawLineShape('/', origin_col    , origin_row + 6, 3, colour);
            drawLineShape('|', origin_col + 4, origin_row    , 6, colour);
            break;

        case 'N':
            drawLineShape('|', origin_col    , origin_row    , 6, colour);
            drawLineShape('|', origin_col + 5, origin_row    , 6, colour);
            drawLineShape('-', origin_col    , origin_row    , 1, colour);
            drawLineShape('-', origin_col + 4, origin_row + 6, 1, colour);
            drawLineShape('Y', origin_col + 1, origin_row    , 2, colour);
            drawLineShape('Y', origin_col + 2, origin_row + 2, 2, colour);
            drawLineShape('Y', origin_col + 3, origin_row + 2, 2, colour);
            break;

        case 'P':
            drawLineShape('|', origin_col    , origin_row    , 6, colour);
            drawCircle   ('S', origin_col + 2, origin_row + 2,    colour);
            break;

        case 'R':
            drawLineShape('|', origin_col    , origin_row    , 6, colour);
            drawCircle   ('S', origin_col + 2, origin_row + 2,    colour);
            drawLineShape('Y', origin_col + 1, origin_row + 3, 4, colour);
            break;

        case':':
            drawLineShape('|', origin_col, origin_row + 1, 0, colour);
            drawLineShape('|', origin_col, origin_row + 5, 0, colour);
            break;


    }
}

// drawUnits(measurement, origin_col, origin_row, colour): draws out the units
// for the measuremnet being displayed. The orign is the trectangle that
// contains the units.
void drawUnits(char measurement, uint8_t origin_col, uint8_t origin_row, colour_t colour)
{
    switch (measurement)
    {
        case 'T': // Temperature in degrees C
            // Draw the degree sign
            drawSquare(2, origin_col, origin_row, colour);
            // Draw a large C
            drawLineShape('|', origin_col + 5 , origin_row + 2   , 0x0E, colour);
            drawLineShape('-', origin_col + 7 , origin_row       , 4   , colour);
            drawLineShape('-', origin_col + 7 , origin_row + 0x12, 3   , colour);
            drawLineShape('/', origin_col + 5 , origin_row + 2   , 3   , colour);
            drawLineShape('Y', origin_col + 0xB, origin_row      , 3   , colour);
            drawLineShape('Y', origin_col + 5 , origin_row + 0x10, 3   , colour);
            drawLineShape('/', origin_col + 0xB, origin_row + 0x12, 3  , colour);
            break;

        case 'P': // Pressure in mBar
            // Where gonna use the text to contrcut this to make life easier
            drawLetter('M', origin_col     , origin_row + 6, colour);
            drawLetter('D', origin_col + 6 , origin_row    , colour); // Two Ds make a B
            drawLetter('D', origin_col + 6 , origin_row + 6, colour);
            drawLetter('A', origin_col + 0xB, origin_row + 6, colour);
            drawLetter('R', origin_col + 0x12, origin_row + 6, colour);
            break;

        case 'H': // Humidity in %
            // Draw 2 cirlces
            drawSquare(2, origin_col + 1, origin_row + 2 , colour);
            drawSquare(2, origin_col + 5, origin_row + 0xC, colour);

            // Draw slash, done in
            uint8_t col_increment = 0;
            uint8_t row_increment = 0x10 ;
            while (row_increment > 0)
            {
                drawLineShape('/', origin_col + col_increment,
                              origin_row + row_increment, 2, colour);
                col_increment = col_increment + 0x1;  // Col goes 0,1,2,3,4,5,6,7
                row_increment = row_increment - 0x2;	// Row goes +16,+14, 12, 10, 8, 6, 4, 2
            }

            break;

        case 'W': // Wind speed in KPH
            // Draw a K
            drawLineShape('|', origin_col    , origin_row    , 0x13, colour);

            // Up part of the K
            row_increment = 0x8;
            col_increment = 0x1;
            int i = 3;
            while (i > 0)
            {
                drawLineShape('/', origin_col + col_increment,
                              origin_row + row_increment, 2, colour);
                col_increment = col_increment + 0x02;  // Col goes 0,1,2,3,4,5,6,7
                row_increment = row_increment - 0x03;	// Row goes +16,+14, 12, 10, 8, 6, 4, 2
                i = i - 1;
            }
            // Down part of the K
            row_increment = 0x9;
            col_increment =0x1;
            while (row_increment < 0x13)
            {
                drawLineShape('Y', origin_col + col_increment,
                              origin_row + row_increment, 2, colour);
                col_increment = col_increment + 0x2;  // Col goes 0,1,2,3,4,5,6,7
                row_increment = row_increment + 0x3;	// Row goes +16,+14, 12, 10, 8, 6, 4, 2
            }

            // Draw a P
            drawLineShape('|', origin_col + 0xA, origin_row    , 0x13, colour);
            drawCircle   ('B', origin_col + 0xB, origin_row    ,     colour);

            // Draw a H
            drawLineShape('|', origin_col + 0x15, origin_row     , 0x13, colour);
            drawLineShape('-', origin_col + 0x15, origin_row + 0xA, 5 , colour);
            drawLineShape('|', origin_col + 0x1A, origin_row     , 0x13, colour);

            break;
    }
}

// screen1(temperature, pressure): draw the screen that shows the temperature
// and pressure from the sensortag. Temperature and pressure are passed as
// strings so they can be split up and passed as character to the function
// drawNumber()
// Note: temperture is passed as +00.0 and pressure as 0000.0
void screen1(char temperature[], char pressure[])
{
    colour_t text_colour; 		// For the description text
    text_colour.red   = 0x35d;
    text_colour.green = 0x00d;
    text_colour.blue  = 0x00d;

    colour_t value_colour;		// For a measurement value and its units
    value_colour.red   = 0x00d;
    value_colour.green = 0x00d;
    value_colour.blue  = 0x35d;

    drawSeperator();

    // Temperature
    drawLetter('T', 0x01, 0x01, text_colour);
    drawLetter('E', 0x09, 0x01, text_colour);
    drawLetter('M', 0x0F, 0x01, text_colour);
    drawLetter('P', 0x15, 0x01, text_colour);
    drawLetter('E', 0x1B, 0x01, text_colour);
    drawLetter('R', 0x21, 0x01, text_colour);
    drawLetter('A', 0x27, 0x01, text_colour);
    drawLetter('T', 0x2D, 0x01, text_colour);
    drawLetter('U', 0x35, 0x01, text_colour);
    drawLetter('R', 0x3B, 0x01, text_colour);
    drawLetter('E', 0x41, 0x01, text_colour);
    drawLetter(':', 0x49, 0x01, text_colour);

    if (temperature[0] == '-')
    {
        drawNumber('-', 0x01, 0x13, value_colour);
    }

    if (temperature[1] != '0')
    {
        drawNumber(temperature[1], 0xD, 0xA, value_colour);
    }	// Tens, if they exist

    drawNumber(temperature[2], 0x19, 0xA, value_colour); 		// Units
    drawSquare(2, 0x25, 0x1B, value_colour);			   		// Decimal point
    drawNumber(temperature[4], 0x2B, 0xA, value_colour); 		// Tenths
    drawUnits('T', 0x3C, 0xA, value_colour);					// Measurement units


    // Pressure
    drawLetter('P', 0x01, 0x22, text_colour);
    drawLetter('R', 0x07, 0x22, text_colour);
    drawLetter('E', 0x0D, 0x22, text_colour);
    drawLetter('S', 0x13, 0x22, text_colour);
    drawLetter('S', 0x19, 0x22, text_colour);
    drawLetter('U', 0x1F, 0x22, text_colour);
    drawLetter('R', 0x25, 0x22, text_colour);
    drawLetter('E', 0x2B, 0x22, text_colour);
    drawLetter(':', 0x32, 0x22, text_colour);

    // Assume pressure comes as 0123.4
    if (pressure[0] == '1')
    {
        drawNumber('l', 0x01, 0x2C, value_colour);
    } // Thosands if they exist

    drawNumber(pressure[1],0x04, 0x2C, value_colour);	// 100s
    drawNumber(pressure[2],0x10, 0x2C, value_colour);	// 10s
    drawNumber(pressure[3],0x1C, 0x2C, value_colour);	// 0s

    drawUnits('P', 0x3A, 0x32, value_colour);
}

// screen2(humidity, WindSpeed): draw the screen that shows the humidity
// and wind speed from the sensortag. Humidity and wind speed are passed as
// strings so they can be split up and passed as character to the function
// drawNumber()
// Note: humidty is passed as 00.0 and wind speed as 00.0
void screen2(char humidity[], char WindSpeed[])
{
    colour_t text_colour; 		// For the description text
    text_colour.red   = 0x35d;
    text_colour.green = 0x00d;
    text_colour.blue  = 0x00d;

    colour_t value_colour;		// For a measurement value and its units
    value_colour.red   = 0x00d;
    value_colour.green = 0x00d;
    value_colour.blue  = 0x35d;

    drawSeperator();
    // Humidity
    drawLetter('H', 0x01, 0x01, text_colour);
    drawLetter('U', 0x07, 0x01, text_colour);
    drawLetter('M', 0x0D, 0x01, text_colour);
    drawLetter('I', 0x13, 0x01, text_colour);
    drawLetter('D', 0x15, 0x01, text_colour);
    drawLetter('I', 0x1A, 0x01, text_colour);
    drawLetter('T', 0x1C, 0x01, text_colour);
    drawLetter('Y', 0x24, 0x01, text_colour);
    drawLetter(':', 0x2D, 0x01, text_colour);

    // Write the humidty string passed to the function out
    drawNumber(humidity[0], 0x01, 0x0A, value_colour);  // 10s
    drawNumber(humidity[1], 0x0D, 0x0A, value_colour);	// 1s
    drawSquare(2, 0x1A, 0x1B, value_colour);			   		// Decimal point
    drawNumber(humidity[3], 0x1F, 0x0A, value_colour);	// 1/10th
    drawUnits('H', 0x41, 0x0A, value_colour);


    // Wind Speed
    drawLetter('W', 0x01, 0x22, text_colour);
    drawLetter('I', 0x07, 0x22, text_colour);
    drawLetter('N', 0x09, 0x22, text_colour);
    drawLetter('D', 0x10, 0x22, text_colour);
    drawLetter('S', 0x1A, 0x22, text_colour);
    drawLetter('P', 0x20, 0x22, text_colour);
    drawLetter('E', 0x26, 0x22, text_colour);
    drawLetter('E', 0x2C, 0x22, text_colour);
    drawLetter('D', 0x32, 0x22, text_colour);
    drawLetter(':', 0x39, 0x22, text_colour);

    if (WindSpeed[0] != '0')
        drawNumber(WindSpeed[0], 0x01, 0x2C, value_colour); 	// 10s, if they exist

    drawNumber(WindSpeed[1],0x0D, 0x2C, value_colour);	// 1s
    drawSquare(2, 0x1A, 0x3D, value_colour);
    drawNumber(WindSpeed[3],0x1F, 0x2C, value_colour);	// 1/10th
    drawUnits('W', 0x30, 0x2C, value_colour);
}

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
    PORT_HAL_SetMuxMode(PORTB_BASE, 0u, kPortMuxAsGpio);


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
     *	Initialization sequence, borrowed from https://github.com/adafruit/Adafruit-SSD1331-OLED-Driver-Library-for-Arduino
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
//	SEGGER_RTT_WriteString(0, "\r\n\tDone with initialization sequence...\n");

    /*
     *	To use fill commands, you will have to issue a command to the display to enable them. See the manual.
     */
    writeCommand(kSSD1331CommandFILL);
    writeCommand(0x01);
//	SEGGER_RTT_WriteString(0, "\r\n\tDone with enabling fill...\n");

    /*
     *	Clear Screen
     */
    writeCommand(kSSD1331CommandCLEAR);
    writeCommand(0x00);
    writeCommand(0x00);
    writeCommand(0x5F);
    writeCommand(0x3F);
//	SEGGER_RTT_WriteString(0, "\r\n\tDone with screen clear...\n");




//	SEGGER_RTT_WriteString(0, "\r\n\tDone with draw rectangle...\n");



    return 0;
}
