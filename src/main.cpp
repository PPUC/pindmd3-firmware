
#include <Arduino.h>
#include <EEPROM.h>
#include <SmartMatrix.h>
#include "dfu.h"

#define AT_COMMAND                        0x41
#define FRAME_4_COLOUR		          0x30
#define FRAME_16_COLOUR		          0x31
#define FRAME_16_COLOUR_X		  0x32
#define FRAME_GLEDIATOR	                  0x1
#define FRAME_256_COLOUR	          0x2
#define SET_SETTING	                  0x20
#define GET_SETTING	                  0x21
#define SETTING_DEBUG		          0x22
#define SETTING_BRIGHTNESS		  0x23
#define SETTING_4SHADES                   0x24
#define SETTING_16SHADES                  0x25
#define SETTING_RAINBOW_SPEED		  0x26
#define SETTING_RESET    		  0x27
#define DLL_AUTHENTICATE                  0x43
#define GET_DEVICE_INFO	                  0x42


#define PACKET_SIZE_FRAME_4_COLOUR        1024+14
#define PACKET_SIZE_FRAME_16_COLOUR       2048+14
#define PACKET_SIZE_FRAME_16_COLOUR_X     2048+50
#define PACKET_SIZE_FRAME_256_COLOUR      12288
#define PACKET_SIZE_SET_SETTING	          20+3
#define PACKET_SIZE_GET_SETTING	          1+1
#define PACKET_SIZE_DLL_AUTHENTICATE      8+2
#define PACKET_SIZE_GET_DEVICE_INFO       1+1

#define USER_LED 13
#define WIDTH 128
#define HEIGHT 32

#define key "000000001111111122222222"

// DMD DEFAULT EEPROM SETTINGS
double FRAME_4_SHADES[4]   = {0,18,50,100};
double FRAME_16_SHADES[16] = {0,6,12,18,24,32,38,44,50,56,62,68,74,80,86,100};
uint8_t DMD_BRIGHTNESS     = 100;
double RAINBOW_SPEED       = 20;

const uint8_t FIRMWARE_AUTH_KEY[4] = { 0xed, 0x1b, 0x96, 0x08 };
const char firmwareRev[] = "REV-vPin-01015R";
const uint8_t logo[1024] = {255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,63,65,63,0,0,0,0,0,128,1,0,0,0,0,0,0,127,99,127,0,0,0,0,0,128,1,0,0,0,0,0,0,127,119,127,0,0,0,0,0,128,1,0,0,0,0,0,0,119,127,119,0,0,0,0,0,128,1,0,0,0,240,255,119,119,127,119,0,0,0,0,0,128,1,0,0,0,16,6,85,119,127,119,0,0,0,0,0,128,1,0,0,0,208,221,93,119,119,119,0,0,0,0,0,128,1,0,0,0,208,85,89,119,119,119,0,0,0,0,0,128,1,0,0,0,16,86,85,119,119,119,206,36,0,0,0,128,1,0,0,0,208,83,77,119,119,119,33,61,0,0,0,128,1,0,0,0,80,220,93,127,119,127,33,37,0,0,0,128,1,0,0,0,80,4,85,127,119,127,33,37,0,0,0,128,1,0,0,0,112,252,119,63,119,191,206,36,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,63,65,63,0,0,0,0,0,128,1,0,0,0,0,0,0,127,99,127,0,0,0,0,0,128,1,0,0,0,0,0,0,127,119,127,0,0,0,0,0,128,1,0,0,0,0,0,0,119,127,119,0,0,0,0,0,128,1,0,0,0,240,255,119,119,127,119,0,0,0,0,0,128,1,0,0,0,16,6,85,119,127,119,0,0,0,0,0,128,1,0,0,0,208,221,93,119,119,119,0,0,0,0,0,128,1,0,0,0,208,85,89,119,119,119,0,0,0,0,0,128,1,0,0,0,16,86,85,119,119,119,206,36,0,0,0,128,1,0,0,0,208,83,77,119,119,119,33,61,0,0,0,128,1,0,0,0,80,220,93,127,119,127,33,37,0,0,0,128,1,0,0,0,80,4,85,127,119,127,33,37,0,0,0,128,1,0,0,0,112,252,119,63,119,191,206,36,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255};

