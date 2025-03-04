// Simple State Machine Based Soda Dispenser
// GD4 and GD5 with special guest GD3
#include <TimeLib.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <FS.h>
#include <SD.h>
#include <AnimatedGIF.h>

TFT_eSPI tft = TFT_eSPI();
AnimatedGIF gif;

String gifFiles[] = {"/1.gif"};
int gifIndex = 0;

bool AnimateGIF = false;

// Touchscreen pins
#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 25   // T_CLK
#define XPT2046_CS 33    // T_CS

// SD Card Reader pins
#define SD_CS 5 // Chip Select

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define FONT_SIZE 2

// Define Button 1
#define BTN1_X 30
#define BTN1_Y 150
#define BTN1_WIDTH 240
#define BTN1_HEIGHT 35

// Define State Machine States
#define STATE_ACTIVE 1
#define STATE_STANDBY 2
#define STATE_VENDING 3

// Touchscreen coordinates: (x, y) and pressure (z)
int x, y, z;

// State Machine Current State
int currentState;

// State Machine New State
int newState = 0;

// Indicate State Change is in Progess
bool stateChange = false;

// Standby Timer Variables
#define STANDBY_DURATION 1
time_t standbyStart = now();

// Function to see if Button 1 has been pressed
// Inputs are X and Y from touchscreen press
// Returns true if X and Y are within Button 1 area
// Returns false otherwise
//
bool btn1_pressed(int x,int y) {
  // Check to see if the coordinates of the touch fall inside the range of the button
  if (((x>=BTN1_X) && (x<=(BTN1_X+BTN1_WIDTH))) && ((y>=BTN1_Y)&&(y<=(BTN1_Y+BTN1_HEIGHT)))) {
    Serial.println("Button 1 Pressed");
    return(true);
  } else {
    Serial.println("Button 1 Not Pressed");
    return(false);
  }
}

// Initialize SD Card Reader
bool SD_Init(int cs) {
  if (!SD.begin(cs, tft.getSPIinstance())) {
    Serial.println("Card Mount Failed");
    return(false);
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return(false);
  }

    Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  Serial.println("SD Card initialization Complete");
  return(true);
}

// Create Pointer for hardware timer instance
hw_timer_t *Timer0 = NULL;

// Interrupt handler for Timer0
void IRAM_ATTR onTimer0()
{
  Serial.println("Timer0 IRQ Handler Entered");

  // Checks if Touchscreen was touched, then checks if button pressed and changes state
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    // Touchscreen event has occurred
    // Get Touchscreen points
    TS_Point p = touchscreen.getPoint();
    // Calibrate Touchscreen points with map function to the correct width and height
    x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
    z = p.z;


    if (currentState == STATE_ACTIVE) {
      // Check and see if touch was pressing Button 1
      // If so, change state to vending
      if (btn1_pressed(x, y)) {
        changeState(STATE_VENDING);
      } 
    } else if (currentState == STATE_STANDBY) {
      changeState(STATE_ACTIVE);
    } else if (currentState == STATE_VENDING) {
      // **TEMP** -> Stop Animations if screen is touched
      AnimateGIF = false;
    }
  } else {    // No Touchscreen Event
    // Check and see if inactivity time has been exceeded and if so, go into standby state
    if ((currentState == STATE_ACTIVE) && ((minute(now()) - minute(standbyStart)) >= STANDBY_DURATION)) {
      changeState(STATE_STANDBY);
    }
  }
}

// changeState - Set Semaphores to signal loop() on state changes
void changeState(int ns) {
  stateChange = true;   // Signal state change is in progress
  newState = ns;        // Set new state
  AnimateGIF = false;   // Signal all animations to stop
}

// Setup Timer0
void Setup_Timer0() {
  Timer0 = timerBegin(1000000); // Timer Frequency in HZ, 1000000 = 1 MHZ
  timerAttachInterrupt(Timer0, &onTimer0);
    timerAlarm(Timer0, 100000,true, 0); // Set Timer to call handler every 100000 ticks = 100 milliseconds
}

