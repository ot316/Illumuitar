//**********************************************************************
// Project           : DE_3 Audio Installation Project
//
// Program name      : GuitarProjectCode
//
// Author            : Oli Thompson, Emily Branson, Richard Zhang
//
//**********************************************************************
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#define LOG_OUT 1 // use the log output function
#define FHT_N 128 // set to 256 point fht
#define OCTAVE 1
#include <FHT.h> // include the library
#define PIN 6
#define NUM_LEDS 206
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800); //declare number of LEDs and data pin
int x = 16;
int y = 26; //dimensions of LED matrix
//data storage variables
byte newData = 0;
byte prevData = 0;
unsigned int time = 0;//keeps time and sends vales to store in timer[] occasionally
int timer[10];//storage for timing of events
int slope[10];//storage for slope of events
unsigned int totalTimer;//used to calculate period
unsigned int period;//storage for period of wave
byte index = 0;//current storage index
float frequency;//storage for frequency calculations
int maxSlope = 0;//used to calculate max slope as trigger point
int newSlope;//storage for incoming slope data

//variables for deciding whether you have a match
byte noMatch = 0;//counts how many non-matches you've received to reset variables if it's been too long
byte slopeTol = 3;//slope tolerance- adjust this if you need
int timerTol = 10;//timer tolerance- adjust this if you need

//variables for amp detection
unsigned int ampTimer = 0;
byte maxAmp = 0;
byte checkMaxAmp;
byte ampThreshold = 30;//raise if you have a very noisy signal

//variables for tuning
int correctFrequency;//the correct frequency for the string being played
byte lookuptable[16][26]={ //lookup table for LEDs
       {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  7,  8,  0,  0,  0,  0  },
       {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  5,  6,  0,  12, 11, 10, 9,  0,  0  }, 
       {0,  0,  0,  0,  0,  1,  2,  3,  0,  0,  0,  0,  0,  0,  4,  0,  0,  13, 14 ,15 ,16 ,17, 18, 19, 20, 0  },
       {0,  0,  0,  0,  0,  0,  0,  34, 33, 32, 31, 30, 29, 0,  0,  28, 27, 0,  0,  26, 25, 0,  24, 23, 22, 21 },
       {0,  0,  0,  0,  0,  0,  0,  0,  35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52 },
       {0,  0,  0,  0,  0,  0,  0,  0,  69, 68, 67, 66, 65, 64, 63, 0,  62, 61, 60, 59, 58, 57, 56, 55, 54, 53 },
       {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  70, 71, 0,  85, 84, 0,  0,  0,  0,  0,  0,  90, 89, 88, 87, 86 },
       {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  73, 72, 0,  82, 83, 0,  0,  0,  0,  0,  0,  91, 92, 93, 94, 95 },
       {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  74, 75, 0,  81, 80, 0,  0,  0,  0,  0,  0,  100,99, 98, 97, 96 },
       {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  77, 76, 0,  78, 79, 0,  0,  0,  0,  0,  0,  101,102,103,104,105},
       {0,  0,  0,  0,  0,  126,125,124,123,122,121,120,119,118,117,116,115,114,113,112,111,110,109,108,107,106},
       {0,  0,  0,  0,  0,  127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147},
       {0,  0,  0,  0,  169,168,167,166,165,164,163,162,161,160,159,158,157,156,155,154,153,152,151,150,149,148},
       {170,171,172,173,174,175,176,177,0,  0,  0,  0,  0,  181,182,183,184,185,186,187,188,189,190,191,192,193},
       {0,  0,  180,179,178,0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  202,201,200,199,198,197,196,195,194,0  },
       {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  203,204,205,206,0,  0,  0  }
    }; 
void setup() {
Serial.begin(115200); // initialise the serial port
  strip.begin(); // initialise LED strip
  strip.setBrightness(100);
pinMode(12,INPUT_PULLUP); 
pinMode(9,INPUT_PULLUP);  //declare pins for toggle switches
}
void allLEDsOff(){ //turns all LED's off.
       for(int j=0; j <= 206; j++){
                strip.setPixelColor(j, 0,0,0);
 
               }}

