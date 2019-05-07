#include "MCUFRIEND_kbv.h"

MCUFRIEND_kbv tft;

#include "TouchScreen_kbv.h"         //my hacked version

const int XP=8,XM=A2,YP=A3,YM=9;    //240x320 ID=0x7575
const int TS_LEFT=888,TS_RT=161,TS_TOP=919,TS_BOT=111;

TouchScreen_kbv ts(XP, YP, XM, YM, 300);   //re-initialised after diagnose
TSPoint_kbv tp;                            //global point

#define LOWFLASH (defined(__AVR_ATmega328P__) && defined(MCUFRIEND_KBV_H_))

#define DICE_SIZE 64
#define DOT_RAD   6
#define BAR_W     12
#define BAR_GAP   6
#define BAR_BASE  300
#define SCREEN_W  260

#define RED       0xF800
#define WHITE     0xFFFF
#define GREY      0x8410
#define BLACK     0x0000

int globalCount = 0;
long diceOne;
long diceTwo;
int stats[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int bar_left = (SCREEN_W - (BAR_W + BAR_GAP) * 12) / 2;
float statsProcThreshold = 100/16;

//-----------------------------------------

void setup() {
  Serial.begin(9600);
  uint16_t ID = tft.readID();

  tft.begin(ID);
  tft.fillScreen(BLACK);
  
  rollTwoDices();
  printStats();
}

void loop() {
  bool justReleased = false;

  while (ISPRESSED() == true) {
    rollTwoDices();
    justReleased = true;
  }

  if (justReleased) {
    long seed = analogRead(5);
    randomSeed(seed);

    int extraTimes = random(6, 12);

    for (int i = 0; i <= extraTimes; i++) {
      rollTwoDices();
      delay(5);
    }

    storeStats();
    printStats();
  }
}

//-----------------------------------------

void readResistiveTouch(void) {
  tp = ts.getPoint();
  pinMode(YP, OUTPUT);      //restore shared pins
  pinMode(XM, OUTPUT);
  digitalWrite(YP, HIGH);  //because TFT control pins
  digitalWrite(XM, HIGH);
}

bool ISPRESSED(void) {
  // .kbv this was too sensitive !!
  // now touch has to be stable for 50ms
  int count = 0;
  bool state, oldstate;
  while (count < 10) {
    readResistiveTouch();
    state = tp.z > 200;     //ADJUST THIS VALUE TO SUIT YOUR SCREEN e.g. 20 ... 250
    if (state == oldstate) count++;
    else count = 0;
    oldstate = state;
    delay(5);
  }
  return oldstate;
}

void drawDice(int dice, int x, int y) {
  drawDiceBody(x, y);
  drawDots(dice, x, y);
}

void drawDiceBody(int x, int y) {
  tft.fillRect(x, y, DICE_SIZE, DICE_SIZE, RED);
}

void drawDots(int dice, int x, int y) {
  int y_top = y + DICE_SIZE / 4;
  int y_middle = y + DICE_SIZE / 2;
  int y_bottom = y + DICE_SIZE / 4 * 3;

  int x_left = x + DICE_SIZE / 4;
  int x_middle = x + DICE_SIZE / 2;
  int x_right = x + DICE_SIZE / 4 * 3;

  switch (dice)
  {
    case 1:
         tft.fillCircle(x_middle, y_middle, DOT_RAD, WHITE);
         break;
    case 2:
         tft.fillCircle(x_left, y_top, DOT_RAD, WHITE);
         tft.fillCircle(x_right, y_bottom, DOT_RAD, WHITE);
         break;
    case 3:
         tft.fillCircle(x_left, y_top, DOT_RAD, WHITE);
         tft.fillCircle(x_middle, y_middle, DOT_RAD, WHITE);
         tft.fillCircle(x_right, y_bottom, DOT_RAD, WHITE);
         break;
    case 4:
         tft.fillCircle(x_left, y_top, DOT_RAD, WHITE);
         tft.fillCircle(x_right, y_top, DOT_RAD, WHITE);
         tft.fillCircle(x_left, y_bottom, DOT_RAD, WHITE);
         tft.fillCircle(x_right, y_bottom, DOT_RAD, WHITE);
         break;
    case 5:
         tft.fillCircle(x_left, y_top, DOT_RAD, WHITE);
         tft.fillCircle(x_right, y_top, DOT_RAD, WHITE);
         tft.fillCircle(x_middle, y_middle, DOT_RAD, WHITE);
         tft.fillCircle(x_left, y_bottom, DOT_RAD, WHITE);
         tft.fillCircle(x_right, y_bottom, DOT_RAD, WHITE);
         break;
    case 6:
         tft.fillCircle(x_left, y_top, DOT_RAD, WHITE);
         tft.fillCircle(x_right, y_top, DOT_RAD, WHITE);
         tft.fillCircle(x_left, y_middle, DOT_RAD, WHITE);
         tft.fillCircle(x_right, y_middle, DOT_RAD, WHITE);
         tft.fillCircle(x_left, y_bottom, DOT_RAD, WHITE);
         tft.fillCircle(x_right, y_bottom, DOT_RAD, WHITE);
         break;
  }
}

void rollTwoDices() {
  long seed = analogRead(5);
//  Serial.println(seed);
  randomSeed(seed);
  
  diceOne = rollTheDice();
  diceTwo = rollTheDice();

  drawDice(diceOne, 30,  60);
  drawDice(diceTwo, 145, 60);
}

int rollTheDice() {
  int face = random(1, 100) % 3 + 1; // 1..3
  int direction = random(1, 100) % 2; // 0..1
  int dice = face + 3 * direction;

  if (dice < 1 ) {
    return 1;
  }

  if (dice > 6) {
    return 6;
  }

  return dice;
}

void storeStats() {
  int idx = diceOne + diceTwo - 2; // 0 based array starting with 2

  stats[idx] += 1;
  globalCount += 1;
}

void printStats() {
  int setColumn = 2;
  int num, idx;
  int maxVal = getMaxValue();

  for (int i = 0; i <= 10; i++) {
    printStatsBar(setColumn, stats[i], maxVal);
    setColumn += 1;
  }
}

void printStatsBar(int col, int val, int maxVal) {
  float proc;
  int x, y;

  if (maxVal == 0) {
    proc = 0;
  } else {
    proc = val * 100 / maxVal;
  }

  Serial.print(col);
  Serial.print(" >> ");
  Serial.println(proc);

  x = (col - 2) * (BAR_W + BAR_GAP) + bar_left;
  y = BAR_BASE - proc;

  // erase before
  tft.fillRect(x, BAR_BASE - 100, BAR_W, 100, GREY);  
  tft.fillRect(x, y, BAR_W, proc, WHITE);

  drawBarNum(x, BAR_BASE + 5, col);
  drawStatsCount(x, BAR_BASE, col);
}

void drawBarNum(int x, int y, int num) {
  int delta = 0;

  if (num < 10) {
    delta = 3;
  }
  
  tft.setCursor(x + delta, y);
  tft.setTextColor(WHITE);
  tft.print(num);
}

void drawStatsCount(int x, int y, int col) {
  int delta = 1;

  if (stats[col - 2] < 10) {
    delta = 3;
  }

  tft.setCursor(x + delta, y - 10);
  tft.setTextColor(BLACK);
  tft.print(stats[col - 2]);
}

int getMaxValue() {
  int max = 0;

  for (int i = 0; i <= 10; i++){
    if (max < stats[i]) {
      max = stats[i];
    }
  }
  return max;
}
