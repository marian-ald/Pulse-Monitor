/*
 * PM - 2018
 * Aldescu Marian 331CA
 */
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <avr/interrupt.h>

#include "usart.h"
#include "lcd.h"


char buf[32];


//  Variables
int pulsePin = 0;                  // Pulse Sensor purple wire connected to analog pin 0
int blinkPin = 13;                 // pin to blink led at each beat

// Volatile Variables, used in the interrupt service routine!
volatile int BPM;                   // int that holds raw Analog in 0. updated every 2mS
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // int that holds the time interval between beats! Must be seeded! 
volatile int Pulse = 0;     		// "True" when User's live heartbeat is detected. "False" when not a "live beat". 
volatile int QS = 0;        		// becomes true when Arduoino finds a beat.

volatile int rate[10];              		// array to hold last ten IBI values
volatile unsigned long sampleCounter = 0;   // used to determine pulse timing
volatile unsigned long lastBeatTime = 0;    // used to find IBI
volatile int P = 512;                       // used to find peak in pulse wave, seeded
volatile int T = 512;                       // used to find trough in pulse wave, seeded
volatile int thresh = 525;                  // used to find instant moment of heart beat, seeded
volatile int amp = 100;                     // used to hold amplitude of pulse waveform, seeded
volatile int firstBeat = 1;          		// used to seed rate array so we startup with reasonable BPM
volatile int secondBeat = 1;      			// used to seed rate array so we startup with reasonable BPM
volatile int stare = 0;

/* Tensiunea de referinta utilizata de ADC. */
#define ADC_AREF_VOLTAGE 5

/*
 * Functia porneste o noua conversie pentru canalul precizat.
 * In modul fara intreruperi, apelul functiei este blocant. Aceasta se
 * intoarce cand conversia este finalizata.
 * In modul cu intreruperi apelul NU este blocant.
 * @return Valoarea numerica raw citita de ADC de pe canlul specificat.
 */
uint16_t ADC_get(uint8_t channel)
{
    // start ADC conversion on "channel"
    // wait for completion
    // return the result
    ADMUX = (ADMUX & ~(0x1f << MUX0)) | channel;

    ADCSRA |= (1 << ADSC);
    while(ADCSRA & (1 << ADSC));

    return ADC;
    (void)channel;
}

ISR(PCINT3_vect)
{
	_delay_ms(2);
    if ((PIND & (1 << PD6)) == 0)
    {
        stare ^= 1;
    }
}

void treci_in_stop(void) {
    LCD_writeInstr(LCD_INSTR_clearDisplay);
	LCD_printAt(0x00, "Buton=start/stop");
}

//triggered when Timer2 counts to 124
ISR(TIMER2_COMPA_vect)
{
  cli();                                      // disable interrupts while we do this
  Signal = ADC_get(0);
   int runningTotal = 0;
  sampleCounter += 2;                         // keep track of the time in mS with this variable
  int N = sampleCounter - lastBeatTime;       // monitor the time since the last beat to avoid noise
                                              //  find the peak and trough of the pulse wave
  if(Signal < thresh && N > (IBI/5)*3) 		  // avoid dichrotic noise by waiting 3/5 of last IBI
    {      
      if (Signal < T) 						  // T is the trough
      {                        
        T = Signal; 						  // keep track of lowest point in pulse wave 
      }
    }

  if(Signal > thresh && Signal > P)
    {          // thresh condition helps avoid noise
      P = Signal;                             // P is the peak
    }                                         // keep track of highest point in pulse wave

  //  NOW IT'S TIME TO LOOK FOR THE HEART BEAT
  // signal surges up in value every time there is a pulse
  if (N > 250)
  {                                   		 // avoid high frequency noise
    if ( (Signal > thresh) && (Pulse == false) && (N > (IBI/5)*3) )
      {        
        Pulse = true;                        // set the Pulse flag when we think there is a pulse
        if (stare) {	
        	if (BPM < 100) {
    			PORTD |= (1 << PD7);
    			PORTB |= (1 << PB7);
    		} else {
    			LCD_printAt(0x40, "Calibrare...");
	    	}
	    } else {
	    	treci_in_stop();
	    }

        IBI = sampleCounter - lastBeatTime;         // measure time between beats in mS
        lastBeatTime = sampleCounter;               // keep track of time for next pulse
  
        if(secondBeat)
        {                        // if this is the second beat, if secondBeat == TRUE
          secondBeat = false;                  // clear secondBeat flag
          for(int i=0; i<=9; i++) // seed the running total to get a realisitic BPM at startup
          {             
            rate[i] = IBI;                      
          }
        }
  
        if(firstBeat) // if it's the first time we found a beat, if firstBeat == TRUE
        {                         
          firstBeat = false;                   // clear firstBeat flag
          secondBeat = true;                   // set the second beat flag
          sei();                               // enable interrupts again
          return;                              // IBI value is unreliable so discard it
        }   
      // keep a running total of the last 10 IBI values
      runningTotal = 0;                  // clear the runningTotal variable    

      for(int i=0; i<=8; i++)
        {                // shift data in the rate array
          rate[i] = rate[i+1];                  // and drop the oldest IBI value 
          runningTotal += rate[i];              // add up the 9 oldest IBI values
        }

      rate[9] = IBI;                          // add the latest IBI to the rate array
      runningTotal += rate[9];                // add the latest IBI to runningTotal
      runningTotal /= 10;                     // average the last 10 IBI values 
      BPM = 60000/runningTotal;               // how many beats can fit into a minute? that's BPM!
      QS = true;                              // set Quantified Self flag 
      // QS FLAG IS NOT CLEARED INSIDE THIS ISR
    }                       
  }

  if (Signal < thresh && Pulse == true)
    {   // when the values are going down, the beat is over
      
	PORTD &= ~(1 << PD7);
	PORTB &= ~(1 << PB7);

      Pulse = false;                         // reset the Pulse flag so we can do it again
      amp = P - T;                           // get amplitude of the pulse wave
      thresh = amp/2 + T;                    // set thresh at 50% of the amplitude
      P = thresh;                            // reset these for next time
      T = thresh;
    }

  if (N > 2500)
    {                           			 // if 2.5 seconds go by without a beat
      thresh = 512;                          // set thresh default
      P = 512;                               // set P default
      T = 512;                               // set T default
      lastBeatTime = sampleCounter;          // bring the lastBeatTime up to date        
      firstBeat = true;                      // set these to avoid noise
      secondBeat = false;                    // when we get the heartbeat back
    }

  sei();                                   // enable interrupts when youre done!
}

