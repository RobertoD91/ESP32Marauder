//                            USER DEFINED SETTINGS
//   M5Core2 for AWS (M5Stack Core2 AWS IoT EduKit)
//
//   Panel:   ILI9342C, 320 x 240, driven through the ILI9341 driver with the
//            M5STACK option so TFT_eSPI uses the M5 init and rotation tables.
//            Rotation 0 then yields the 240 x 320 portrait frame the Marauder
//            full-screen UI is built for.
//   Bus:     VSPI, shared with the microSD slot (SD CS on GPIO 4).
//   Reset:   the panel reset line hangs off AXP192 GPIO4, not an ESP32 pin, so
//            TFT_RST is -1 and the reset pulse is issued by the PMU bring-up.
//   Backlight: AXP192 DC-DC3, so there is no backlight GPIO either.
//   Touch:   FT6336U on the internal I2C bus, handled by ft6336.h, not by
//            TFT_eSPI. TOUCH_CS stays undefined.

// ##################################################################################
//
// Section 1. Call up the right driver file and any options for it
//
// ##################################################################################

#define ILI9341_DRIVER

// Selects the M5Stack panel init sequence and rotation table (ILI9342C)
#define M5STACK

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// ##################################################################################
//
// Section 2. Define the pins that are used to interface with the display here
//
// ##################################################################################

#define TFT_MISO 38
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   5   // Chip select control pin
#define TFT_DC   15  // Data Command control pin
#define TFT_RST  -1  // Reset is driven by AXP192 GPIO4
#define TFT_BL   -1  // Backlight is AXP192 DC-DC3

// ##################################################################################
//
// Section 3. Define the fonts that are to be used here
//
// ##################################################################################

#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:-.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

#define SMOOTH_FONT

// ##################################################################################
//
// Section 4. Other options
//
// ##################################################################################

#define SPI_FREQUENCY  40000000

// Optional reduced SPI frequency for reading TFT
#define SPI_READ_FREQUENCY  20000000

// Leave the default VSPI port selected: the SD card shares this bus.