// Function to dispense Soda
//
bool dispenseSoda() {
  // Put code here to drive motor and dispense soda
  return(true);
}

// Perform Actions for Vending State
void StateVending() {
  Serial.println("Entering State = Vending");
  
  // Set current state to Vending and mark stateChange as false
  currentState = STATE_VENDING;
  stateChange = false;

  // Stop Animations
  AnimateGIF = false;

  // Clear TFT screen 
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  // Prepare to Print to screen
  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT / 2;
  int textY = 80;
 
  // Print to screen
  String tempText = "Vending State";

  // Call Function to dispense Soda
  //
  dispenseSoda();

  tft.drawCentreString(tempText, centerX, textY, FONT_SIZE);

  PlayGIF(gifFiles[gifIndex], 5); // Play GIF off of SD card for 5 seconds
  // delay(1000);
  
  // Vending is complete - Set State to Active 
  changeState(STATE_ACTIVE);
}

// Perform Actions for Active State
void StateActive() {
  Serial.println("Entering State = Active");
  
  // Set current state to Active and mark stateChange as false
  currentState = STATE_ACTIVE;
  stateChange = false;

  // Stop Animations
  AnimateGIF = false;

  // Clear TFT screen 
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  // Prepare to Print to screen
  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT / 2;
  int textY = 80;
 
  // Print to screen
  String tempText = "Active State";
  tft.drawCentreString(tempText, centerX, textY, FONT_SIZE);
  tft.drawCentreString("Welcome to Jordes Sodaland", centerX, 30, FONT_SIZE);
  tft.drawCentreString("Press the button for a Soda!", centerX, centerY, FONT_SIZE);

  // Draw Button
  tft.drawRect( BTN1_X, BTN1_Y, BTN1_WIDTH, BTN1_HEIGHT, TFT_BLACK);

  // Start Time for Inactive Timer
  standbyStart = now();
}

// Perform Actions for StandBy State
void StateStandBy() {
  Serial.println("Entering State = StandBy");

  // Set current state to StandBy
  currentState = STATE_STANDBY;
  stateChange = false;

  // Stop Animations
  AnimateGIF = false;

  // Clear TFT screen 
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  // Prepare to Print to screen
  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT / 2;
  int textY = 80;
 
  // Print to screen
  String tempText = "StandBy State";
  tft.drawCentreString(tempText, centerX, textY, FONT_SIZE);
}

void setup() {
  Serial.begin(9600);

  // Start the SPI for the touchscreen and init the touchscreen
  //
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);

  // Set the Touchscreen rotation in landscape mode
  // Note: in some displays, the touchscreen might be upside down, so you might need to set the rotation to 3: touchscreen.setRotation(3);
  //
  touchscreen.setRotation(3);

  // Start the tft display
  //
  tft.init();

  // Set the TFT display rotation in landscape mode
  //
  tft.setRotation(1);

  // Clear the screen before writing to it
  //
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  // Initialize the gif decoder
  gif.begin(BIG_ENDIAN_PIXELS);

  // Initialize SD Card Reader
  if (SD_Init(SD_CS) == false) {
    Serial.println("SD Card Initialization Failed");
  }

  // Initialize Timer0 for IRQ driven events
  Setup_Timer0();
  
  // Set initial State to Active State
  changeState(STATE_ACTIVE);
}

void loop() {
  if (stateChange) {
    switch (newState) {
    case STATE_ACTIVE:
      StateActive();
      break;
    case STATE_STANDBY:
      StateStandBy();
      break;
    case STATE_VENDING:
      StateVending();
      break;
    }
  }
}