/*
 * Functia initializeaza convertorul Analog-Digital.
 */
void ADC_init(void)
{
    // enable ADC with:
    // * reference AVCC with external capacitor at AREF pin
    // * without left adjust of conversion result
    // * no auto-trigger
    // * no interrupt
    // * prescaler at 32
    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (5 << ADPS0);
}

/*
 * Functia primeste o valoare numerica raw citita de convertul Analog-Digital
 * si calculeaza tensiunea (in volti) pe baza tensiunei de referinta.
 */
double ADC_voltage(int raw)
{
    (void)raw;
    return raw * ADC_AREF_VOLTAGE / 1023.0;
}

/*
 * Functia afiseaza pe display-ul grafic numele canalelor ADC si valorile
 * citite de la acestea.
 */
void ADC_show(void)
{
	int val = (int)(ADC_get(0));
	sprintf(buf, "%d\n", val);
}

void serialOutputWhenBeatHappens() {    
    
	sprintf(buf, "BPM: %d", BPM);
	if (stare) {
		LCD_writeInstr(LCD_INSTR_clearDisplay);
		LCD_printAt(0x00, buf);

		if (BPM > 90)
		LCD_printAt(0x40, "Calibrare...");
	} else {
		treci_in_stop();
	}
}

void interruptSetup() {
  // Initializes Timer2 to throw an interrupt every 2mS.
  TCCR2A = 0x02;     // DISABLE PWM ON DIGITAL PINS 3 AND 11, AND GO INTO CTC MODE
  TCCR2B = 0x06;     // DON'T FORCE COMPARE, 256 PRESCALER 
  OCR2A = 0X7C;      // SET THE TOP OF THE COUNT TO 124 FOR 500Hz SAMPLE RATE
  TIMSK2 = 0x02;     // ENABLE INTERRUPT ON MATCH BETWEEN TIMER2 AND OCR2A
  sei();             // MAKE SURE GLOBAL INTERRUPTS AREx` ENABLED      
}

void setup(void) {
	interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS
	/* led LCD */
	DDRC |= 1 << PC2;
	PORTC |= 1 << PC2;

	/* Buzzer */
	DDRB |= 1 << PB7;
	PORTB &= ~(1 << PB7);

	DDRD &= ~(1 << PD6);
    PORTD |= (1 << PD6);

    // Activez PIN CHANGE INTERRUPT ENABLE pentru PCINT15:8 (portul B) si PCINT24:31 (portul D)
    PCICR |= (1 << PCIE3);
    // Activez intreruperea pe butoanele PB2 si PD6
    PCMSK3 |= (1 << PCINT30);
}

int main(void) {
	/* Setăm pinul 4 al portului C ca pin de ieșire. */
	DDRD |= (1 << PD7);
	
	sprintf(buf, "%d", 12345);
	
	USART0_init();
	ADC_init();
	LCD_init();

	/* Sting LED-ul. */

	setup();
	while(1) {
		//_delay_ms(100);

		 ADC_show();
		USART0_print(buf);
		if (QS == true) { // A Heartbeat Was Found {     
			// BPM and IBI have been Determined
			// Quantified Self "QS" true when arduino finds a heartbeat	      
	    	serialOutputWhenBeatHappens(); // A Beat Happened, Output that to serial.     
	    	QS = false; // reset the Quantified Self flag for next time    
	    }
	 	_delay_ms(20); //  take a break
	}
	return 0;
}
