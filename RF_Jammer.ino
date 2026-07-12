// ============================================================
// INCLUDES
// ============================================================

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RF24.h>

// ============================================================
// PIN DEFINITIONS
// ============================================================

#define UP_BUTTON 4
#define DOWN_BUTTON 5
#define SELECT_BUTTON 3

// ============================================================
// DISPLAY CONFIGURATION
// ============================================================

Adafruit_SSD1306 display(128, 64, &Wire);

// ============================================================
// RF24 CONFIGURATION
// ============================================================

// Pin config (CE, CSN)

RF24 radio1(9, 8); 

RF24 radio2(7,6);

bool writing = false;

// ============================================================
// MENU STATE
// ============================================================

enum MenuState {
  MAIN_MENU,
  SPECTRUM_MENU,
  START_SCREEN,
  END_SCREEN
};

MenuState currentState = MAIN_MENU;

int selectedIndex = 0;
unsigned long lastDebounce = 0;
const int debounceDelay = 150;

// ============================================================
// MENU VARIABLES
// ============================================================

const String MAIN_MENU_OPTIONS[] = {"Start", "End", "Spectrum Analyzer"};
MenuState MAIN_MENU_ELEMENTS[] = {START_SCREEN, END_SCREEN, SPECTRUM_MENU};

const String SPECTRUM_MENU_OPTIONS[] = {"Back"};
MenuState SPECTRUM_MENU_ELEMENTS[] = {MAIN_MENU};

// ============================================================
// SETUP
// ============================================================

void radio_setup(RF24 &radio){
  radio.setAutoAck(false);
  radio.stopListening();
  radio.setRetries(0, 0);
  radio.setPALevel(RF24_PA_MAX, true);
  radio.setDataRate(RF24_2MBPS);
  radio.disableCRC(); 
  radio.printPrettyDetails();
}

void setup() {

  // SERIAL SETUP

  Serial.begin(115200);

  // RADIO SETUP AND CONFIGURATION

  if (radio2.begin() && radio1.begin()) {
    Serial.println(F("Radios are responding!"));
  }
  else {
    Serial.println(F("Radios are not responding"));
    Serial.println(radio1.begin());
    Serial.println(radio2.begin());
    while (1) {}
  }

  radio_setup(radio1);
  radio_setup(radio2);

  // DISPLAY SET UP

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();

  // BUTTON PINMODE SETUP

  pinMode(UP_BUTTON, INPUT_PULLUP);
  pinMode(DOWN_BUTTON, INPUT_PULLUP);
  pinMode(SELECT_BUTTON, INPUT_PULLUP);
}

// ============================================================
// LOOP
// ============================================================

void loop() {

  if (writing == true){
    THING(radio1);
    THING(radio2);  
  }

  switch (currentState) {
    case MAIN_MENU:
      MENU_SCREEN(3, MAIN_MENU_OPTIONS, "Main Menu", MAIN_MENU_ELEMENTS);
      break;
    case SPECTRUM_MENU:
      SPECTRUM_SCREEN("Spectrum Analyzer", MAIN_MENU);
      break;
    case START_SCREEN:
      INFO_SCREEN("Starting...", MAIN_MENU);
      START_THING();
      break;
    case END_SCREEN:
      INFO_SCREEN("Ending...", MAIN_MENU);
      STOP_THING(radio1);
      STOP_THING(radio2);
      break;
  }
}

// ============================================================
// RF24 CONTROL
// ============================================================

void START_THING(){
  writing = true;
}

void STOP_THING(RF24 &radio){
  writing = false;
  radio.stopConstCarrier();
  radio.powerDown();
  radio.powerUp();
  radio_setup(radio);
}

void THING(RF24 &radio){
  radio.startConstCarrier(RF24_PA_MAX, random(125));
}

// ============================================================
// BUTTON HANDLING
// ============================================================

bool buttonPressed(int pin) {
  if (digitalRead(pin) == LOW && millis() - lastDebounce > debounceDelay) {
    lastDebounce = millis();
    return true;
  }
  return false;
}

// ============================================================
// MENU DRAWING
// ============================================================

void drawMenu(const char* title, const String options[], int optionCount) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(title);
  display.println("----------------");

  for (int i = 0; i < optionCount; i++) {
    if (i == selectedIndex) {
      display.print("> ");
    } else {
      display.print("  ");
    }
    display.println(options[i]);
  }

  display.display();
}

// ============================================================
// SCREEN FUNCTIONALITES
// ============================================================

void MENU_SCREEN(int OPTION_COUNT, const String MENU_OPTIONS[], const char* menu_title, MenuState MENU_ELEMENTS_ARRAY[]) {
  drawMenu(menu_title, MENU_OPTIONS, OPTION_COUNT);

  if (buttonPressed(UP_BUTTON)) {
    selectedIndex = (selectedIndex - 1 + OPTION_COUNT) % OPTION_COUNT;
  }
  if (buttonPressed(DOWN_BUTTON)) {
    selectedIndex = (selectedIndex + 1) % OPTION_COUNT;
  }
  if (buttonPressed(SELECT_BUTTON)) {
    currentState = MENU_ELEMENTS_ARRAY[selectedIndex];
    selectedIndex = 0;
  }
}

void INFO_SCREEN(String text, MenuState backstate) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 20);
  display.println(text);
  display.display();

  if (buttonPressed(SELECT_BUTTON)) {
    currentState = backstate;
  }
}

void SPECTRUM_SCREEN(String text, MenuState backstate){
  display.clearDisplay();
  display.setTextSize(0.25);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 5);
  display.println("                 Amp");
  display.fillRect(108, 15, 1, 43, SSD1306_WHITE);
  display.setCursor(0, 54);

  for (int i = 0; i < 10; i++){
    if (RSSI_ANALYSIS(radio2, (i * 10), 10)){
      DRAW_HIGH_RSSI_READING(i);
    } else {
      DRAW_LOW_RSSI_READING(i);
    }
  }

  display.print(F("------------------ F"));
  display.display();

  if (buttonPressed(SELECT_BUTTON)) {
    currentState = backstate;
  }
}

// ============================================================
// SPECTRUM BAR FUNCTIONS
// ============================================================

void DRAW_HIGH_RSSI_READING(int increment_parameter){
  display.fillRect(5 + (10 * increment_parameter), 48, 2, 10, SSD1306_WHITE);
}

void DRAW_LOW_RSSI_READING(int increment_parameter){
  display.fillRect(5 + (10 * increment_parameter), 52, 2, 5, SSD1306_WHITE);
}

// ============================================================
// RSSI ANALYSIS
// ============================================================

bool RSSI_ANALYSIS(RF24 &radio, int channel, int samples) {
  radio.stopConstCarrier();
  radio.setChannel(channel);

  int hits = 0;
  for (int i = 0; i < samples; i++){
    radio.startListening();
    delayMicroseconds(500);
    bool result = radio.testRPD();
    if (result) hits++;
    radio.stopListening();
  }
  if (hits > 4){
    return true;
  }
  else{
    return false;
  }
}