static uint8_t tmp_buf[PACKET_SIZE_FRAME_16_COLOUR_X];
static uint8_t tmp_buf_settings[20];
uint8_t commandByte = FRAME_4_COLOUR;
uint8_t prevFrameType = 0;
rgb24 previousFrameColour;
rgb24 shades[16];
double hueCounter = 0.65;
int hueDelay = 0;
boolean rainbowMode = 0;
boolean fromInit = 1;
boolean debug = 0;
boolean enabled = 0;
int frameCount = 0;
int ledCounter = 0;
int letCounter1 = 0;
boolean ledState = 1;
double rainbowSpeed;
boolean settings4shade = 1;

SmartMatrix matrix;




void setup() {
    uint8_t i;
    matrix.begin();

    // load default settings into eeprom
    if(EEPROM.read(0)==0xFF){
      // 0    BRIGHTNESS
      // 1    RAINBOW
      // 2-5  SHADES4
      // 6-21 SHADES16
      EEPROM.write(0,DMD_BRIGHTNESS);
      EEPROM.write(1,RAINBOW_SPEED);
      for(i=0; i<4; i++)
        EEPROM.write(i+2,FRAME_4_SHADES[i]);
      for(i=0; i<16; i++)
        EEPROM.write(i+6,FRAME_16_SHADES[i]);
    } else {
      // load settings from eeprom
      DMD_BRIGHTNESS        = EEPROM.read(0);
      RAINBOW_SPEED         = EEPROM.read(1);
      for(i=0; i<4; i++)
        FRAME_4_SHADES[i]   = EEPROM.read(i+2);
      for(i=0; i<16; i++)
        FRAME_16_SHADES[i]  = EEPROM.read(i+6);
    }

    rainbowSpeed = ((RAINBOW_SPEED*(255/100))/50000);
    matrix.setBrightness(255/2);
    matrix.setColorCorrection(cc24);

    // pinDMD start up logo
    memcpy(tmp_buf+13,logo,1024);

    // user led
    pinMode(USER_LED, OUTPUT);

    matrix.drawString(1, 1, {0xff, 0x45, 0x0}, "firmware:");
    matrix.drawString(65, 1, {0xff, 0x45, 0x0}, firmwareRev);
    matrix.swapBuffers(true);
    delay(500);

    char serial[24];
    sprintf(serial, "%08X%08X%08X", (SIM_UIDMH&0xfffffff)^0x6cccaa4, (SIM_UIDML&0xfffffff)^0x2166121, (SIM_UIDL&0xfffffff)^0x784c353);
    enabled = true;//memcmp(serial,key,24) == 0;

    matrix.drawString(1, 7, {0xff, 0x45, 0x0}, "authentication:");
    matrix.drawString(65, 7, {0xff, 0x45, 0x0}, enabled?"PASS":"FAIL");
    matrix.swapBuffers(true);
    delay(500);
    matrix.drawString(1, 13, {0xff, 0x45, 0x0}, "eeprom:");
    matrix.drawString(65, 13, {0xff, 0x45, 0x0}, "NOT FOUND");
    matrix.swapBuffers(true);
    delay(500);
    matrix.drawString(1, 19, {0xff, 0x45, 0x0}, "bluetooth:");
    matrix.drawString(65, 19, {0xff, 0x45, 0x0}, "NOT FOUND");
    matrix.swapBuffers(true);
    delay(1000);
    matrix.fillScreen({0,0,0});
    matrix.setBrightness(DMD_BRIGHTNESS*(255/250));
}

