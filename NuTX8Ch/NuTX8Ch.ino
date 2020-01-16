/*
 * PPM generator originally written by David Hasko
 * on https://code.google.com/p/generate-ppm-signal/ 
 * updated by Harindra W Pradhana for 8 ch transmitter with PPM output
 * 
 * Changelog
 * A0 input from aileron
 * A1 input from elevator
 * A2 input from throttle
 * A3 input from rudder
 * A4 input from 6 position flight mode button
 * A5 input from trimpot
 * A6 input from trimpot
 * A8 input from 3 position switch
 */

//////////////////////CONFIGURATION///////////////////////////////
#define CHANNEL_NUMBER 8  //set the number of chanels
#define CHANNEL_DEFAULT_VALUE 1500  //set the default servo value
#define CHANNEL_MINIMUM_VALUE 1000 // set the minimum servo value
#define CHANNEL_MAXIMUM_VALUE 2000 // set the maximum servo value
#define FRAME_LENGTH 22500  //set the PPM frame length in microseconds (1ms = 1000µs)
#define PULSE_LENGTH 300  //set the pulse length
#define onState 1  //set polarity of the pulses: 1 is positive, 0 is negative
#define sigPin 10  //set PPM signal output pin on the arduino

//////////////////////////////////////////////////////////////////

int ppm[CHANNEL_NUMBER];
int minval[CHANNEL_NUMBER]={35,0,0,25,0,0,0,0};
int ctrval[CHANNEL_NUMBER]={509,398,460,545,512,512,512,512};
int maxval[CHANNEL_NUMBER]={1010,945,980,1020,1024,1024,1024,1024};
int val[CHANNEL_NUMBER]={0,0,0,0,0,0,0,0};
int lastModeVal,deltaModeVal,discreetMode;

void setup(){  

  Serial.begin(57600);
  pinMode(sigPin, OUTPUT);
  digitalWrite(sigPin, !onState);  //set the PPM signal pin to the default state (off)
  lastModeVal=125;
  discreetMode=0;
  
  cli();
  TCCR1A = 0; // set entire TCCR1 register to 0
  TCCR1B = 0;
  
  OCR1A = 100;  // compare match register, change this
  TCCR1B |= (1 << WGM12);  // turn on CTC mode
  TCCR1B |= (1 << CS11);  // 8 prescaler: 0,5 microseconds at 16mhz
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  sei();

}

void loop(){
  val[0] = analogRead(A0);
  delay(1);
  val[1] = analogRead(A1);
  delay(1);
  val[2] = analogRead(A2);
  delay(1);
  val[3] = analogRead(A3);
  delay(1);
  val[4] = analogRead(A4);
  delay(1);
  val[5] = analogRead(A5);
  delay(1);
  val[6] = analogRead(A6);
  delay(1);
  val[7] = analogRead(A7);
  delay(1);

  for (int i=0; i<CHANNEL_NUMBER; i++)
  {
    if (i==4)
    {
      if (val[4]<500)
      {
        deltaModeVal=lastModeVal-val[4];
        if ((deltaModeVal >- 50) && (deltaModeVal < 50))
        {
          lastModeVal+=val[4];
          lastModeVal/=2;
        }
        else lastModeVal=val[4];
        if(lastModeVal<180) discreetMode=5;
        else if(lastModeVal<260) discreetMode=4;
        else if(lastModeVal<340) discreetMode=3;
        else if(lastModeVal<400) discreetMode=2;
        else if(lastModeVal<450) discreetMode=1;
        else discreetMode=0;
      }
      ppm[i]=map(discreetMode,0,5,CHANNEL_MINIMUM_VALUE,CHANNEL_MAXIMUM_VALUE);
    }
    else if (val[i] < ctrval[i])
    {
      ppm[i]+=map(val[i],minval[i],ctrval[i],CHANNEL_MINIMUM_VALUE,CHANNEL_DEFAULT_VALUE);
      ppm[i]/=2;
    }
    else
    {
      ppm[i]+=map(val[i],ctrval[i],maxval[i],CHANNEL_DEFAULT_VALUE,CHANNEL_MAXIMUM_VALUE);
      ppm[i]/=2;
    }
    Serial.print(ppm[i]);
    Serial.print("|");
  }
  Serial.println();
  delay(50);
}

ISR(TIMER1_COMPA_vect){  //leave this alone
  static boolean state = true;
  
  TCNT1 = 0;
  
  if (state) {  //start pulse
    digitalWrite(sigPin, onState);
    OCR1A = PULSE_LENGTH * 2;
    state = false;
  } else{  //end pulse and calculate when to start the next pulse
    static byte cur_chan_numb;
    static unsigned int calc_rest;
  
    digitalWrite(sigPin, !onState);
    state = true;

    if(cur_chan_numb >= CHANNEL_NUMBER){
      cur_chan_numb = 0;
      calc_rest = calc_rest + PULSE_LENGTH;// 
      OCR1A = (FRAME_LENGTH - calc_rest) * 2;
      calc_rest = 0;
    }
    else{
      OCR1A = (ppm[cur_chan_numb] - PULSE_LENGTH) * 2;
      calc_rest = calc_rest + ppm[cur_chan_numb];
      cur_chan_numb++;
    }     
  }
}
