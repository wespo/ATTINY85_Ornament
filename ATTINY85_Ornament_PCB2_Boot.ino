//Includes

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

#define INTERRUPTPIN PCINT1 //this is PB1 per the schematic
#define PCINT_VECTOR PCINT0_vect  //this step is not necessary
#define DATADIRECTIONPIN DDB1 //Page 64 of data sheet
#define PORTPIN PB1 //Page 64
#define READPIN PINB1 //page 64
#define LEDPIN1 4 //PB4
#define LEDPIN2 3 //PB3

#define RX    3   // *** D3, Pin 2
#define TX    0   // *** D4, Pin 3

#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit)) //OR
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit)) //AND

/*
 * Alias for the ISR: "PCINT_VECTOR" (Note: There is only one PCINT ISR. 
 * PCINT0 in the name for the ISR was confusing to me at first, 
 * hence the Alias, but it's how the datasheet refers to it)
 */
static volatile byte Blinking = 1; //variable used within ISR must be declared Volatile.
void enterSleep(void)
{
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  
  sleep_enable();


  /* Disable all of the unused peripherals. This will reduce power
   * consumption further and, more importantly, some of these
   * peripherals may generate interrupts that will wake our Arduino from
   * sleep!
   */
  cbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter OFF
  power_timer0_disable();
  /* Now enter sleep mode. */

  sleep_mode();
  sleep_cpu();
  /* The program will continue from here after the timer timeout*/

  sleep_disable(); /* First thing to do is disable sleep. */
  
  /* Re-enable the peripherals. */
  sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter OFF
  power_timer0_enable();
}

// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
void wdtSleep(int ii)
{
  
  noInterrupts();
  


  byte bb;
  int ww;
  if (ii > 9 ) ii=9;
  bb=ii & 7;
  if (ii > 7) bb|= (1<<5);
  bb|= (1<<WDCE);
  ww=bb;

  MCUSR &= ~(1<<WDRF);
  // start timed sequence
  WDTCR |= (1<<WDCE) | (1<<WDE);
  // set new watchdog timeout value
  WDTCR = bb;
  WDTCR |= _BV(WDIE);
  
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);
  sleep_bod_disable();
  interrupts();
  sleep_mode();     

}
 

void setup() {
    cli();//disable interrupts during setup
    pinMode(LEDPIN1, OUTPUT); //we can use standard arduino style for this as an example
    pinMode(LEDPIN2 , OUTPUT); //we can use standard arduino style for this as an example
    pinMode(1, INPUT);
    digitalWrite(LEDPIN1, LOW); //set the LED to LOW
    digitalWrite(LEDPIN2, LOW); //set the LED to LOW
    
    PCMSK |= (1 << INTERRUPTPIN); //sbi(PCMSK,INTERRUPTPIN) also works but I think this is more clear // tell pin change mask to listen to pin2 /pb3 //SBI
    GIMSK |= (1 << PCIE);   // enable PCINT interrupt in the general interrupt mask //SBI

    DDRB &= ~(1 << DATADIRECTIONPIN); //cbi(DDRB, DATADIRECTIONPIN);//  set up as input  - pin2 clear bit  - set to zero
    PORTB |= (1<< PORTPIN); //cbi(PORTB, PORTPIN);// disable pull-up. hook up pulldown resistor. - set to zero
    sei(); //last line of setup - enable interrupts after setup

}

void loop() {
  // put your main code here, to run repeatedly:
  //If you connect a debounced pushbutton to PB2 and to VCC you can tap the button and the LED will come on
  //tap the button again and the LED will turn off.
  //
  if(Blinking)
  {
    for(int i = 0; i < 20; i++)
    {
      digitalWrite(LEDPIN1, HIGH);
      digitalWrite(LEDPIN2, LOW);
      wdtSleep(4);
      digitalWrite(LEDPIN1,LOW);
      digitalWrite(LEDPIN2,HIGH);
      wdtSleep(4);
    }
    digitalWrite(LEDPIN1,LOW);
    digitalWrite(LEDPIN2,LOW);
    Blinking = 0;
  }
  enterSleep();

}


//this is the interrupt handler
ISR(PCINT_VECTOR)
{
  //Since the PCINTn triggers on both rising and falling edge let's just looks for rising edge
  //i.e. pin goes to 5v
  byte pinState;
  pinState = (PINB >> READPIN)& 1; //PINB is the register to read the state of the pins
  if (pinState >0) //look at the pin state on the pin PINB register- returns 1 if high
  {
   if (Blinking == 0)
   {
    Blinking = 1;
   }
  }
}

ISR(WDT_vect)
{
  wdt_disable(); 
}
