#include <Adafruit_NeoPixel.h>
#include <arduinoFFT.h>

#ifdef __AVR__
	#include <avr/power.h>	// Required for 16 MHz Adafruit Trinket
#endif

#define SAMPLES 64		// Must be a power of 2
#define RGB_PIN 6		// On Trinket or Gemma, suggest changing this to 1
#define BTN_PIN 12
#define NUMPIXELS 32	// Popular NeoPixel ring size
#define yres 8			// Total number of rows in the display

// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.
Adafruit_NeoPixel pixels(NUMPIXELS, RGB_PIN, NEO_GRB + NEO_KHZ800);
arduinoFFT FFT = arduinoFFT();		// FFT object

char MY_ARRAY[]={0, 8, 16, 32, 48, 64, 96, 128, 160}; // default = standard pattern
//double ratio[] = {1, 0.86, 0.72, 0.58, 0.44, 0.30, 0.15, 0};
double ratio = 1.0/NUMPIXELS;
char count[NUMPIXELS];
char statel[NUMPIXELS];

double vReal[SAMPLES];
double vImag[SAMPLES];
double data_avgs[NUMPIXELS];

int yvalue;
int displayvalue;
int peaks[NUMPIXELS];
int state = HIGH;             // the current reading from the input pin
int previousState = LOW;   // the previous reading from the input pin
int displaymode = 0;
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

void setup(){
	ADCSRA = 0b11100110;		// set ADC to free running mode and set pre-scalar to 32 (0xe5)
	ADMUX = 0b00000000;			// use pin A0 and external voltage reference
	pinMode(RGB_PIN, OUTPUT);
	pinMode(BTN_PIN, INPUT);
	pixels.begin();				// INITIALIZE NeoPixel strip object (REQUIRED)
	delay(50);					// wait to get reference voltage stabilized
}

void loop(){
   // ++ Sampling
   for(int i=0; i<SAMPLES; i++)
    {
      while(!(ADCSRA & 0x10));        // wait for ADC to complete current conversion ie ADIF bit set
      ADCSRA = 0b11110110;               // clear ADIF bit so that ADC can do next operation (0xf5)
      int value = ADC - 512 ;                 // Read from ADC and subtract DC offset caused value
      vReal[i]= value/8;                      // Copy to bins after compressing
      vImag[i] = 0;                         
    }
    // -- Sampling
 
    // ++ FFT
    FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
    FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);
    // -- FFT
    
    // ++ re-arrange FFT result to match with no. of columns on display ( xres )
    int step = (SAMPLES/2)/NUMPIXELS; 
    int c=0;
    for(int i=0; i<(SAMPLES/2); i+=step)  
    {
      data_avgs[c] = 0;
      for (int k=0 ; k< step ; k++) {
          //data_avgs[c] = data_avgs[c] + vReal[i+k];
          data_avgs[c] = max(data_avgs[c], (int)vReal[i+k]);
      }
      //data_avgs[c] = data_avgs[c]/step; 
      c++;
    }
    // -- re-arrange FFT result to match with no. of columns on display ( xres )
    
    // ++ send to display according measured value
    for(int i=0; i<NUMPIXELS; i++)
    {
      data_avgs[i] = constrain(data_avgs[i],0,64);            // set max & min values for buckets
      data_avgs[i] = map(data_avgs[i], 0, 64, 0, yres);        // remap averaged values to yres
      yvalue=data_avgs[i];

      peaks[i] = peaks[i]-1;    // decay by one light
      if (yvalue > peaks[i]) 
          peaks[i] = yvalue ;
      yvalue = peaks[i];    
      displayvalue=MY_ARRAY[yvalue];

		if(displaymode==0){
			pixels.setPixelColor(i, pixels.Color(displayvalue*(1-ratio*i), displayvalue*ratio*i/2, displayvalue));
		}
		else if(displaymode==1){
			// pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
  		    // Here we're using a moderately bright green color:
     		 pixels.setPixelColor(i, pixels.Color(displayvalue, 0, displayvalue));
		}
		else if(displaymode==1){
			// pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
     		 // Here we're using a moderately bright green color:
    		 pixels.setPixelColor(i, pixels.Color(displayvalue*(statel[i]&0x1), displayvalue*(statel[i]&0x2), displayvalue*(statel[i]&0x4)));
   			 if(!yvalue==0){
      			count[i]++;
      			if(count[i]>=50){
      				count[i] = 0;
      				statel[i]++;
      				if(statel[i]>=8){
      					statel[i]=1;
      				}
      			}
      		}
		}
		
      pixels.show();   // Send the updated pixel colors to the hardware.
     }
     // -- send to display according measured value 
     
    displayModeChange ();         // check if button pressed to change display mode
} 

void displayModeChange() {
  int reading = digitalRead(BTN_PIN);
  if (reading == HIGH && previousState == LOW && millis() - lastDebounceTime > debounceDelay) // works only when pressed
  {
   switch (displaymode) {
    case 0:    //       move from mode 1 to 2
      displaymode = 1;
      break;
	case 1:    //       move from mode 1 to 2
      displaymode = 2;
      break;
    case 2:    //       move from mode 2 to 3
    	for(int i = 0; i < NUMPIXELS; ++i){
    		count[i] = 0;
    		statel[i] = 1;
    	}
      displaymode = 0;
      break;
   }
    lastDebounceTime = millis();
  }
  previousState = reading;
}

/*void displayModeChange() {
  int reading = digitalRead(BTN_PIN);
  if (reading == HIGH && previousState == LOW && millis() - lastDebounceTime > debounceDelay) // works only when pressed
  
  {

   switch (displaymode) {
    case 1:    //       move from mode 1 to 2
      displaymode = 2;
      for (int i=0 ; i<=8 ; i++ ) {
        MY_ARRAY[i]=MY_MODE_2[i];
      }
      break;
    case 2:    //       move from mode 2 to 3
      displaymode = 3;
      for (int i=0 ; i<=8 ; i++ ) {
        MY_ARRAY[i]=MY_MODE_3[i];
      }
      break;
    case 3:    //     move from mode 3 to 4
      displaymode = 4;
      for (int i=0 ; i<=8 ; i++ ) {
        MY_ARRAY[i]=MY_MODE_4[i];
      }
      break;
    case 4:    //     move from mode 4 to 5
      displaymode = 5;
      for (int i=0 ; i<=8 ; i++ ) {
        MY_ARRAY[i]=MY_MODE_5[i];
      }
      break;
    case 5:    //      move from mode 5 to 1
      displaymode = 1;      
      for (int i=0 ; i<=8 ; i++ ) {
        MY_ARRAY[i]=MY_MODE_1[i];
      }
      break;
  }

    lastDebounceTime = millis();
  }
  previousState = reading;
}*/