// ======================================================
// BEGIN - GIF Support Functions
//
bool PlayGIF(String gifFile, int seconds) {
  time_t startTime = now();

  Serial.println("PlayGIF: Entered");

  AnimateGIF = true;
  while (AnimateGIF == true) {
    if (gif.open(gifFile.c_str(), fileOpen, fileClose, fileRead, fileSeek, GIFDraw)) {
    #if defined(SERIALDEBUG)
        Serial.print("SUCCESS gif.open: ");
        Serial.println(gifFiles[gifIndex].c_str());
        Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
    #endif
      while (gif.playFrame(true, NULL)) {
        yield();
      }
      gif.close();
    } else {
      Serial.print("FAIL gif.open: ");
      Serial.println(gifFile.c_str());
      return (false);
    }
    if ((second(now()) - second(startTime)) >= seconds) {
      Serial.println("PlayGIF: Time Exceeded");
      AnimateGIF = false;
    }
    if (AnimateGIF == false) {
      return(true);
    }
  }
}

void* fileOpen(const char* filename, int32_t* pFileSize) {
  File* f = new File();
  *f = SD.open(filename, FILE_READ);
  if (*f) {
    *pFileSize = f->size();
    
#if defined(SERIALDEBUG)
    Serial.print("SUCCESS - fileopen: ");
    Serial.println(filename);
#endif

    return (void*)f;
  } else {
    Serial.print("FAIL - fileopen: ");
    Serial.println(filename);
    delete f;
    return NULL;
  }
}

void fileClose(void* pHandle) {
#if defined(SERIALDEBUG)
  Serial.println("ENTER - fileClose");
#endif

  File* f = static_cast<File*>(pHandle);
  if (f != NULL) {
    f->close();
    delete f;
  }
}

int32_t fileRead(GIFFILE* pFile, uint8_t* pBuf, int32_t iLen) {
#if defined(SERIALDEBUG)
  Serial.println("ENTER - fileRead");
#endif

  File* f = static_cast<File*>(pFile->fHandle);
  if (f == NULL) {
    Serial.println("f == NULL");
    return 0;
  }
#if defined(SERIALDEBUG)
  Serial.print("File* f= ");
  Serial.print(reinterpret_cast<uintptr_t> (f));
  Serial.print("\nCalling f->read");
  Serial.print("\npBuf: ");
  Serial.print(reinterpret_cast<uintptr_t> (pBuf));
  Serial.print("\niLen: ");
  Serial.print(iLen, HEX);
#endif

  int32_t iBytesRead = f->read(pBuf, iLen);
  pFile->iPos = f->position();
  
#if defined(SERIALDEBUG)
  Serial.print("iBytesRead = ");
  Serial.println(iBytesRead);
  Serial.print("iPos = ");
  Serial.println(pFile->iPos);
#endif

  return iBytesRead;
}

int32_t fileSeek(GIFFILE* pFile, int32_t iPosition) {
#if defined(SERIALDEBUG)
  Serial.println("ENTER - fileSeek");
#endif

  File* f = static_cast<File*>(pFile->fHandle);
  if (f == NULL) return 0;
  f->seek(iPosition);
  pFile->iPos = f->position();

#if defined(SERIALDEBUG)
  Serial.print("iPos = ");
  Serial.println(pFile->iPos);
#endif

  return pFile->iPos;
}

//
// END -- GIF Support Functions
// ======================================================

//
// Start of Code for GIF Renderer = GIFDraw()
//
// GIFDraw is called by AnimatedGIF library to draw frame to screen

#define DISPLAY_WIDTH  tft.width()
#define DISPLAY_HEIGHT tft.height()
#define BUFFER_SIZE 256            // Optimum is >= GIF width or integral division of width

#ifdef USE_DMA
  uint16_t usTemp[2][BUFFER_SIZE]; // Global to support DMA use
#else
  uint16_t usTemp[1][BUFFER_SIZE];    // Global to support DMA use
#endif
bool     dmaBuf = 0;
  
