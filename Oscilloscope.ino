#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>

MCUFRIEND_kbv tft;

/*
------- SETTINGS -------
*/

#define ANALOG_PIN A5
#define TFT_WIDTH 320
#define TFT_HEIGHT 240

#define MINPRESSURE 200
#define MAXPRESSURE 1000

#define BUTTON_DELAY_ms 300

const int XP=6,XM=A2,YP=A1,YM=7; //240x320 ID=0x9341
const int TS_LEFT=177,TS_RT=915,TS_TOP=952,TS_BOT=204;
// ----------------------------------------

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define VOLTAGE_STEP 2
#define CHART_HEIGHT 200
#define OFFSET_Y -5
#define VOLTAGE_TEXT_OFFSET_Y -10
#define MAX_VOLTAGES_CAPACITY 1500
#define TIME_STEP_DELTA 20
#define TIME_STEP_DELTA_us 100
#define TIME_STEP_DEFAULT 100

#define INDICATORS_BASE_Y 2

#define TIME_STEP_TEXT_X 180
#define LAST_VOLTAGE_TEXT_X 2
#define AVG_VOLTAGE_TEXT_X 2
#define PEAK_VOLTAGE_TEXT_X 82
#define MIN_VOLTAGE_TEXT_X 82

bool isTimeStepMicros = false;

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

uint8_t lowBound = 0;
uint8_t peakBound = 5;

//uint8_t voltages[MAX_VOLTAGES_CAPACITY];
uint16_t voltagesAmount = TFT_WIDTH / VOLTAGE_STEP; 
uint16_t currentArrayIdx = 0;
uint16_t timeStep = 100;

uint16_t previousX = 0;
uint16_t lastAnalogReading = 0;
uint16_t previousY;

double voltagesSum = 0;
double peakVoltage = 0;
double minVoltage = 999;

uint16_t voltagesAmountMeasured = 0;

Adafruit_GFX_Button pauseButton, timeStepUpButton, timeStepDownButton;

void setup() {
  pinMode(ANALOG_PIN, INPUT);
  tft.reset();
  tft.begin();  
  tft.setRotation(1);
  resetScreen(); 
  
  Serial.begin(57600);
}

uint16_t touch_x, touch_y;     
bool Touch_getXY(void)
{
    TSPoint p = ts.getPoint();
    pinMode(YP, OUTPUT);      //restore shared pins
    pinMode(XM, OUTPUT);
    digitalWrite(YP, HIGH);   //because TFT control pins
    digitalWrite(XM, HIGH);
    bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);
    if (pressed) {
        int16_t temp = p.x;
        p.x = p.y;
        p.y = temp;
        touch_x = map(p.x, TS_LEFT, TS_RT, 0, tft.width()); 
        touch_y = map(p.y, TS_TOP, TS_BOT, 0, tft.height());
        touch_x = TFT_WIDTH - touch_x;
    }
    
    return pressed;
}

void resetScreen()
{
  tft.fillScreen(BLACK);
  uint16_t step = CHART_HEIGHT / (peakBound);
  for (uint8_t i = 0; i <= peakBound; i++)
  {
    uint16_t y = TFT_HEIGHT - step * i + OFFSET_Y;    
    tft.drawFastHLine(0, y, TFT_WIDTH, RED);
    tft.drawChar(0, y + VOLTAGE_TEXT_OFFSET_Y,(char)(i + 48), BLUE, BLACK, 1);
  }  
  pauseButton.initButton(&tft, 300, 10, 20, 20, GREEN, BLACK, CYAN, "P", 1);
  timeStepUpButton.initButton(&tft, 270, 10, 20, 20, GREEN, BLACK, CYAN, "+", 1);
  timeStepDownButton.initButton(&tft, 240, 10, 20, 20, GREEN, BLACK, CYAN, "-", 1);

  timeStepUpButton.drawButton();
  timeStepDownButton.drawButton();
  pauseButton.drawButton();
  writeTimeStep();

  voltagesSum = 0;
  voltagesAmountMeasured = 0;
  peakVoltage = 0;
  minVoltage = 999;
}

void writeTimeStep()
{  
  tft.fillRect(TIME_STEP_TEXT_X, 10, 40, 10, BLACK);
  tft.setCursor(TIME_STEP_TEXT_X, 10);  
  tft.setTextColor(WHITE);
  tft.setTextSize(1);  
  tft.print(timeStep);
  if (isTimeStepMicros)
    tft.print(" us");  
  else
    tft.print(" ms");  
}

void writeLastVoltage(int analogReading)
{
  //tft.fillRect(LAST_VOLTAGE_TEXT_X, INDICATORS_BASE_Y + 10, 40, 10, BLACK);
  tft.setCursor(LAST_VOLTAGE_TEXT_X, INDICATORS_BASE_Y + 10);  
  tft.setTextColor(WHITE);
  tft.setTextSize(1);  
  tft.print("Curr:");
  tft.print(analogToVoltage(analogReading));
  tft.print(" V");  
}

