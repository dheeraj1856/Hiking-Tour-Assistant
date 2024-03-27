#include "config.h"
#include <ArduinoBLE.h>

TTGOClass *watch;
TFT_eSPI *tft;
TTGOClass *ttgo;
BMA *sensor;
bool isCountingSteps = false; // Step counting initially inactive
volatile bool irq = false; // Mark as volatile since it's used in interrupt context
#define COLOR565(r,g,b)  ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
//const unsigned int BCKGRDCOL = COLOR565(138, 235, 244);
const unsigned int BCKGRDCOL = COLOR565(0, 0, 0);
volatile uint32_t stepCount = 0; // Renamed from 'step' to avoid conflict
volatile double calories = 0;
//long totalHikeTime = 0;
//uint32_t previousStepCount = 0;
unsigned long lastUpdateTime = 0;
volatile unsigned long totalHikeTime=0;
const long updateInterval = 500; // Update interval in milliseconds
const uint32_t DEBOUNCE_TIME = 50; // 50 milliseconds
uint32_t lastTouchTime = 0;
bool needsDisplayInit = true; // Global variable to control display initialization
double previousCalories = 0;
uint32_t previousDistance = 0;
long lastTime = 0, minutes = 0, hours = 0, distance = 0;
//extern TFT_eSPI *tft;

unsigned long previousMillis = 0;  // Variable to store the last time the message was sent
const unsigned long interval = 1000; 
char buf[128];
bool rtcIrq = false;

#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
BLEService uartService(SERVICE_UUID); // create service
BLEStringCharacteristic rxCharacteristic(CHARACTERISTIC_UUID_RX, BLEWrite, 30);
BLEStringCharacteristic txCharacteristic(CHARACTERISTIC_UUID_TX, BLENotify, 30);

void setup() {
    Serial.begin(115200);
    watch = TTGOClass::getWatch();
    watch->begin();
    watch->openBL();
    tft = watch->tft;
    sensor = watch->bma;
    // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");

    while (1);
  }
  
  // set the local name peripheral advertises
  BLE.setLocalName("BLE_peripheral_uart");
  // set the UUID for the service this peripheral advertises
  BLE.setAdvertisedService(uartService);

  // add the characteristic to the service
  uartService.addCharacteristic(rxCharacteristic);
  uartService.addCharacteristic(txCharacteristic);

  // add service
  BLE.addService(uartService);

  // assign event handlers for connected, disconnected to peripheral
  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  // assign event handlers for characteristic
  //rxCharacteristic.setEventHandler(BLEWritten, rxCharacteristicWritten);
  // set an initial value for the characteristic
  //rxCharacteristic.setValue("BLE_peripheral_uart");

  // start advertising
  BLE.advertise();
     // Accel parameter structure
    Acfg cfg;
    /*!
        Output data rate in Hz, Optional parameters:
            - BMA4_OUTPUT_DATA_RATE_0_78HZ
            - BMA4_OUTPUT_DATA_RATE_1_56HZ
            - BMA4_OUTPUT_DATA_RATE_3_12HZ
            - BMA4_OUTPUT_DATA_RATE_6_25HZ
            - BMA4_OUTPUT_DATA_RATE_12_5HZ
            - BMA4_OUTPUT_DATA_RATE_25HZ
            - BMA4_OUTPUT_DATA_RATE_50HZ
            - BMA4_OUTPUT_DATA_RATE_100HZ
            - BMA4_OUTPUT_DATA_RATE_200HZ
            - BMA4_OUTPUT_DATA_RATE_400HZ
            - BMA4_OUTPUT_DATA_RATE_800HZ
            - BMA4_OUTPUT_DATA_RATE_1600HZ
    */
    cfg.odr = BMA4_OUTPUT_DATA_RATE_800HZ;
    /*!
        G-range, Optional parameters:
            - BMA4_ACCEL_RANGE_2G
            - BMA4_ACCEL_RANGE_4G
            - BMA4_ACCEL_RANGE_8G
            - BMA4_ACCEL_RANGE_16G
    */
    cfg.range = BMA4_ACCEL_RANGE_2G;
    /*!
        Bandwidth parameter, determines filter configuration, Optional parameters:
            - BMA4_ACCEL_OSR4_AVG1
            - BMA4_ACCEL_OSR2_AVG2
            - BMA4_ACCEL_NORMAL_AVG4
            - BMA4_ACCEL_CIC_AVG8
            - BMA4_ACCEL_RES_AVG16
            - BMA4_ACCEL_RES_AVG32
            - BMA4_ACCEL_RES_AVG64
            - BMA4_ACCEL_RES_AVG128
    */
    cfg.bandwidth = BMA4_ACCEL_NORMAL_AVG4;

    /*! Filter performance mode , Optional parameters:
        - BMA4_CIC_AVG_MODE
        - BMA4_CONTINUOUS_MODE
    */
    cfg.perf_mode = BMA4_CONTINUOUS_MODE;

    // Configure the BMA423 accelerometer
    sensor->accelConfig(cfg);

    // Enable BMA423 accelerometer
    // Warning : Need to use steps, you must first enable the accelerometer
    // Warning : Need to use steps, you must first enable the accelerometer
    // Warning : Need to use steps, you must first enable the accelerometer
    sensor->enableAccel();

    //sensor->accelConfig(cfg);
    sensor->enableAccel();

    pinMode(BMA423_INT1, INPUT);
    attachInterrupt(digitalPinToInterrupt(BMA423_INT1), []() {irq = true;}, RISING);
    
    pinMode(RTC_INT_PIN, INPUT_PULLUP);
    attachInterrupt(RTC_INT_PIN, [] {
        rtcIrq = 1;
    }, FALLING);
    
    sensor->enableFeature(BMA423_STEP_CNTR, true);
    sensor->resetStepCounter();
    sensor->enableStepCountInterrupt();
    
    tft->fillScreen(TFT_BLACK);
    tft->setTextColor(TFT_WHITE);
    tft->setTextSize(2);
    displayInit();

    //tft->begin();  // Initialize the TFT screen
    //tft->setRotation(1);  // Set screen rotation
}