ISR(ADC_vect) {//when new ADC value ready
  
  PORTB &= B11101111;//set pin 12 low
  prevData = newData;//store previous value
  newData = ADCH;//get value from A0
  //Serial.println(newData);
  if (prevData < 127 && newData >=127){//if increasing and crossing midpoint
    newSlope = newData - prevData;//calculate slope
    if (abs(newSlope-maxSlope)<slopeTol){//if slopes are ==
      //record new data and reset time
      slope[index] = newSlope;
      timer[index] = time;
      time = 0;
      if (index == 0){//new max slope just reset
        PORTB |= B00010000;//set pin 12 high
        noMatch = 0;
        index++;//increment index
      }
      else if (abs(timer[0]-timer[index])<timerTol && abs(slope[0]-newSlope)<slopeTol){//if timer duration and slopes match
        //sum timer values
        totalTimer = 0;
        for (byte i=0;i<index;i++){
          totalTimer+=timer[i];
        }
        period = totalTimer;//set period
        //reset new zero index values to compare with
        timer[0] = timer[index];
        slope[0] = slope[index];
        index = 1;//set index to 1
        PORTB |= B00010000;//set pin 12 high
        noMatch = 0;
      }
      else{//crossing midpoint but no match
        index++;//increment index
        if (index > 9){
          reset();
        }
      }
    }
    else if (newSlope>maxSlope){//if new slope is much larger than max slope
      maxSlope = newSlope;
      time = 0;//reset clock
      noMatch = 0;
      index = 0;//reset index
    }
    else{//slope not steep enough
      noMatch++;//increment no match counter
      if (noMatch>9){
        reset();
      }
    }
  }
  
  time++;//increment timer at rate of 38.5kHz
  
  ampTimer++;//increment amplitude timer
  if (abs(127-ADCH)>maxAmp){
    maxAmp = abs(127-ADCH);
  }
  if (ampTimer==1000){
    ampTimer = 0;
    checkMaxAmp = maxAmp;
    maxAmp = 0;
  }
  
}



void(* resetFunc) (void) = 0;//declare reset function at address 0
               
void visualiserplay(volatile byte r, volatile byte g, volatile byte b) { //function for visualiser
   while(1) { // reduces jitter
      TIMSK0 = 0; // turn off timer0 for lower jitterM
      ADCSRA = 0xe5; // set the adc to free running mode
      ADMUX = 0x40; // use adc0
      DIDR0 = 0x01; // turn off the digital input for adc0
      strip.show(); // Initialize all pixels to 'off'      
        for (int i = 0 ; i < FHT_N ; i++) { // save 256 samples
          while(!(ADCSRA & 0x10)); // wait for adc to be ready
          ADCSRA = 0xf5; // restart adc
          byte m = ADCL; // fetch adc data
          byte j = ADCH;
          int k = (j << 8) | m; // form into an int
          k -= 0x0200; // form into a signed int
          k <<= 6; // form into a 16b signed int
          fht_input[i] = k; // put real data into bins
          }
          
          
          fht_window(); // window the data for better frequency response
          fht_reorder(); // reorder the data before doing the fht
          fht_run(); // process the data in the fht
          fht_mag_octave(); 
          int switchboi = digitalRead(12);
          int switchboi2 = digitalRead(9);        
          if ((switchboi == 1) && (switchboi2 == 1)){ //check toggle swtiches
            int maparray[] = {0,0,0,0,0,0};
            //manually tune parameters
            maparray[0] = map(fht_oct_out[0],217,165,0,15);
            maparray[1] = map(fht_oct_out[1],190,210,0,15);
            maparray[2] = map(fht_oct_out[2],40,200,0,15);  
            maparray[3] = map(fht_oct_out[3],50,180,0,15); 
            maparray[4] = map(fht_oct_out[4],45,160,0,15); 
            maparray[5] = map(fht_oct_out[5],35,120,0,15);
            for (int m=0; m <=6; m++){ //iterate through frequency bins
              for(int j=(26-(4*m)); j >= (26-(4*(m+1))); j--){ //iterate across matrix
                  for(int i=0; i <= maparray[m]; i++){
                    if ((lookuptable[i][j] != 0) && (i < 16) && (j >=0)) { //find corresponding value in loopup table
                      strip.setPixelColor(lookuptable[i][j]-1, r,g,b); //set coluour
                    }
                    for(byte k = 15; k > maparray[m]; --k){ //turn all the other LEDs off
                      if ((lookuptable[k][j] != 0) && (j >=0)){
                      strip.setPixelColor(lookuptable[k][j]-1, 0,0,0);
                      
                      }}}}}return;}  //exit function
          else if ((switchboi == 1) && (switchboi2 == 0)){ //same as previous but with x and y reversed
            int maparray2[] = {0,0,0,0,0,0};
          //manually tune parameters
          maparray2[0] = map(fht_oct_out[0],217,165,0,(y-2));
          maparray2[1] = map(fht_oct_out[1],190,210,0,(y-2));
          maparray2[2] = map(fht_oct_out[2],40,200,0,(y-2));  
          maparray2[3] = map(fht_oct_out[3],50,180,0,(y-2)); 
          maparray2[4] = map(fht_oct_out[4],45,160,0,(y-2)); 
          maparray2[5] = map(fht_oct_out[5],35,120,0,(y-2));
              for (int m=0; m <=6; m++){
                for(int j=17-(3*m); j >= 17-(3*(m+1)); j--){
                    for(int i=0; i <= maparray2[m]; i++){
                      if ((lookuptable[j][25-i] != 0) && (i < 25) && (j >=0)) {
                        strip.setPixelColor(lookuptable[j][25-i]-1, r,g,b);
                      }
                      for(int k = 25; k > maparray2[m]; k--){
                        if ((lookuptable[j][25-k] != 0) && (i >=0)){
                        strip.setPixelColor(lookuptable[j][25-k]-1, 0,0,0);
                        strip.setPixelColor(151, r,g,b);
                        strip.setPixelColor(97, r,g,b);
                        
                        }}}}}return;}
                else {
                  resetFunc(); //reset arduino
                  }}}
                