void clearUsbBuffer(){
   // clear usb buffer
    while(Serial.available()>0)
      Serial.read();
}


void pinMameDisplayUpdate() {
  if(!enabled)
    return;

  int offset =0;
  uint8_t x,y;
  uint8_t pixel;
  uint8_t byte1;
  uint8_t byte2;
  uint8_t byte3 = 0;
  uint8_t byte4 = 0;

  rgb24 *frame_buffer = matrix.backBuffer();

  for(y=0; y<32; y++){
    for(x=0; x<16; x++){
      if(commandByte==FRAME_16_COLOUR_X)
      {
        byte1 = tmp_buf[(y*16)+x+49];
        byte2 = tmp_buf[(y*16)+x+561];  // 512+49
        byte3 = tmp_buf[(y*16)+x+1073]; // 1024+49
        byte4 = tmp_buf[(y*16)+x+1585]; // 1536+49
      }
      else
      {
        byte1 = tmp_buf[(y*16)+x+13];
        byte2 = tmp_buf[(y*16)+x+525];    // 512+13

        if(commandByte==FRAME_16_COLOUR){
          byte3 = tmp_buf[(y*16)+x+1037]; // 1024+13
          byte4 = tmp_buf[(y*16)+x+1549]; // 1536+13
        }
      }

      for(int j=0; j<8; j++){
        pixel                 = (byte4 & 0x1)<<3 | (byte3 & 0x1)<<2 | (byte2 & 0x1)<<1 | (byte1 & 0x1);
        frame_buffer[offset]  = shades[pixel];

        byte1 >>= 1;
        byte2 >>= 1;
        byte3 >>= 1;
        byte4 >>= 1;
        offset++;
      }
    }
  }

  hueDelay = 500;
}

double threeway_max(double a, double b, double c) {
    return max(a, max(b, c));
}

double threeway_min(double a, double b, double c) {
    return min(a, min(b, c));
}

unsigned long hash(unsigned char *str, int len){
  unsigned long hash = 5381;
  int c;

  for (int i = 0; i < len; i++){
  c = *str++;
  hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }

  return hash;
}

/**
 * Converts an RGB color value to HSV. Conversion formula
 * adapted from http://en.wikipedia.org/wiki/HSV_color_space.
 * Assumes r, g, and b are contained in the set [0, 255] and
 * returns h, s, and v in the set [0, 1].
 *
 * @param   Number  r       The red color value
 * @param   Number  g       The green color value
 * @param   Number  b       The blue color value
 * @return  Array           The HSV representation
 */
void rgbToHsv(uint8_t r, uint8_t g, uint8_t b, double hsv[]) {
    double rd = (double) r/255;
    double gd = (double) g/255;
    double bd = (double) b/255;
    double max = threeway_max(rd, gd, bd), min = threeway_min(rd, gd, bd);
    double h, s, v = max;

    double d = max - min;
    s = max == 0 ? 0 : d / max;

    if (max == min) {
        h = 0; // achromatic
    } else {
        if (max == rd) {
            h = (gd - bd) / d + (gd < bd ? 6 : 0);
        } else if (max == gd) {
            h = (bd - rd) / d + 2;
        } else if (max == bd) {
            h = (rd - gd) / d + 4;
        }
        h /= 6;
    }

    hsv[0] = h;
    hsv[1] = s;
    hsv[2] = v;
}

/**
 * Converts an HSV color value to RGB. Conversion formula
 * adapted from http://en.wikipedia.org/wiki/HSV_color_space.
 * Assumes h, s, and v are contained in the set [0, 1] and
 * returns r, g, and b in the set [0, 255].
 *
 * @param   Number  h       The hue
 * @param   Number  s       The saturation
 * @param   Number  v       The value
 * @return  Array           The RGB representation
 */