void writeAvgVoltage()
{
  //tft.fillRect(AVG_VOLTAGE_TEXT_X, INDICATORS_BASE_Y, 70, 10, BLACK);
  tft.setCursor(AVG_VOLTAGE_TEXT_X, INDICATORS_BASE_Y);  
  tft.setTextColor(WHITE);
  tft.setTextSize(1);    
  tft.print("Avg: "); 
  tft.print((float)voltagesSum / voltagesAmountMeasured);
  tft.print(" V"); 
}

void writePeakVoltage()
{
  //tft.fillRect(PEAK_VOLTAGE_TEXT_X, INDICATORS_BASE_Y, 70, 10, BLACK);
  tft.setCursor(PEAK_VOLTAGE_TEXT_X, INDICATORS_BASE_Y);  
  tft.setTextColor(WHITE);
  tft.setTextSize(1);    
  tft.print("Peak: "); 
  tft.print(peakVoltage);
  tft.print(" V"); 
}

void writeMinVoltage()
{
  //tft.fillRect(MIN_VOLTAGE_TEXT_X, INDICATORS_BASE_Y + 10, 70, 10, BLACK);
  tft.setCursor(MIN_VOLTAGE_TEXT_X, INDICATORS_BASE_Y + 10);  
  tft.setTextColor(WHITE);
  tft.setTextSize(1);    
  tft.print("Min: "); 
  tft.print(minVoltage);
  tft.print(" V"); 
}

float analogToVoltage(int analogReading)
{
  return analogReading / 1023.0 * peakBound;
}

bool isFirstIteration = true;
bool isPaused = false;
uint64_t startMicros = micros();
void loop() {
  bool isScreenPressed = Touch_getXY();
  pauseButton.press(isScreenPressed && pauseButton.contains(touch_x, touch_y));
  timeStepUpButton.press(isScreenPressed && timeStepUpButton.contains(touch_x, touch_y));
  timeStepDownButton.press(isScreenPressed && timeStepDownButton.contains(touch_x, touch_y));

  if (pauseButton.justReleased()) pauseButton.drawButton();
  if (timeStepUpButton.justReleased()) timeStepUpButton.drawButton();
  if (timeStepDownButton.justReleased()) timeStepDownButton.drawButton();

  if (pauseButton.justPressed()) 
  {
    pauseButton.drawButton(true);
    isPaused = !isPaused;
    if (isPaused) 
    {
      writeLastVoltage(lastAnalogReading);
      writeAvgVoltage();
      writePeakVoltage();
      writeMinVoltage();
    }
    else 
    {
      tft.fillRect(LAST_VOLTAGE_TEXT_X, INDICATORS_BASE_Y + 10, 80, 10, BLACK);
      tft.fillRect(AVG_VOLTAGE_TEXT_X, INDICATORS_BASE_Y, 80, 10, BLACK);  
      tft.fillRect(MIN_VOLTAGE_TEXT_X, INDICATORS_BASE_Y + 10, 80, 10, BLACK);     
      tft.fillRect(PEAK_VOLTAGE_TEXT_X, INDICATORS_BASE_Y, 80, 10, BLACK);
    }
    delay(BUTTON_DELAY_ms);
  }
  if (timeStepUpButton.justPressed())
  {
    timeStepUpButton.drawButton(true);
    if (timeStep + TIME_STEP_DELTA > 1000) 
    {
      isTimeStepMicros = false;
      timeStep = 20;
    }
    else timeStep += (isTimeStepMicros ? TIME_STEP_DELTA_us : TIME_STEP_DELTA);
    writeTimeStep();    
    delay(BUTTON_DELAY_ms);
  }
  if (timeStepDownButton.justPressed())
  {
    timeStepDownButton.drawButton(true);
    if (static_cast<int16_t>(timeStep) - TIME_STEP_DELTA > 0) timeStep -= (isTimeStepMicros ? TIME_STEP_DELTA_us : TIME_STEP_DELTA);
    else
    {
      isTimeStepMicros = true;
      timeStep = 1000;
    }
    writeTimeStep();
    delay(BUTTON_DELAY_ms);
  }

  uint64_t currentMicros = micros();
  if (!isPaused && currentMicros - startMicros >= (isTimeStepMicros ? timeStep : timeStep * 1000))
  {
    startMicros = currentMicros;
    uint16_t analogReading = analogRead(ANALOG_PIN);
    lastAnalogReading = analogReading;
    ++voltagesAmountMeasured;

    float currentVoltage = analogToVoltage(analogReading);
    if (currentVoltage > peakVoltage) peakVoltage = currentVoltage;
    if (currentVoltage < minVoltage) minVoltage = currentVoltage;
    voltagesSum += currentVoltage;
    uint8_t currentY = map(analogReading, 0, 1023, 0, CHART_HEIGHT);  
    /*
    Serial.print(analogReading);
    Serial.print(" ");
    Serial.println(currentY);
    */
    currentY = abs((TFT_HEIGHT + OFFSET_Y - currentY));
    if (previousX >= TFT_WIDTH)
    {
      resetScreen();
      previousX = 0;
      isFirstIteration = true;
    }
    if (isFirstIteration)
    {
      //tft.drawPixel(previousX, currentY, WHITE);
      isFirstIteration = false;
      return;
    }
    tft.drawLine(previousX, previousY, previousX + VOLTAGE_STEP, currentY, WHITE);
    previousX += VOLTAGE_STEP;
    previousY = currentY;
  }

}