void reset(){//clean out variables
  index = 0;//reset index
  noMatch = 0;//reset match couner
  maxSlope = 0;//reset slope
}    
void stringCheck(){

}
   
void frequencyCheck(){ // function for checking how far away the frequency is from the correct value
  if(frequency>correctFrequency+1.5){
    for(int j=17; j <= 19; j++){
      for(int i=0; i <= 15; i++){
         if ((lookuptable[i][j] != 0) && (i <=15)&& (j <= 19)) { //illuminate bar of LEDs depending on how far flat or sharp the note it
          strip.setPixelColor(lookuptable[i][17]-1, 51, 255, 0);
          strip.setPixelColor(lookuptable[i][18]-1, 102, 255, 0);
          strip.setPixelColor(lookuptable[i][19]-1, 153, 255, 0);
         
         }
  }
  }
  }
  if(frequency>correctFrequency+6){
//    analogWrite(A2,255);
     for(int j=20; j <= 22; j++){
       for(int i=0; i <= 15; i++){
         if ((lookuptable[i][j] != 0) && (i <=15)&& (j <= 22)) {
           strip.setPixelColor(lookuptable[i][20]-1, 204,255,0);
          strip.setPixelColor(lookuptable[i][21]-1, 255,255,0);
          strip.setPixelColor(lookuptable[i][22]-1, 255,192,0);
         }
  }
  }
  }
  if(frequency>correctFrequency+9){
//    analogWrite(A1,255);
    for(int j=23; j <= 25; j++){
      for(int i=0; i <= 15; i++){
         if ((lookuptable[i][j] != 0) && (i <=15)&& (j <= 25)) {
          strip.setPixelColor(lookuptable[i][23]-1, 255,128,0);
          strip.setPixelColor(lookuptable[i][24]-1, 255,64,0);
          strip.setPixelColor(lookuptable[i][25]-1, 255,0,0);

         }
  }
  }
  }
  if(frequency<correctFrequency-1.5){
//    analogWrite(A5,255);
      for(int j=11; j <= 13; j++){
        for(int i=0; i <= 15; i++){
         if ((lookuptable[i][j] != 0) && (i <=15)&& (j <= 13)) {
            strip.setPixelColor(lookuptable[i][13]-1, 51,255,0);
          strip.setPixelColor(lookuptable[i][12]-1, 102,255,0);
          strip.setPixelColor(lookuptable[i][11]-1, 153,255,0);
         }
  }
  }
  }
  if(frequency<correctFrequency-6){
//    digitalWrite(9,1);
      for(int j=8; j <= 10; j++){
       for(int i=0; i <= 15; i++){
         if ((lookuptable[i][j] != 0) && (i <=15)&& (j <= 10)) {
          strip.setPixelColor(lookuptable[i][10]-1, 204,255,0);
          strip.setPixelColor(lookuptable[i][9]-1, 255,255,0);
          strip.setPixelColor(lookuptable[i][8]-1, 255,192,0);
         }
  }
  }
  }
  if(frequency<correctFrequency-9){
//    digitalWrite(8,1);
    for(int j=0; j <= 7; j++){
      for(int i=0; i <= 15; i++){
         if ((lookuptable[i][j] != 0) && (i <=15)&& (j <= 7)) {
          strip.setPixelColor(lookuptable[i][7]-1, 255,128,0);
          strip.setPixelColor(lookuptable[i][6]-1, 255,64,0);
          strip.setPixelColor(lookuptable[i][5]-1, 255,0,0);
          strip.setPixelColor(lookuptable[i][4]-1, 255,0,0);
          strip.setPixelColor(lookuptable[i][3]-1, 255,0,0);
          strip.setPixelColor(lookuptable[i][2]-1, 255,0,0);
          strip.setPixelColor(lookuptable[i][1]-1, 255,0,0);
          strip.setPixelColor(lookuptable[i][0]-1, 255,0,0);
         }
  }
  }
  }
  if(frequency>correctFrequency-1.5&frequency<correctFrequency+1.5){
//    analogWrite(A4,255);
  for(int j=14; j <= 16; j++){
    for(int i=0; i <= 15; i++){
         if ((lookuptable[i][j] != 0) && (i <=15)&& (j <= 16)) {
          strip.setPixelColor(lookuptable[i][j]-1, 0,255,0);
         }
  }
  }
  }
} 
void loop(){
  
  int switchboi = digitalRead(12);
  int switchboi2 = digitalRead(9);
  allLEDsOff();
  if (switchboi == 1){
      
    unsigned int rgbColour[3]={}; //cycle through the RGB colour spectrum
    for (int decColour = 0; decColour < 3; decColour += 1) {
    int incColour = decColour == 2 ? 0 : decColour + 1;
    for(int i = 0; i < 255; i += 1) {
      rgbColour[decColour] -= 1;
      rgbColour[incColour] += 1;
      visualiserplay(rgbColour[0],rgbColour[1],rgbColour[2]); //pass RGB values to the visualiser display function
  }}}
  if (switchboi == 0){

  //set up continuous sampling of analog pin 0 at 38.5kHz
   cli();//disable interrupts
  //clear ADCSRA and ADCSRB registers
  ADCSRA = 0;
  ADCSRB = 0;
  
  ADMUX |= (1 << REFS0); //set reference voltage
  ADMUX |= (1 << ADLAR); //left align the ADC value- so we can read highest 8 bits from ADCH register only
  
  ADCSRA |= (1 << ADPS2) | (1 << ADPS0); //set ADC clock with 32 prescaler- 16mHz/32=500kHz
  ADCSRA |= (1 << ADATE); //enabble auto trigger
  ADCSRA |= (1 << ADIE); //enable interrupts when measurement complete
  ADCSRA |= (1 << ADEN); //enable ADC
  ADCSRA |= (1 << ADSC); //start ADC measurements
    sei();//enable interrupts



  allLEDsOff();
  if (checkMaxAmp>ampThreshold){
    frequency = 38462/float(period);//calculate frequency timer rate/period
  }}
    
    if(frequency>70&frequency<90){ //This is for low E
          frequencyCheck();
    strip.setPixelColor(196, 255,255,255);
    strip.setPixelColor(195, 255,255,255);
    strip.setPixelColor(194, 255,255,255);
    strip.setPixelColor(193, 255,255,255);
    strip.setPixelColor(191, 255,255,255);
    strip.setPixelColor(148, 255,255,255);
    strip.setPixelColor(145, 255,255,255);
    strip.setPixelColor(106, 255,255,255);
    strip.setPixelColor(107, 255,255,255);
    strip.setPixelColor(108, 255,255,255);
    strip.setPixelColor(109, 255,255,255);
    strip.setPixelColor(149, 255,255,255);
    strip.setPixelColor(150, 255,255,255);
    strip.setPixelColor(151, 255,255,255);
      strip.show();

    correctFrequency = 82.4;
  }
  else if(frequency>100&frequency<120){ //This is for A
    frequencyCheck();
    strip.setPixelColor(109, 255,255,255);
    strip.setPixelColor(142, 255,255,255);
    strip.setPixelColor(143, 255,255,255);
    strip.setPixelColor(144, 255,255,255);
    strip.setPixelColor(145, 255,255,255);
    strip.setPixelColor(106, 255,255,255);
    strip.setPixelColor(148, 255,255,255);
    strip.setPixelColor(191, 255,255,255);
    strip.setPixelColor(194, 255,255,255);
    strip.setPixelColor(195, 255,255,255);
    strip.setPixelColor(188, 255,255,255);
    strip.setPixelColor(151, 255,255,255);
      strip.show();
    correctFrequency = 110;
  }
  else if(frequency>135&frequency<155){ //This is for D
    frequencyCheck();
    strip.setPixelColor(188, 255,255,255);
    strip.setPixelColor(195, 255,255,255);
    strip.setPixelColor(194, 255,255,255);
    strip.setPixelColor(193, 255,255,255);
    strip.setPixelColor(191, 255,255,255);
    strip.setPixelColor(148, 255,255,255);
    strip.setPixelColor(145, 255,255,255);
    strip.setPixelColor(106, 255,255,255);
    strip.setPixelColor(107, 255,255,255);
    strip.setPixelColor(108, 255,255,255);
    strip.setPixelColor(142, 255,255,255);
    strip.setPixelColor(151, 255,255,255);
      strip.show();
    correctFrequency = 146.8;
  }
  else if(frequency>186&frequency<205){ //This is for G
    frequencyCheck();
    strip.setPixelColor(150, 255,255,255);
    strip.setPixelColor(151, 255,255,255);
    strip.setPixelColor(142, 255,255,255);
    strip.setPixelColor(109, 255,255,255);
    strip.setPixelColor(108, 255,255,255);
    strip.setPixelColor(107, 255,255,255);
    strip.setPixelColor(145, 255,255,255);
    strip.setPixelColor(148, 255,255,255);
    strip.setPixelColor(191, 255,255,255);
    strip.setPixelColor(194, 255,255,255);
    strip.setPixelColor(195, 255,255,255);
    strip.setPixelColor(196, 255,255,255);  
      strip.show();
    correctFrequency = 196;
  }
  else if(frequency>235&frequency<255){ //This is for B
//    otherLEDsOff(2,4,5,6,7);
//    digitalWrite(6,1);
    frequencyCheck();
    strip.setPixelColor(188, 255,255,255);
    strip.setPixelColor(195, 255,255,255);
    strip.setPixelColor(194, 255,255,255);
    strip.setPixelColor(193, 255,255,255);
    strip.setPixelColor(191, 255,255,255);
    strip.setPixelColor(148, 255,255,255);
    strip.setPixelColor(145, 255,255,255);
    strip.setPixelColor(106, 255,255,255);
    strip.setPixelColor(107, 255,255,255);
    strip.setPixelColor(108, 255,255,255);
    strip.setPixelColor(142, 255,255,255);
    strip.setPixelColor(149, 255,255,255);
    strip.setPixelColor(150, 255,255,255);
      strip.show();
    correctFrequency = 246.9;
  }
  else if(frequency>320&frequency<340){ //This is for high E
    frequencyCheck();
    strip.setPixelColor(196, 255,255,255);
    strip.setPixelColor(195, 255,255,255);
    strip.setPixelColor(194, 255,255,255);
    strip.setPixelColor(193, 255,255,255);
    strip.setPixelColor(191, 255,255,255);
    strip.setPixelColor(148, 255,255,255);
    strip.setPixelColor(145, 255,255,255);
    strip.setPixelColor(106, 255,255,255);
    strip.setPixelColor(107, 255,255,255);
    strip.setPixelColor(108, 255,255,255);
    strip.setPixelColor(109, 255,255,255);
    strip.setPixelColor(149, 255,255,255);
    strip.setPixelColor(150, 255,255,255);
    strip.setPixelColor(151, 255,255,255);
    strip.show();
    correctFrequency = 329.6;
  }
    delay(100);


  if (switchboi == 1){
    resetFunc();}} //reset if switch has changed