rgb24 hsvToRgb(double h, double s, double v) {
    double r = 0;
    double g = 0;
    double b = 0;

    int i = int(h * 6);
    double f = h * 6 - i;
    double p = v * (1 - s);
    double q = v * (1 - f * s);
    double t = v * (1 - (1 - f) * s);

    switch(i % 6){
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }

    return {r*255,g*255,b*255};
}

void setShades() {
  // set new shades if changes from previous ones
  if( (previousFrameColour.red!=tmp_buf[1]) || (previousFrameColour.green!=tmp_buf[2]) || (previousFrameColour.blue!=tmp_buf[3]) || (prevFrameType!=commandByte)){
      // set cur colour to pre colour
    previousFrameColour = {tmp_buf[1],tmp_buf[2],tmp_buf[3]};

    double hsv[3];
    // convert dmd rgb colour to hsv
    rgbToHsv(tmp_buf[1],tmp_buf[2],tmp_buf[3],hsv);
    // calculate shades from the rgb colour
    switch(commandByte){
      case FRAME_4_COLOUR:
        for(int i=0; i<4; i++)
          shades[i] = hsvToRgb(hsv[0],hsv[1], hsv[2]*(FRAME_4_SHADES[i]/100) );
        break;

      case FRAME_16_COLOUR:
      case FRAME_16_COLOUR_X:
        for(int i=0; i<16; i++)
          shades[i] = hsvToRgb(hsv[0],hsv[1], hsv[2]*(FRAME_16_SHADES[i]/100) );
        break;
    }
  }
}

uint8_t colourize(){
  uint8_t rv = 0;
  if(!(tmp_buf[4]==0 && tmp_buf[5]==0 && tmp_buf[6]==0)){
    shades[3] = {tmp_buf[1],tmp_buf[2],tmp_buf[3]};
    shades[2] = {tmp_buf[4],tmp_buf[5],tmp_buf[6]};
    shades[1] = {tmp_buf[7],tmp_buf[8],tmp_buf[9]};
    shades[0] = {tmp_buf[10],tmp_buf[11],tmp_buf[12]};
    rv = 1;
  }
  return rv;
}

uint8_t colourize16(){
  shades[0] = {tmp_buf[1],tmp_buf[2],tmp_buf[3]};
  shades[1] = {tmp_buf[4],tmp_buf[5],tmp_buf[6]};
  shades[2] = {tmp_buf[7],tmp_buf[8],tmp_buf[9]};
  shades[3] = {tmp_buf[10],tmp_buf[11],tmp_buf[12]};
  shades[4] = {tmp_buf[13],tmp_buf[14],tmp_buf[15]};
  shades[5] = {tmp_buf[16],tmp_buf[17],tmp_buf[18]};
  shades[6]  = {tmp_buf[19],tmp_buf[20],tmp_buf[21]};
  shades[7]  = {tmp_buf[22],tmp_buf[23],tmp_buf[24]};
  shades[8]  = {tmp_buf[25],tmp_buf[26],tmp_buf[27]};
  shades[9]  = {tmp_buf[28],tmp_buf[29],tmp_buf[30]};
  shades[10]  = {tmp_buf[31],tmp_buf[32],tmp_buf[33]};
  shades[11]  = {tmp_buf[34],tmp_buf[35],tmp_buf[36]};
  shades[12]  = {tmp_buf[37],tmp_buf[38],tmp_buf[39]};
  shades[13]  = {tmp_buf[40],tmp_buf[41],tmp_buf[42]};
  shades[14]  = {tmp_buf[43],tmp_buf[44],tmp_buf[45]};
  shades[15]  = {tmp_buf[46],tmp_buf[47],tmp_buf[48]};

  return 1;
}

