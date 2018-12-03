/*
 * PPM generator originally written by David Hasko
 * on https://code.google.com/p/generate-ppm-signal/ 
 * updated by Harindra W Pradhana to modify WlToys Transmitter
 * 
 * Changelog
 * A0 input from throttle
 * A1 input from steering
 * A2 input from steering trim
 * A3 input from 3 position switch
 * A4 input from 3 position switch
 */

//////////////////////CONFIGURATION///////////////////////////////
#define CHANNEL_NUMBER 8  //set the number of chanels
#define CHANNEL_DEFAULT_VALUE 1500  //set the default servo value
#define FRAME_LENGTH 22500  //set the PPM frame length in microseconds (1ms = 1000µs)
#define PULSE_LENGTH 300  //set the pulse length
#define onState 1  //set polarity of the pulses: 1 is positive, 0 is negative
#define sigPin 10  //set PPM signal output pin on the arduino

// config based on your transmitter
#define thr_fwd 10
#define thr_ctr 490
#define thr_rev 800

#define str_lft 60
#define str_ctr 525
#define str_rgt 890
//////////////////////////////////////////////////////////////////

int ppm[CHANNEL_NUMBER];
int val,val0,val1,val2,val3,val4,revsteer = 0;

int currentChannelStep;

void setup(){  


  pinMode(sigPin, OUTPUT);
  digitalWrite(sigPin, !onState);  //set the PPM signal pin to the default state (off)
  
  cli();
  TCCR1A = 0; // set entire TCCR1 register to 0
  TCCR1B = 0;
  
  OCR1A = 100;  // compare match register, change this
  TCCR1B |= (1 << WGM12);  // turn on CTC mode
  TCCR1B |= (1 << CS11);  // 8 prescaler: 0,5 microseconds at 16mhz
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  sei();

  // to reverse steering, simply turn to left or right on startup
  val = analogRead(A1);
  if (val<400) revsteer=1;
  if (val>700) revsteer=1;

}



void loop(){
  
  // throttle
  val = analogRead(A0);
  val0 = (val0+val)/2; // smoothing
  if (val0<thr_ctr) ppm[0]=map(val0,thr_fwd,thr_ctr,2000,1500);
  else ppm[0]=map(val0,thr_ctr,thr_rev,1500,1000);

  // steering & trim
  val = analogRead(A1);
  val1 = (val1+val)/2; // smoothing
  val = analogRead(A2); // 0 ; 512 ; 1023
  val2 = (val2+val)/2; // smoothing
  int triming = map(val2,0,1023,-100,100); // trim limited to -100us - 100us
  int steering = 0;
  if (val1<str_ctr) steering=map(val1,str_lft,str_ctr,1100,1500);
  else steering=map(val1,str_ctr,str_rgt,1500,1900);
  if (revsteer==1) ppm[1]=steering+triming;
  else ppm[1]=3000+triming-steering;

  // CH 3
  val = analogRead(A3);
  val3 = (val3+val)/2; // smoothing
  ppm[2]=map(val3,0,1023,1000,2000);

  // CH 4
  val = analogRead(A4);
  val4 = (val4+val)/2; // smoothing
  ppm[3]=map(val4,0,1023,1000,2000);

  // reserved for more channel
  ppm[4]=1000;
  ppm[5]=1000;
  ppm[6]=1000;
  ppm[7]=1000;
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
