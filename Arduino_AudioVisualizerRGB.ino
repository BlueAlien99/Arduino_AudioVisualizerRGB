#include <Adafruit_NeoPixel.h>
#include <arduinoFFT.h>

#ifdef __AVR__
	#include <avr/power.h>	// Required for 16 MHz Adafruit Trinket
#endif

#define SAMPLES 64		// Must be a power of 2
#define RGB_PIN 6		// On Trinket or Gemma, suggest changing this to 1
#define BTN_PIN 12
#define NUMPIXELS 8		// Popular NeoPixel ring size
#define yres 8			// Total number of rows in the display

// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.
Adafruit_NeoPixel pixels(NUMPIXELS, RGB_PIN, NEO_GRB + NEO_KHZ800);
arduinoFFT FFT = arduinoFFT();		// FFT object

int MY_ARRAY[]={0, 8, 16, 32, 48, 64, 96, 128, 160}; // default = standard pattern
char count[]={0,0,0,0,0,0,0,0};
char statel[]={1,1,1,1,1,1,1,1};
int MY_MODE_1[]={0, 128, 192, 224, 240, 248, 252, 254, 255}; // standard pattern
int MY_MODE_2[]={0, 128, 64, 32, 16, 8, 4, 2, 1}; // only peak pattern
int MY_MODE_3[]={0, 128, 192, 160, 144, 136, 132, 130, 129}; // only peak +  bottom point
int MY_MODE_4[]={0, 128, 192, 160, 208, 232, 244, 250, 253}; // one gap in the top , 3rd light onwards
int MY_MODE_5[]={0, 1, 3, 7, 15, 31, 63, 127, 255}; // standard pattern, mirrored vertically

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

void setup() {
    ADCSRA = 0b11100110;      // set ADC to free running mode and set pre-scalar to 32 (0xe5)
    ADMUX = 0b00000000;       // use pin A0 and external voltage reference
    pinMode(RGB_PIN, OUTPUT);
    pinMode(BTN_PIN, INPUT);
    pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  Serial.begin(115200);
  Serial.println("Ready");
    delay(50);            // wait to get reference voltage stabilized
}
 
void loop() {
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
    Serial.println("Computed magnitudes:");
  PrintVector(vReal, (SAMPLES >> 1), 2);
  //double x = FFT.MajorPeak(vReal, SAMPLES, samplingFrequency);
  //Serial.println(x, 6); //Print out what frequency is the most dominant.

    
    // ++ re-arrange FFT result to match with no. of columns on display ( xres )
    int step = (SAMPLES/2)/NUMPIXELS; 
    int c=0;
    for(int i=0; i<(SAMPLES/2); i+=step)  
    {
      data_avgs[c] = 0;
      for (int k=0 ; k< step ; k++) {
          //data_avgs[c] = data_avgs[c] + vReal[i+k];
          data_avgs[c] = max(data_avgs[c], (int)vReal[i+k]);
          if(i==0){
            Serial.println(data_avgs[c]);
          }
      }
      //Serial.println(data_avgs[c], 6);
      //data_avgs[c] = data_avgs[c]/step; 
      c++;
    }
    // -- re-arrange FFT result to match with no. of columns on display ( xres )

    
    // ++ send to display according measured value
    if(displaymode==0){ 
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

      // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
      // Here we're using a moderately bright green color:
      pixels.setPixelColor(i, pixels.Color(displayvalue, 0, displayvalue));
  
      pixels.show();   // Send the updated pixel colors to the hardware.
     }
    }
    else if(displaymode==1){
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
  
      pixels.show();   // Send the updated pixel colors to the hardware.
     }
    }
     // -- send to display according measured value 
     
    displayModeChange ();         // check if button pressed to change display mode
} 

const double samplingFrequency = 16000000.0/(13*SAMPLES); //Hz, must be less than 10000 due to ADC
unsigned int sampling_period_us = round(1000000*(1.0/samplingFrequency));
unsigned long microseconds;

void PrintVector(double *vData, uint16_t bufferSize, uint8_t scaleType)
{
  for (uint16_t i = 0; i < bufferSize; i++)
  {
    double abscissa;
    /* Print abscissa value */
    switch (scaleType)
    {
      case 0://index
        abscissa = (i * 1.0);
  break;
      case 1://time
        abscissa = ((i * 1.0) / samplingFrequency);
  break;
      case 2://frequency
        abscissa = ((i * 1.0 * samplingFrequency) / SAMPLES);
  break;
    }
    Serial.print(abscissa, 6);
    if(scaleType==2)
      Serial.print("Hz");
    Serial.print(" ");
    Serial.println(vData[i], 4);
  }
  Serial.println();
}

void displayModeChange() {
  int reading = digitalRead(BTN_PIN);
  if (reading == HIGH && previousState == LOW && millis() - lastDebounceTime > debounceDelay) // works only when pressed
  {
   switch (displaymode) {
    case 0:    //       move from mode 1 to 2
      displaymode = 1;
      break;
    case 1:    //       move from mode 2 to 3
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