// Draw a line of image directly on the LCD
void GIFDraw(GIFDRAW *pDraw)
{
  uint8_t *s;
  uint16_t *d, *usPalette;
  int x, y, iWidth, iCount;

//  Serial.print("GIFDraw() Entered");


  // Display bounds check and cropping
  iWidth = pDraw->iWidth;
  if (iWidth + pDraw->iX > DISPLAY_WIDTH)
    iWidth = DISPLAY_WIDTH - pDraw->iX;
  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y; // current line
  if (y >= DISPLAY_HEIGHT || pDraw->iX >= DISPLAY_WIDTH || iWidth < 1)
    return;

  tft.startWrite();

  // Old image disposal
  s = pDraw->pPixels;
  if (pDraw->ucDisposalMethod == 2) // restore to background color
  {
    for (x = 0; x < iWidth; x++)
    {
      if (s[x] == pDraw->ucTransparent)
        s[x] = pDraw->ucBackground;
    }
    pDraw->ucHasTransparency = 0;
  }

  // Apply the new pixels to the main image
  if (pDraw->ucHasTransparency) // if transparency used
  {
    uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
    pEnd = s + iWidth;
    x = 0;
    iCount = 0; // count non-transparent pixels
    while (x < iWidth)
    {
      c = ucTransparent - 1;
      d = &usTemp[0][0];
      while (c != ucTransparent && s < pEnd && iCount < BUFFER_SIZE )
      {
        c = *s++;
        if (c == ucTransparent) // done, stop
        {
          s--; // back up to treat it like transparent
        }
        else // opaque
        {
          *d++ = usPalette[c];
          iCount++;
        }
      } // while looking for opaque pixels
      if (iCount) // any opaque pixels?
      {
        // DMA would degrtade performance here due to short line segments
        tft.setAddrWindow(pDraw->iX + x, y, iCount, 1);
        tft.pushPixels(usTemp, iCount);
        x += iCount;
        iCount = 0;
      }
      // no, look for a run of transparent pixels
      c = ucTransparent;
      while (c == ucTransparent && s < pEnd)
      {
        c = *s++;
        if (c == ucTransparent)
          x++;
        else
          s--;
      }
    }
  }
  else
  {
    s = pDraw->pPixels;

    // Unroll the first pass to boost DMA performance
    // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
    if (iWidth <= BUFFER_SIZE)
      for (iCount = 0; iCount < iWidth; iCount++) usTemp[dmaBuf][iCount] = usPalette[*s++];
    else
      for (iCount = 0; iCount < BUFFER_SIZE; iCount++) usTemp[dmaBuf][iCount] = usPalette[*s++];

#ifdef USE_DMA // 71.6 fps (ST7796 84.5 fps)
    tft.dmaWait();
    tft.setAddrWindow(pDraw->iX, y, iWidth, 1);
    tft.pushPixelsDMA(&usTemp[dmaBuf][0], iCount);
    dmaBuf = !dmaBuf;
#else // 57.0 fps
    tft.setAddrWindow(pDraw->iX, y, iWidth, 1);
    tft.pushPixels(&usTemp[0][0], iCount);
#endif

    iWidth -= iCount;
    // Loop if pixel buffer smaller than width
    while (iWidth > 0)
    {
      // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
      if (iWidth <= BUFFER_SIZE)
        for (iCount = 0; iCount < iWidth; iCount++) usTemp[dmaBuf][iCount] = usPalette[*s++];
      else
        for (iCount = 0; iCount < BUFFER_SIZE; iCount++) usTemp[dmaBuf][iCount] = usPalette[*s++];

#ifdef USE_DMA
      tft.dmaWait();
      tft.pushPixelsDMA(&usTemp[dmaBuf][0], iCount);
      dmaBuf = !dmaBuf;
#else
      tft.pushPixels(&usTemp[0][0], iCount);
#endif
      iWidth -= iCount;
    }
  }
  tft.endWrite();
} /* GIFDraw() */


