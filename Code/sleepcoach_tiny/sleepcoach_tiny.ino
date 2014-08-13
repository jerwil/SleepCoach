#include <math.h>
#include <avr/sleep.h>

// Sleep coach using ATtiny85

//Programming ATtiny85 learned from: http://www.instructables.com/id/Program-an-ATtiny-with-Arduino/step2/Wire-the-circuit/

// Setting up bits for sleep mode (from: http://www.insidegadgets.com/2011/02/05/reduce-attiny-power-consumption-by-sleeping-with-the-watchdog-timer/)

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif


char* mode = "time_choose"; // Modes are time_choose, sleep_coach, and off
// Time choose mode is where the user choses between 7, 14, 21, and 28 minutes of sleep coaching
// sleep coach mode is the mode with pulsating light
// off is when the sleep coaching is complete. A button press will bring it into time choose mode

int time_choice = 7;

const int LEDPin = 0;


const int ButtonPin = 1;
int button_state = 0;
int button_pushed = 0; // This is the indicator that the button was pushed and released
int button_counter = 0; // This is used to detect how long the button is held for

int potPin = 3; // Pin for potentiometer
int pot_val; // Value of potentiometer reading

int blink = 1; // This is used for blinking the LEDs
int blink_time = 500;
int blink_value = 254;

int timeout = 0; // This is the timeout counter
int timeout_setting = 60; // This is the setting for timeout in time_choose mode. If this is exceeded, the device will go to off mode


unsigned long currentTime;
double milis_timer[1] = {0}; // This is used to keep track of the timer used to tick for each milisecond
double second_timer[1] = {0}; // This is used to keep track of the timer used to tick for each second
double blink_timer[1] = {0};  // This is used to keep track of each half second for blinking
int ticked = 0;

int delay_int = 1;
int brightness = 0;
int max_brightness = 244;
double brightincrease = 1;
double k = 0.00108*5;
double k_initial = 0.00108*5;
double k_final = 0.00065;
double x = 3*3.14159/2/k; // This starts it at 0 brightness

double total_time = 420; // seconds for entire breathing coaching
double current_time = 0;

int button_press_initiate[1];     // storage for button press function
int button_press_completed[1];    // storage for button press function


void setup() {                
  // initialize the digital pin as an output.
  pinMode(LEDPin, OUTPUT);
  pinMode(ButtonPin, INPUT);
  
  sbi(GIMSK,PCIE); // Turn on Pin Change interrupt
  sbi(PCMSK,PCINT1); // Which pins are affected by the interrupt
//  Serial.begin(9600);  
}


void loop() {
  
pot_val = analogRead(potPin);

max_brightness = pot_val/(1024/254);

button_state = digitalRead(ButtonPin);
button_pushed = button_press (button_state, button_press_initiate, button_press_completed);
  
if (mode == "time_choose"){
  
delay(10);

if (button_pushed == 1){
time_choice += 7;
button_pushed = 0;
}
if (time_choice > 28){
time_choice = 0;
mode = "off";
}
  
  if (time_choice == 7){
    k_final = .003; // Equates to 10 second breath cycle
    total_time = 420;
    blink_time = 250;
  }
  if (time_choice == 14){
    k_final = .0025; // Equates to 12 second breath cycle
    total_time = 840;
    blink_time = 500;
  }
  if (time_choice == 21){
    k_final = .0022; // Equates to 14 second breath cycle
    total_time = 1260;
    blink_time = 750;
  }
  if (time_choice == 28){
    k_final = .0019; // Equates to 16 second breath cycle
    total_time = 1680;
    blink_time = 1000;
  }
 
if (tick(1000,second_timer) == 1){
timeout += 1;
if (button_state == 1){button_counter += 1;}
}

if (button_state == 0){button_counter = 0;}

if (tick(blink_time,blink_timer) == 1){
 if (blink == 1){blink = 0;}
 else if (blink == 0){blink = 1;}
}

if (button_state == 1){
blink = 1;
timeout = 0;
}

blink_value = max_brightness;

if (blink == 0){  
blink_value = 0;
}

analogWrite(LEDPin, blink_value);


if (button_counter >= 3){ // If the user holds the button for 3 seconds, start the sleep coach
button_pushed = 0;
mode = "sleep_coach";
button_counter = 0;
}

if (timeout >= timeout_setting){mode = "off";}
 
}
  
if (mode == "sleep_coach"){
  
timeout = 0;
  
if (current_time >= total_time && brightness <= 10){
current_time = 0;
time_choice = 0;
mode = "off";
x = 0;
}
  
brightness = max_brightness/2*(1 + sin(k*x));
  
if (tick(delay_int,milis_timer) == 1){
  x += brightincrease;
}
if (tick(1000,second_timer) == 1){
  current_time += 1;
  k = k_initial + current_time*(k_final-k_initial)/total_time;
  if (button_state == 1){button_counter += 1;} 
}
if (x*k >= 2*3.14159){x=0;}
//else if (brightness <= 0){brightincrease = 1;}
  analogWrite(LEDPin,brightness);

if (button_state == 0){button_counter = 0;}

if (button_pushed == 1 && current_time > 5){ // Turn the device off by pushing the button, but do not create false press after starting
mode = "off";
button_pushed = 0;
button_counter = 0;
current_time = 0;
time_choice = 0;
x = 3*3.14159/2/k; // Start it back at 0 brightness
}

}

if (mode == "off"){

analogWrite(LEDPin,  0);  
  
system_sleep();
  
delay(1);

if (button_state == 1){ // Turn the device on by pushing the button
mode = "time_choose";
button_pushed = 0;
button_counter = 0;
timeout = 0;
}

current_time = 0;
  
}

}

int button_press (int button_indicator, int button_press_initiated[1], int button_press_complete[1]){
	if (button_indicator == 0 && button_press_initiated[0] == 1) {
	button_press_complete[0] = 1;
	button_press_initiated[0] = 0;
	}
	else if (button_indicator == 1){
	button_press_initiated[0] = 1;
	button_press_complete[0] = 0;
	}
	else {button_press_complete[0] = 0;}
return button_press_complete[0];
}

int tick(int delay, double timekeeper[1]){
currentTime = millis();
if(currentTime >= (timekeeper[0] + delay)){
	timekeeper[0] = currentTime;
	return 1;
  }
else {return 0;}
}

void tick_reset(double timekeeper[1]){
currentTime = millis();
timekeeper[0] = currentTime;
}

void system_sleep() {
  cbi(ADCSRA,ADEN); // Switch Analog to Digital converter OFF
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Set sleep mode
  sleep_mode(); // System sleeps here
  sbi(ADCSRA,ADEN);  // Switch Analog to Digital converter ON
}

ISR(PCINT0_vect) {
}