void updateSettingsDisplay(void){
  char str[20];
  int i;
  rgb24 settingShade = hsvToRgb(hueCounter,1, 1);

  memset(tmp_buf+13,0,2048);
  matrix.fillScreen({0,0,0});
  matrix.fillRectangle(0,0,128,6,settingShade);
  matrix.setFont(font3x5);

  switch(tmp_buf_settings[1]){
    case SETTING_DEBUG:
      matrix.drawString(1, 1, {0,0,0}, "DEVICE DEBUG");
      matrix.setFont(font6x10);
      matrix.drawString(1, 7, settingShade, tmp_buf_settings[2]==1?"ENABLED":"DISABLED");
      break;

    case SETTING_BRIGHTNESS:
      matrix.drawString(1, 1, {0,0,0}, "BRIGHTNESS");
      matrix.setFont(font6x10);
      sprintf(str, "%d%%", tmp_buf_settings[2]);
      matrix.drawString(1, 7, settingShade, str);
      break;

    case SETTING_RAINBOW_SPEED:
      matrix.drawString(1, 1, {0,0,0}, "RAINBOW SPEED");
      matrix.setFont(font6x10);
      sprintf(str, "%d%%", tmp_buf_settings[2]);
      matrix.drawString(1, 7, settingShade, str);
      break;

    case SETTING_4SHADES:
      matrix.drawString(1, 1, {0,0,0}, "4 SHADES");
      matrix.setFont(font6x10);
      sprintf(str, "Shade%d %d%%", tmp_buf_settings[2],tmp_buf_settings[3]);
      matrix.drawString(1, 7, settingShade, str);
      settings4shade = 1;
      break;

    case SETTING_16SHADES:
      matrix.drawString(1, 1, {0,0,0}, "16 SHADES");
      matrix.setFont(font6x10);
      sprintf(str, "Shade%d %d%%", tmp_buf_settings[2],tmp_buf_settings[3]);
      matrix.drawString(1, 7, settingShade, str);
      settings4shade = 0;
      break;
  }

  if(settings4shade){
    for(i=0; i<4; i++){
      shades[i] = hsvToRgb(hueCounter,1, (FRAME_4_SHADES[i]/100) );
      matrix.fillRectangle(0+(i*4),16,0+(i*4)+3,20,shades[i]);
    }
  } else {
    for(i=0; i<16; i++){
      shades[i] = hsvToRgb(hueCounter,1, (FRAME_16_SHADES[i]/100) );
      //matrix.fillRectangle(63+(i*4),25,63+(i*4)+4,29,shades[i]);
      matrix.fillRectangle(0+(i*4),16,0+(i*4)+3,20,shades[i]);
    }
  }
  matrix.setFont(font3x5);

  // add version text
  matrix.drawString(2, 25, settingShade, firmwareRev);
}

void setSetting(void){
  switch(tmp_buf_settings[1]){
    case SETTING_DEBUG:
      debug = tmp_buf_settings[2]?1:0;
      frameCount = 0;
      break;

    case SETTING_BRIGHTNESS:
      DMD_BRIGHTNESS = tmp_buf_settings[2];
      EEPROM.write(0,DMD_BRIGHTNESS);
      matrix.setBrightness(DMD_BRIGHTNESS*(255/250));
      break;

    case SETTING_RAINBOW_SPEED:
      RAINBOW_SPEED = tmp_buf_settings[2];
      EEPROM.write(1,RAINBOW_SPEED);
      rainbowSpeed = ((RAINBOW_SPEED*(255/100))/50000);
      break;

    case SETTING_4SHADES:
      FRAME_4_SHADES[tmp_buf_settings[2]] = tmp_buf_settings[3];
      EEPROM.write(tmp_buf_settings[2]+2,tmp_buf_settings[3]);
      break;

    case SETTING_16SHADES:
      FRAME_16_SHADES[tmp_buf_settings[2]] = tmp_buf_settings[3];
      EEPROM.write(tmp_buf_settings[2]+6,tmp_buf_settings[3]);
      break;
  }

  previousFrameColour = {0,0,0};
}