void loop() {
  unsigned long currentMillis1 = millis();
  if (currentMillis1 - previousMillis >= interval) {
    // Save the last time the message was sent
    previousMillis = currentMillis1;

    // Send the message
    String message = String(stepCount) + ";" + String(calories) + ";" + String(totalHikeTime) + ";" + String(stepCount / 1408);
    txCharacteristic.setValue(message);
  }
  BLE.poll();
  
    if (needsDisplayInit) {
        displayInit();
        needsDisplayInit = false; 
    }

    
    if (irq) {
        handleInterrupt();
    }
    
    int16_t x, y; 
    unsigned long currentMillis = millis();

    unsigned long elapsedHours = currentMillis / (1000UL * 60UL * 60UL);
    Serial.println(elapsedHours);
    if (watch->getTouch(x, y) && (currentMillis - lastTouchTime > DEBOUNCE_TIME)) {
        lastTouchTime = currentMillis; // Update touch time to manage debounce

        // Calculate button positions as before
        int16_t centerX = (tft->width() / 2) - 50;
        int16_t centerY = (tft->height() / 2) - 20;

        // Common button dimensions
        uint16_t buttonWidth = 100;
        uint16_t buttonHeight = 40;
        int16_t gapBetweenButtons = 20; // Gap between the "Start" and "Reset" buttons
                // "Reset" button area (directly below the "Start" button)
        int16_t resetX = centerX;
        int16_t resetY = centerY + buttonHeight + gapBetweenButtons;
    
        int16_t homeX = 120, homeY = 200, homeWidth = 100, homeHeight = 40;

                        // "End" button touch area
        int16_t endButtonX = 10; // 10 pixels padding from the left edge
        int16_t endButtonY = tft->height() - 50; // Positioned 50 pixels up from the bottom edge
        uint16_t endButtonWidth = 100;
        uint16_t endButtonHeight = 40;

        // Handle "Start" button touch
        if (x >= centerX && x <= centerX + 100 && y >= centerY && y <= centerY + 40) {
            isCountingSteps = !isCountingSteps; // Toggle step counting state
            //displayFullBackgroundImage();
            //delay(200);
            calories = getCalories(false);
            totalHikeTime = getTime(false);
            updateDisplay();
        }
        // Handle "Reset" button touch
        else if (x >= resetX && x <= resetX + buttonWidth && y >= resetY && y <= resetY + buttonHeight) {
            resetSteps(); // Reset step count
            totalHikeTime = 0;
            needsDisplayInit = true; // Indicate need to reinitialize display (e.g., to update UI)
        }

        // Handle "Home" button touch
        else if (x >= homeX && x <= homeX + homeWidth && y >= homeY && y <= homeY + homeHeight) {
            needsDisplayInit = true; // Reinitialize display to initial state
        }

        else if (x >= endButtonX && x < endButtonX + endButtonWidth && y >= endButtonY && y < endButtonY + endButtonHeight) {
            // Touch detected within the "End" button area
            // Perform your "End" button action here
                totalHikeTime = 0; // Reset total hike time to zero
                updateDisplay(); // Update the display to reflect the change
                needsDisplayInit = true; // Indicate need to reinitialize display (e.g., to update UI)
        }
    }

    // Throttle step count start/update based on interval
    if (currentMillis - lastUpdateTime >= updateInterval && isCountingSteps) {
        lastUpdateTime = currentMillis;
        step_count_start(); // Update step count
    }
    delay(20);
}        

void step_count_start() {
    if (irq) {
        irq = false;
        if (sensor->readInterrupt() && sensor->isStepCounter()) {          
            stepCount = sensor->getCounter(); // Update stepCount with the new value
            // Recalculate distance (assuming distance is recalculated here or elsewhere based on stepCount)
            uint32_t currentDistance = stepCount / 1408; // Your formula for distance
            previousCalories = getCalories(false); // Recalculate calories based on new step count and time

            // Check if calories or distance have changed significantly
            if (abs(calories - previousCalories) >= 1 || currentDistance != previousDistance) {
                
                // Update previous values for next comparison
                previousCalories = calories;
                previousDistance = currentDistance;
                updateDisplay();
            }
        }
    }
}

