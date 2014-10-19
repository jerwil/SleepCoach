#include <math.h>
#include <EEPROM.h>
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



char* mode = "initialize"; // Modes are time_choose, sleep_coach, and off
// Time choose mode is where the user choses between 7, 14, 21, and 28 minutes of sleep coaching
// sleep coach mode is the mode with pulsating light
// off is when the sleep coaching is complete. A button press will bring it into time choose mode

int profile = 1;

const int LEDPin = 0;
const int pin_A = 3;  // pin 12
const int pin_B = 4;  // pin 11
const int ButtonPin = 1;

unsigned char encoder_A;
unsigned char encoder_B;
unsigned char encoder_A_prev=0;

int button_state = 0;
int button_pushed = 0; // This is the indicator that the button was pushed and released
int button_counter = 0; // This is used to detect how long the button is held for

int blink = 1; // This is used for blinking the LEDs
int blink_time = 500;
int blink_value = 254;

int timeout = 0; // This is the timeout counter
int timeout_setting = 60; // This is the setting for timeout in time_choose mode. If this is exceeded, the device will go to off mode

int brightness_mult = 9;    // how bright the LED is, start at half brightness
int fadeAmount = 1;    // how many points to fade the LED by


unsigned long currentTime;
double milis_timer[1] = {0}; // This is used to keep track of the timer used to tick for each milisecond
double second_timer[1] = {0}; // This is used to keep track of the timer used to tick for each second
double blink_timer[1] = {0};  // This is used to keep track of each half second for blinking
int ticked = 0;

int clockwise = 0;
int counterclockwise = 0;

int delay_int = 1;
int brightness = 0;
int max_brightness = 244;
double brightincrease = 1;
double k = 0.00108*5;
double k_initial = 0.00108*5;
double k_final = 0.00065;
double x = 3*3.14159/2/k; // This starts it at 0 brightness
double breath_length = 6; // The user determined breath length

// K-factors - these are used in the sine wave generation portion to change the period of the sine wave. (a k of .0054 corresponds to 6 second breath, a k of .001 corresponds to an 18 second breath) //

double k_delta = .0002; // The amount of change in k for each rotary encoder tick

double k_values[8] = {.0054, .0054, .0054, .0054, .003, .0025, .0022, .0019}; // {k1_initial, k2_initial, k3_initial, k4_initial, k1_final, k2_final, k3_final, k4_final}

double k1_initial = .0054; // 6 seconds
double k2_initial = .0054;
double k3_initial = .0054;
double k4_initial = .0054;
double k1_final = .003;  // 10 seconds
double k2_final = .0025; // 12 seconds
double k3_final = .0022; //14 seconds
double k4_final = .0019; //16 seconds

int eepromwrite = 200;

int val = 200; // Current value read from EEPROM

double total_time = 420; // seconds for entire breathing coaching
double current_time = 0;

int button_press_initiate[1];     // storage for button press function
int button_press_completed[1];    // storage for button press function


void setup() {                
  // initialize the digital pin as an output.
  pinMode(LEDPin, OUTPUT);
  pinMode(ButtonPin, INPUT);
  pinMode(pin_A, INPUT);
  pinMode(pin_B, INPUT);
  
  sbi(GIMSK,PCIE); // Turn on Pin Change interrupt
  sbi(PCMSK,PCINT1); // Which pins are affected by the interrupt
//  Serial.begin(9600); 


}