void getSettings(void){
    uint8_t out_buf[100];
    memset(out_buf,0,100);

    out_buf[0] = debug;
    out_buf[1] = DMD_BRIGHTNESS;
    out_buf[2] = RAINBOW_SPEED;
    for(int i=0; i<4; i++)
      out_buf[i+3] = (uint8_t)FRAME_4_SHADES[i];
    for(int i=0; i<16; i++)
      out_buf[i+7] = (uint8_t)FRAME_16_SHADES[i];

    Serial.write(out_buf,100);

    settings4shade = 1;
}

void updateRainbowMode() {
  if(!enabled)
    return;

  hueDelay--;
  if(hueDelay<=0){
    if(commandByte == SET_SETTING){
      updateSettingsDisplay();
    } else {
      // update shades
      if((commandByte == FRAME_4_COLOUR) || fromInit){
        for(int i=0; i<4; i++)
          shades[i] = hsvToRgb(hueCounter,1, (FRAME_4_SHADES[i]/100) );
      } else
      if(commandByte == FRAME_16_COLOUR){
        for(int i=0; i<16; i++)
          shades[i] = hsvToRgb(hueCounter,1, (FRAME_16_SHADES[i]/100) );
      }
      // update dmd
      pinMameDisplayUpdate();

      // add version text
      if(fromInit)
        matrix.drawString(2, 25, shades[3], firmwareRev);
    }

    // increment hue
    hueCounter+=rainbowSpeed;
    if(hueCounter==1)
      hueCounter=0;

    // update dmd
    matrix.swapBuffers(true);
  }
}

void authenticateDLL(void){
  char out_buf[20];
  memset(out_buf,0,20);

  uint8_t bufToHash[10];
  memcpy(bufToHash, tmp_buf + 1, 6);
  memcpy(bufToHash + 6, FIRMWARE_AUTH_KEY, 4);
  unsigned long sendHash = hash(bufToHash,10);

  out_buf[3] = (int)((sendHash >> 24) & 0xFF) ;
  out_buf[2] = (int)((sendHash >> 16) & 0xFF) ;
  out_buf[1] = (int)((sendHash >> 8) & 0XFF);
  out_buf[0] = (int)((sendHash & 0XFF));

  Serial.write(out_buf,20);
}

void getDeviceInformation(void){
  char out_buf[100];
  memset(out_buf,0,100);

  out_buf[0] = WIDTH;
  out_buf[1] = HEIGHT;
  memcpy(out_buf+2, firmwareRev, 16);

  Serial.write(out_buf,100);
}

void checkSerialCommands(void){
  int count = 0;
  char buf[100];
  memset(buf,0,100);

  while(Serial.available())
    buf[count++] = Serial.read();

  if(strstr(buf, "AT") != NULL) {

    if(!strcmp(buf, "AT")){
      Serial.println("OK");
    }

    char *tokens[10];
    uint8_t i = 0;

    tokens[i] = strtok(buf, "+");
    while (tokens[i])
      tokens[++i] = strtok(NULL,",");

    boolean query = (strstr(tokens[1], "?") != NULL);
    boolean params= (strstr(tokens[1], "=") != NULL);

    if(strstr(tokens[1], "RST") != NULL){
      CPU_RESTART;
    }else
    if(strstr(tokens[1], "GMR") != NULL){
      Serial.println(firmwareRev);
    }else
    if(strstr(tokens[1], "UPDATE") != NULL){
      digitalWriteFast(USER_LED, HIGH);
      enabled = 0;
      matrix.stop();
      upgrade_firmware();
    }else
    if(strstr(tokens[1], "KUID") != NULL){
      Serial.printf("KUID=%08X-%08X-%08X", (SIM_UIDMH&0xfffffff)^0x6cccaa4, (SIM_UIDML&0xfffffff)^0x2166121, (SIM_UIDL&0xfffffff)^0x784c353);
    }
  }
}