void displayInit() {
    tft->fillScreen(BCKGRDCOL);
    tft->setTextColor(TFT_WHITE, BCKGRDCOL);
    tft->setTextFont(2);
    tft->setCursor(0, 50);
    tft->drawString("     Go Hike :) ", 3, 50, 2);

    // Calculate the center of the screen
    int16_t centerX = (tft->width() / 2) - (100 / 2); // Half the screen width minus half the button width
    int16_t centerY = (tft->height() / 2) - (40 / 2);  // Half the screen height minus half the button height

    // Draw the "Start" button centered
    tft->fillRoundRect(centerX, centerY, 100, 40, 5, TFT_GREEN); // Button dimensions and position
    tft->setTextColor(TFT_WHITE, TFT_GREEN); // Text color: Black on white button
    tft->setTextSize(2);
    tft->setCursor(centerX + 20, centerY + 10); // Adjust text position within the button
    tft->print("Start");

    // Reset button details (below the Start button)
    uint16_t buttonWidth = 100;
    uint16_t buttonHeight = 40; 
    int16_t resetX = centerX; // Align with the Start button horizontally
    int16_t resetY = centerY + buttonHeight + 20; // 20 pixels gap below the Start button

    // Draw the "Reset" button
    tft->fillRoundRect(resetX, resetY, 100, 40, 5, TFT_BROWN);
    tft->setTextColor(TFT_WHITE, TFT_BROWN);
    tft->setTextSize(2);
    tft->setCursor(resetX + 20, resetY + 10);
    tft->print("Reset");
}

void updateDisplay() {
    
    char calorieStr[20];

    tft->fillScreen(BCKGRDCOL); // Clear screen area where steps are displayed
    tft->setTextColor(TFT_WHITE, BCKGRDCOL); // White text, black background
    tft->setCursor(0, 30);
    tft->printf("Steps : %u", stepCount); // Use the renamed variable
    Serial.println(stepCount);

    sprintf(calorieStr, "Calories: %.1f kcal", calories);
    tft->setTextColor(TFT_WHITE, BCKGRDCOL);
    tft->setTextSize(2);
    tft->setCursor(0, 60);
    tft->print(calorieStr);
   

    
    tft->setTextColor(TFT_WHITE, BCKGRDCOL);
    tft->setTextSize(2);
    tft->setCursor(0,90);
    tft->printf("Distance: %u km", stepCount / 1408); // avg steps for human during normal walk

    tft->setTextColor(TFT_WHITE, BCKGRDCOL);
    tft->setTextSize(2);
    tft->setCursor(0,120);
    tft->printf("Time: %u min", totalHikeTime);


    // Draw the "Home" button on every update    
    tft->fillRoundRect(120, 200, 100, 40, 5, TFT_BLUE); // Adjust position and size as needed
    tft->setTextColor(TFT_WHITE, TFT_BLUE);
    tft->setTextSize(2);
    tft->setCursor(130, 210); // Adjust text position as needed
    tft->print("Home");   

    int16_t endButtonX = 10; // 10 pixels padding from the left edge
    int16_t endButtonY = tft->height() - 40; // Positioned 40 pixels up from the bottom edge 
    uint16_t endButtonWidth = 100;
    uint16_t endButtonHeight = 40;
    uint16_t endButtonColor = TFT_RED;

    tft->fillRoundRect(endButtonX, endButtonY, endButtonWidth, endButtonHeight, 5, endButtonColor);
    tft->setTextColor(TFT_WHITE, endButtonColor);
    tft->setTextSize(2);
    tft->setCursor(endButtonX + 20, endButtonY + 10);
    tft->print("End");
}

void handleInterrupt() {
    irq = false;
    if (sensor->readInterrupt() && sensor->isStepCounter()) {
        stepCount = sensor->getCounter();
        updateDisplay(); // Update display with new step count
    }
}

void resetSteps() {
    stepCount = 0;
    calories = 0;
    totalHikeTime = 0;
    sensor->resetStepCounter();
    // Update display immediately if needed or flag for update in the next loop iteration
    needsDisplayInit = true; // Flag to refresh the display
}

bool checkTouch(int16_t &x, int16_t &y) {
    if (ttgo->getTouch(x, y)) {
        uint32_t currentMillis = millis();
        if (currentMillis - lastTouchTime > DEBOUNCE_TIME) {
            lastTouchTime = currentMillis;
            return true; // Valid touch detected
        }
    }
    return false; // No valid touch detected
}

float getCalories(bool x) {
  // calories = T × MET × 3.5 × W / (200) 
  // W : weight(kg), T : duration(sec), MET : metabolic equivalen task
  return x ? 0: stepCount / 20;
}
unsigned long getTime(bool x) {
  return x ? 0: millis() / (1000UL * 60UL);
}

void blePeripheralConnectHandler(BLEDevice central) {
  // central connected event handler
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
}

void blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
}