void loop() {
  
if (mode == "initialize"){
 
if (EEPROM.read(8) == 1){

for (int i = 0; i < 8; i++){
  val = EEPROM.read(i);
  k_values[i] = val;
  k_values[i] = k_values[i]/10000;
}
}

else {
  for (int i = 0; i < 512; i++)
    EEPROM.write(i, 0);
  for (int i = 0; i < 8; i++){
  EEPROM.write(i,54);
  }
    EEPROM.write(8,1);
}
  
mode = "time_choose";
}
  
    clockwise = 0;
    counterclockwise = 0;
    encoder_A = digitalRead(pin_A);    // Read encoder pins
    encoder_B = digitalRead(pin_B);   
    if((!encoder_A) && (encoder_A_prev)){
      // A has gone from high to low 
      if(encoder_B) {
        // B is high so clockwise
        // increase the brightness multiplier, dont go over 25
        clockwise = 1;
        if(brightness_mult + fadeAmount <= 25) brightness_mult += fadeAmount;               
      }   
      else {
        // B is low so counter-clockwise      
        // decrease the brightness, dont go below 0
        if(brightness_mult - fadeAmount >= 2) brightness_mult -= fadeAmount;
         counterclockwise = 1;        
      } 
    }
encoder_A_prev = encoder_A;  
max_brightness = brightness_mult*10;

button_state = digitalRead(ButtonPin);
button_pushed = button_press (button_state, button_press_initiate, button_press_completed);
  
if (mode == "time_choose"){
  
x = 0;
  
//delay(10);

if (button_pushed == 1){
profile += 1;
button_pushed = 0;
}
if (profile > 4){
profile = 0;
mode = "off";
}
  
  if (profile == 1){
    blink_time = 250;
  }
  if (profile == 2){
    blink_time = 500;
  }
  if (profile == 3){
    blink_time = 750;
  }
  if (profile == 4){
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

  if (profile == 1){

    total_time = 420;
  }
  if (profile == 2){

    total_time = 840;
  }
  if (profile == 3){

    total_time = 1260;
  }
  if (profile == 4){

    total_time = 1680;
  }

k_initial = k_values[profile-1];
k_final = k_values[profile+3];

}

if (timeout >= timeout_setting){mode = "off";}
 
}
  
if (mode == "sleep_coach"){
  
timeout = 0;
  
if (current_time >= total_time && brightness <= 10){
current_time = 0;
profile = 0;
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

if (button_counter >= 3){ // If the user holds the button for 3 seconds, start the sleep coach
button_pushed = 0;
mode = "to_initial_adjust";
button_counter = 0;
}

if (button_pushed == 1 && current_time > 5){ // Turn the device off by pushing the button, but do not create false press after starting
mode = "off";
button_pushed = 0;
button_counter = 0;
current_time = 0;
profile = 0;
x = 3*3.14159/2/k; // Start it back at 0 brightness
}

}

if (mode == "to_initial_adjust"){
  
x = 0;
  
button_pushed = 0;
  
// Flash LED 5 times
  
analogWrite(LEDPin,244);
delay(100);               
analogWrite(LEDPin,0);
delay(100);
analogWrite(LEDPin,244);
delay(100);               
analogWrite(LEDPin,0);
delay(100);        
analogWrite(LEDPin,244);
delay(100);               
analogWrite(LEDPin,0);
delay(100);
analogWrite(LEDPin,244);
delay(100);               
analogWrite(LEDPin,0);
delay(100);
analogWrite(LEDPin,244);
delay(100);               
analogWrite(LEDPin,0);
delay(100);      
mode = "initial_adjust";
}

if (mode == "initial_adjust"){

// Adjusts the initial breath length of the currently selected profile
  
if (counterclockwise == 1)
  {if(k + k_delta <= .01) 
    {k += k_delta;
    timeout = 0;}
  }
else if (clockwise == 1)
  {if(k - k_delta >= .0019) 
    {k -= k_delta;
    timeout = 0;}
  }
//k = pow(breath_length,3)*-0.000004166667+pow(breath_length,2)*0.000175000000+breath_length*-0.002583333333+0.015500000000;
brightness = 127*(1 + sin(k*x));  
if (tick(delay_int,second_timer) == 1){
  x += brightincrease;
}

if (x*k >= 2*3.14159){x=0;}

analogWrite(LEDPin,brightness);

if (tick(1000,blink_timer) == 1){
timeout += 1;
if (button_state == 1){button_counter += 1;}
}

if (button_state == 0){button_counter = 0;}

if (button_counter >= 3){ // If the user holds the button for 3 seconds, start the sleep coach
button_pushed = 0;
mode = "to_final_adjust";
button_counter = 0;
timeout = 0;
}

if (button_state == 1){
timeout = 0;
}

if (timeout >= timeout_setting){
mode = "off";
timeout = 0;
}

}

if (mode == "to_final_adjust"){
  
x = 0;
  
//  if (profile == 1){
//    k1_initial = k;
//  }
//  if (profile == 2){
//    k2_initial = k;
//  }
//  if (profile == 3){
//    k3_initial = k;
//  }
//  if (profile == 4){
//    k4_initial = k;
//  }
//
//eepromwrite = k1_initial*10000 ; 
//EEPROM.write(0, eepromwrite);
//eepromwrite = k2_initial*10000 ; 
//EEPROM.write(1, eepromwrite);
//eepromwrite = k3_initial*10000 ; 
//EEPROM.write(2, eepromwrite);
//eepromwrite = k4_initial*10000 ; 
//EEPROM.write(3, eepromwrite);

k_values[profile-1] = k;
val = k*10000;
EEPROM.write(profile-1,val);
EEPROM.write(8, 1);
  
// Flash LED 3 times
  
analogWrite(LEDPin,244);
delay(100);               
analogWrite(LEDPin,0);
delay(100);
analogWrite(LEDPin,244);
delay(100);               
analogWrite(LEDPin,0);
delay(100);        
analogWrite(LEDPin,244);
delay(100);               
analogWrite(LEDPin,0);
delay(100);
mode = "final_adjust";
}


if (mode == "final_adjust"){

// Adjusts the final breath length of the currently selected profile
  
if (counterclockwise == 1)
  {if(k + k_delta <= .01) 
    {k += k_delta;
    timeout = 0;}
  }
else if (clockwise == 1)
  {if(k - k_delta >= .0019) 
    {k -= k_delta;
    timeout = 0;}
  }

brightness = 127*(1 + sin(k*x));  
if (tick(delay_int,second_timer) == 1){
  x += brightincrease;
}

if (x*k >= 2*3.14159){x=0;}

analogWrite(LEDPin,brightness);


if (tick(1000,blink_timer) == 1){
timeout += 1;
if (button_state == 1){button_counter += 1;}
}

if (button_state == 0){button_counter = 0;}

if (button_counter >= 3){ // If the user holds the button for 3 seconds, start the sleep coach
button_pushed = 0;
mode = "time_choose";
button_counter = 0;
timeout = 0;

//  if (profile == 1){
//    k1_final = k;
//  }
//  if (profile == 2){
//    k2_final = k;
//  }
//  if (profile == 3){
//    k3_final = k;
//  }
//  if (profile == 4){
//    k4_final = k;
//  }
//  
//eepromwrite = k1_final*10000 ; 
//EEPROM.write(4, eepromwrite);
//eepromwrite = k2_final*10000 ; 
//EEPROM.write(5, eepromwrite);
//eepromwrite = k3_final*10000 ; 
//EEPROM.write(6, eepromwrite);
//eepromwrite = k4_final*10000 ; 
//EEPROM.write(7, eepromwrite);

k_values[profile+3] = k;
val = k*10000;
EEPROM.write(profile+3,val);
EEPROM.write(8, 1);

}

if (button_state == 1){
timeout = 0;
}

if (timeout >= timeout_setting){
mode = "off";
timeout = 0;
}

}



if (mode == "off"){
  
x = 0;

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