void recieveUSBPacket(){
  int packetSize = 0;
  rgb24 *frame_buffer = matrix.backBuffer();

  commandByte = Serial.peek();

  switch (commandByte){
    case FRAME_4_COLOUR:
      packetSize = PACKET_SIZE_FRAME_4_COLOUR-1;
      break;

    case FRAME_16_COLOUR:
      packetSize = PACKET_SIZE_FRAME_16_COLOUR-1;
      break;

    case FRAME_16_COLOUR_X:
      packetSize = PACKET_SIZE_FRAME_16_COLOUR_X-1;
      break;

    case FRAME_GLEDIATOR:
    case FRAME_256_COLOUR:
      packetSize = PACKET_SIZE_FRAME_256_COLOUR;

      uint8_t buf[12288];
      Serial.read();
      Serial.readBytes((char *) buf, packetSize);

      if(!enabled)
        break;

      memcpy(frame_buffer,buf,12288);
      return;
      break;

    case DLL_AUTHENTICATE:
      packetSize = PACKET_SIZE_DLL_AUTHENTICATE-1;
      break;

    case SET_SETTING:
      packetSize = PACKET_SIZE_SET_SETTING-1;
      Serial.readBytes((char *) tmp_buf_settings, packetSize);
      return;
      break;

    case GET_SETTING:
    case GET_DEVICE_INFO:
      Serial.read();
      return;
      break;

    case AT_COMMAND:
      checkSerialCommands();
      break;

    default:
      clearUsbBuffer();
      break;
  }

  // read in full packet from usb
  Serial.readBytes((char *) tmp_buf, packetSize);
}

void loop() {
  // user led
  ledCounter++;
  if(ledCounter>=(fromInit?800:50000)){
   ledCounter = 0;
   ledState=!ledState;
   if(letCounter1<10000 && fromInit)
     ledState=0;
   digitalWriteFast(USER_LED, ledState?HIGH:LOW);
  }
  letCounter1++;
  letCounter1%=20000;



  // rainbow cycle effect
  if(rainbowMode || fromInit || (commandByte == SET_SETTING))
    updateRainbowMode();

  // check usb coms
  if(Serial.available()==0)
    return;

  recieveUSBPacket();

  if(commandByte!=FRAME_GLEDIATOR){
    // check packet footer byte is correct
    if(Serial.read()!=commandByte){
      // clear usb buffer
      clearUsbBuffer();
      return;
    }
  }

  uint8_t colorized = 0;

  // switch to frame type
  switch(commandByte){
    case FRAME_4_COLOUR:
    case FRAME_16_COLOUR:
    case FRAME_16_COLOUR_X:
      if(commandByte==FRAME_4_COLOUR)
        colorized = colourize();
      if(commandByte==FRAME_16_COLOUR_X)
        colorized = colourize16();

      // enable rainbow mode
      rainbowMode = (tmp_buf[1] == 0 && tmp_buf[2] == 0 && tmp_buf[3] == 0);
      // set new shades if changes from previous ones
      if(!colorized && !rainbowMode)
        setShades();
      else
        previousFrameColour = {0,0,0};

      // draw frame
      prevFrameType = commandByte;
      fromInit = 0;
      ledCounter = 50000;
      pinMameDisplayUpdate();
      break;

    case FRAME_GLEDIATOR:
    case FRAME_256_COLOUR:
      fromInit = 0;
      rainbowMode = 0;
      ledCounter = 50000;
      break;

    case SET_SETTING:
      setSetting();
      break;

    case GET_SETTING:
      getSettings();
      break;

    case DLL_AUTHENTICATE:
      authenticateDLL();
      break;

    case GET_DEVICE_INFO:
      getDeviceInformation();
      break;
  }

  if(debug){
    char str[10];
    sprintf(str, "%d", frameCount++);
    matrix.drawString(1, 1, { 0xff, 0x45, 0x0 }, str);
  }

  matrix.swapBuffers(true);
}
