#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

//-------------------------------------------------------------------------------------
// macro for easier usage
#define read_eeprom_word(address) eeprom_read_word ((const uint16_t*)address)
#define write_eeprom_word(address,value) eeprom_write_word ((uint16_t*)address,(uint16_t)value)
#define update_eeprom_word(address,value) eeprom_update_word ((uint16_t*)address,(uint16_t)value)

//declare an eeprom array
unsigned char EEMEM  highscore;

// declare a ram array
unsigned char highscoreram;



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~LCD STUFF~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define SET_BIT(p,i) ((p) |= (1 << (i)))
#define CLR_BIT(p,i) ((p) &= ~(1 << (i)))
#define GET_BIT(p,i) ((p) & (1 << (i)))

/*-------------------------------------------------------------------------*/

#define DATA_BUS PORTD		// port connected to pins 7-14 of LCD display
#define CONTROL_BUS PORTA	// port connected to pins 4 and 6 of LCD disp.
#define RS 0			// pin number of uC connected to pin 4 of LCD disp.
#define E 1			// pin number of uC connected to pin 6 of LCD disp.

/*-------------------------------------------------------------------------*/

void LCD_WriteCommand (unsigned char Command) {
	CLR_BIT(CONTROL_BUS,RS);
	DATA_BUS = Command;
	SET_BIT(CONTROL_BUS,E);
	asm("nop");
	CLR_BIT(CONTROL_BUS,E);
	delay_ms(2); // ClearScreen requires 1.52ms to execute
}

void LCD_ClearScreen(void) {
	LCD_WriteCommand(0x01);
}

void LCD_init(void) {

	//wait for 100 ms.
	delay_ms(100);
	LCD_WriteCommand(0x38);
	LCD_WriteCommand(0x06);
	LCD_WriteCommand(0x0f);
	LCD_WriteCommand(0x01);
	delay_ms(10);
}

void LCD_WriteData(unsigned char Data) {
	SET_BIT(CONTROL_BUS,RS);
	DATA_BUS = Data;
	SET_BIT(CONTROL_BUS,E);
	asm("nop");
	CLR_BIT(CONTROL_BUS,E);
	delay_ms(1);
}

void LCD_DisplayString( unsigned char column, const char* string) {
	LCD_ClearScreen();
	unsigned char c = column;
	while(*string) {
		LCD_Cursor(c++);
		LCD_WriteData(*string++);
	}
}

void LCD_Cursor(unsigned char column) {
	if ( column < 17 ) { // 16x1 LCD: column < 9
		// 16x2 LCD: column < 17
		LCD_WriteCommand(0x80 + column - 1);
		} else {
		LCD_WriteCommand(0xB8 + column - 9);	// 16x1 LCD: column - 1
		// 16x2 LCD: column - 9
	}
}

void delay_ms(int miliSec) //for 8 Mhz crystal

{
	int i,j;
	for(i=0;i<miliSec;i++)
	for(j=0;j<775;j++)
	{
		asm("nop");
	}
}

//~~~~~~~~~~~~~~~~~~END LCD STUFF~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

unsigned char SetBit(unsigned char pin, unsigned char number, unsigned char bin_value)
{
	return (bin_value ? pin | (0x01 << number) : pin & ~(0x01 << number));
}

////////////////////////////////////////////////////////////////////////////////
//Functionality - Gets bit from a PINx
//Parameter: Takes in a uChar for a PINx and the pin number
//Returns: The value of the PINx
unsigned char GetBit(unsigned char port, unsigned char number)
{
	return ( port & (0x01 << number) );
}

char GetKeypadKey() {
	// Check keys in col 1
	// Enable col 4 with 0, disable others with 1?s
	// The delay allows PORTC to stabilize before checking
	PORTC = 0xEF;
	asm("nop");
	if (GetBit(PINC,0)==0) { return('D'); }
	if (GetBit(PINC,1)==0) { return('C'); }
	if (GetBit(PINC,2)==0) { return('B'); }
	if (GetBit(PINC,3)==0) { return('A'); }
	// Check keys in col 2
	// Enable col 5 with 0, disable others with 1?s
	// The delay allows PORTC to stabilize before checking
	PORTC = 0xDF;
	asm("nop");
	if (GetBit(PINC,0)==0) { return('#'); }
	if (GetBit(PINC,1)==0) { return('9'); }
	if (GetBit(PINC,2)==0) { return('6'); }
	if (GetBit(PINC,3)==0) { return('3'); }
	// ... *****FINISH*****
	// Check keys in col 3
	// Enable col 6 with 0, disable others with 1?s
	// The delay allows PORTC to stabilize before checking
	PORTC = 0xBF;
	asm("nop");
	if (GetBit(PINC,0)==0) { return('0'); }
	if (GetBit(PINC,1)==0) { return('8'); }
	if (GetBit(PINC,2)==0) { return('5'); }
	if (GetBit(PINC,3)==0) { return('2'); }
	// ... *****FINISH*****
	// Check keys in col 4
	// ... *****FINISH*****
	PORTC = 0x7F;
	asm("nop");
	if (GetBit(PINC,0)==0) { return('*'); }
	if (GetBit(PINC,1)==0) { return('7'); }
	if (GetBit(PINC,2)==0) { return('4'); }
	if (GetBit(PINC,3)==0) { return('1'); }
	return('\0'); // default value
}

//~~~~~~~~~~~~~~~~~~~~~~~~~TIMMER STUFF~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// TimerISR() sets this to 1. C programmer should clear to 0.
volatile unsigned char TimerFlag = 0;
// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks
void TimerOn() {
	// AVR timer/counter controller register TCCR1
	// bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: pre-scaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s
	TCCR1B = 0x0B;
	// AVR output compare register OCR1A.
	// Timer interrupt will be generated when TCNT1==OCR1A
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	OCR1A = 125;// AVR timer interrupt mask register
	// bit1: OCIE1A -- enables compare match interrupt
	TIMSK1 = 0x02;
	//Initialize avr counter
	TCNT1=0;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds
	_avr_timer_cntcurr = _avr_timer_M;
	//Enable global interrupts: 0x80: 1000000
	SREG |= 0x80;
}
void TimerOff() {
	// bit3bit1bit0=000: timer off
	TCCR1B = 0x00;
}
void TimerISR() {
	TimerFlag = 1;
}
// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1
	// (every 1 ms per TimerOn settings)
	// Count down to 0 rather than up to TOP (results in a more efficient comparison)
	_avr_timer_cntcurr--;
	if (_avr_timer_cntcurr == 0) {
		// Call the ISR that the user uses
		TimerISR();
		_avr_timer_cntcurr = _avr_timer_M;
	}
}
// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

void set_PWM(double frequency) {
	// Keeps track of the currently set frequency
	// Will only update the registers when the frequency
	// changes, plays music uninterrupted.
	static double current_frequency;
	if (frequency != current_frequency) {

		if (!frequency) TCCR3B &= 0x08; //stops timer/counter
		else TCCR3B |= 0x03; // resumes/continues timer/counter
		
		// prevents OCR3A from overflowing, using prescaler 64
		// 0.954 is smallest frequency that will not result in overflow
		if (frequency < 0.954) OCR3A = 0xFFFF;
		
		// prevents OCR3A from underflowing, using prescaler 64					// 31250 is largest frequency that will not result in underflow
		else if (frequency > 31250) OCR3A = 0x0000;
		
		// set OCR3A based on desired frequency
		else OCR3A = (short)(8000000 / (128 * frequency)) - 1;

		TCNT3 = 0; // resets counter
		current_frequency = frequency;
	}
}

void PWM_on() {
	TCCR3A = (1 << COM3A0);
	// COM3A0: Toggle PB6 on compare match between counter and OCR3A
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
	// WGM32: When counter (TCNT3) matches OCR3A, reset counter
	// CS31 & CS30: Set a prescaler of 64
	set_PWM(0);
}

void PWM_off() {
	TCCR3A = 0x00;
	TCCR3B = 0x00;
}

//~~~~~~~~~~~~~~~~~~~~~~~~END TIMER STUFF~~~~~~~~~~~~~~~~~~~~~~~~~~~



//----------------------------------------------------------------------------------------

char arr[50];
double notes[13] = {440, 466.16, 493.88, 523.25, 554.37, 587.33, 622.25, 698.46, 739.99, 783.99, 830.61, 880.00};
char p1LED = 0;
char p2LED = 0;
char recordflag = 0;
char compareflag = 0;
char playbackflag = 0;
char endchar = 'D';
char song1[2];
char song2[3];
char song3[4];
char song4[5];
char *thesong;
unsigned char songcount;

unsigned char p1score = 0;
unsigned char p2score = 0;
char winflag = 0;
char gameflag = 1;
char singleflag = 1;

char button1 = 0;
char button2 = 0;

char pinna;
char porttb;


double getfreq(char fig) {
	if(fig == '1') {
		return 220;
	} else if(fig == '2') {
		return 233.08;
	} else if(fig == '3') {
		return 246.94;
	} else if(fig == 'A') {
		return 261.63;
	} else if(fig == '4') {
		return 277.18;
	} else if(fig == '5') {
		return 293.66;
	} else if(fig == '6') {
		return 311.13;
	} else if(fig == 'B') {
		return 329.63;
	} else if(fig == '7') {
		return 349.23;
	} else if(fig == '8') {
		return 369.99;
	} else if(fig == '9') {
		return 392.00;
	} else if(fig == 'C') {
		return 415.30;
	} else if(fig == '*') {
		return 440.00;
	} else if(fig == '0') {
		return 466.16;
	} else if(fig == '#') {
		return 493.88;
	} else if(fig == 'D') {
		return -1.00;
	}
		
		
}

void play_song(char song[]){
	TimerSet(200);
	unsigned char s1 = 0;
	
	while(song[s1] != endchar){
		set_PWM(getfreq(song[s1]));
		while (!TimerFlag){}
		TimerFlag = 0;
		s1++;
	}
	TimerSet(200);
	return;
}


//--------Task scheduler data structure--------------------
// Struct for Tasks represent a running process in our
// simple real-time operating system.
/*Tasks should have members that include: state, period, a
measurement of elapsed time, and a function pointer.*/
typedef struct _task {
	//Task's current state, period, and the time elapsed
	// since the last tick
	signed char state;
	unsigned long int period;
	unsigned long int elapsedTime;
	//Task tick function
	int (*TickFct)(int);
} task;
task tasks[4];
const unsigned char numTasks=4;

enum rec_States { rec_init, rec_waitrecorder, rec_record, rec_waitRelease, rec_goCompare } rec_State;
int TickFct_record(int state) {
	/*VARIABLES MUST BE DECLARED STATIC*/
	/*e.g., static int x = 0;*/
	/*Define user variables for this state machine here. No functions; make them global.*/
	static char k = 0;
	switch(state) { // Transitions
		case -1:
			state = rec_init;
			break;
		case rec_init:
			if (!recordflag) {
				state = rec_init;
			} else if (recordflag) {
				state = rec_waitrecorder;
				k = 0;
			}
			
			break;
		case rec_waitrecorder:
			if (GetKeypadKey() != '\0') {
				state = rec_record;
				if(k > 0 && GetKeypadKey() == arr[k-1]) {
					state = rec_waitrecorder;
				}
			}
			else if (GetKeypadKey() == '\0') {
				state = rec_waitrecorder;
			}
			break;
		case rec_record:
			if (1) {
				state = rec_waitRelease;
			}
			break;
		case rec_waitRelease:
			if (GetKeypadKey() == '\0' && arr[k-1] != endchar && k < 50) {
				state = rec_waitrecorder;
			} else if (GetKeypadKey() == '\0' && (arr[k-1] == endchar || k > 49)) {
				state = rec_goCompare;
				p1LED = !p1LED;
				p2LED = !p2LED;
			} else state = rec_waitRelease;
			break;
		case rec_goCompare:
			state = rec_init;
			break;
		default:
		state = -1;
	} // Transitions

	switch(state) { // State actions
		case rec_init:
			k = 0;
			break;
		case rec_waitrecorder:
			//record light flash
			if(GetBit(porttb, 2)) {
				porttb = SetBit(porttb, 2, 0);
			} else porttb = SetBit(porttb, 2, 1);
			break;
		case rec_record:
			if(k > 0 || GetKeypadKey() != 'D') {
				arr[k] = GetKeypadKey();
				k++;
			}
		break;
		case rec_waitRelease:
			set_PWM(getfreq(GetKeypadKey()));
		break;
		case rec_goCompare:
			porttb = SetBit(porttb, 2, 0);
			set_PWM(0.00);
			k = 0;
			recordflag = 0;
			compareflag = 1;
			break;
		default: // ADD default behaviour below
			break;
	} // State actions
	rec_State = state;
	return state;
}


enum play_States { play_init, play_wait, play_playSong } play_State;
int TickFct_playBack(int state) {
	/*VARIABLES MUST BE DECLARED STATIC*/
	/*e.g., static int x = 0;*/
	/*Define user variables for this state machine here. No functions; make them global.*/
	switch(state) { // Transitions
		case -1:
			state = play_init;
			break;
		case play_init:
			if (1) {
				state = play_wait;
			}
			break;
		case play_wait:
			if (!playbackflag) {
				state = play_wait;
			}
			else if (songcount == 1) {
				state = play_playSong;
				thesong = song1;
			}
			else if (songcount == 2) {
				state = play_playSong;
				thesong = song2;
			}
			else if (songcount == 3) {
				state = play_playSong;
				thesong = song3;
			}
			else if (songcount == 4) {
				state = play_playSong;
				thesong = song4;
			}
			break;
		case play_playSong:
			state = play_wait;
			break;
		default:
		state = -1;
	} // Transitions

	switch(state) { // State actions
		case play_init:
			songcount = 0;
			break;
		case play_wait:
			break;
		case play_playSong:
			p1LED = 0;
			LCD_DisplayString(1, "   note");
			LCD_Cursor(1);
			LCD_WriteData(songcount + '0');
			play_song(thesong);
			//for(unsigned j = 0; j < songcount; j++) {
			//	set_PWM(getfreq(thesong[j]));
			//}
			songcount++;
			p1LED = 1;
			playbackflag = 0;
			compareflag = 1;
			break;
		default: // ADD default behaviour below
			break;
	} // State actions
	play_State = state;
	return state;
}

enum comp_States { comp_init, comp_waitPress, comp_waitRelease, comp_youWin, comp_youLose } comp_State;
int TickFct_compare(int state) {
	/*VARIABLES MUST BE DECLARED STATIC*/
	/*e.g., static int x = 0;*/
	/*Define user variables for this state machine here. No functions; make them global.*/
	static char j = 0;
	switch(state) { // Transitions
		case -1:
			state = comp_init;
			break;
		case comp_init:
			if (!compareflag) {
				state = comp_init;
			}
			else if (compareflag) {
				state = comp_waitPress;
				j = 0;
				winflag = 0;
			}
			break;
		case comp_waitPress:
			if (GetKeypadKey() == '\0') {
				state = comp_waitPress;
			}
			else if (GetKeypadKey() == thesong[j]) {
				state = comp_waitRelease;
				j++;
			}
			else if (GetKeypadKey() != thesong[j]) {
				state = comp_youLose;
			}
			break;
		case comp_waitRelease:
			if (thesong[j] == endchar || j > 49) {
				state = comp_youWin;
			}
			else if (GetKeypadKey() != '\0') {
				state = comp_waitRelease;
			}
			else if (GetKeypadKey() == '\0') {
				state = comp_waitPress;
			}
			break;
		case comp_youWin:
			if (1) {
				state = comp_init;
			}
			break;
		case comp_youLose:
			if (1) {
				state = comp_init;
			}
			break;
		default:
		state = -1;
	} // Transitions

	switch(state) { // State actions
		case comp_init:
			break;
		case comp_waitPress:
				//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
				/*
				LCD_Cursor(26);
				LCD_WriteData(j + '0');
				LCD_WriteData(thesong[j]);
				*/
			break;
		case comp_waitRelease:
			set_PWM(getfreq(GetKeypadKey()));
			break;
		case comp_youWin:
			set_PWM(0);
			winflag = 1;
			compareflag = 0;
			break;
		case comp_youLose:
			set_PWM(0);
			winflag = 0;
			compareflag = 0;
			break;
		default: // ADD default behaviour below
			break;
	} // State actions
	comp_State = state;
	return state;
}

enum menu_States { menu_init, menu_single, menu_multi, menu_waitcompare, menu_singlewin, menu_wait2compare, menu_singlelose, menu_multiwin, menu_waitfinish } menu_State;
int TickFct_menu(int state) {
	/*VARIABLES MUST BE DECLARED STATIC*/
	/*e.g., static int x = 0;*/
	/*Define user variables for this state machine here. No functions; make them global.*/
	static int count = 0;
	switch(state) { // Transitions
		case -1:
			state = menu_init;
			break;
		case menu_init:
			if (!(button1 || button2)) {
				state = menu_init;
			}
			else if (button1) {
				state = menu_single;
				singleflag = 1;
				winflag = 1;
				songcount = 0;
				p1LED = 1;
				p2LED = 0;
			}
			else if (button2) {
				state = menu_multi;
				singleflag = 0;
				winflag = 0;
				p1LED = 1;
				p2LED = 0;
				p1score = 0;
				p2score = 0;
				thesong = arr;
				arr[0] = '&';
			}
			break;
		case menu_single:
			if (!winflag) {
				state = menu_singlelose;
			}
			else if (songcount < 9 && winflag) {
				state = menu_waitcompare;
			}
			else if (songcount > 8) {
				state = menu_singlewin;
			}
			break;
		case menu_multi:
			if(!winflag && p1LED && p1score != p2score) {
				state = menu_multiwin;
				recordflag = 0;
			} else {
				state = menu_wait2compare;
				recordflag = 1;
			}
			/*
			if (p1score < 5 && p2score < 5) {
				state = menu_wait2compare;
			}
			else if (p1score > 4 || p2score > 4) {
				state = menu_multiwin;
			}
			*/
			break;
		case menu_waitcompare:
			if (compareflag || playbackflag) {
				state = menu_waitcompare;
			}
			else if (!compareflag) {
				state = menu_single;
			}
			break;
		case menu_singlewin:
			count = 0;
			state = menu_waitfinish;
			break;
		case menu_wait2compare:
			if (compareflag || recordflag) {
				state = menu_wait2compare;
			}
			else if (!compareflag) {
				state = menu_multi;
				if(winflag) {
					if(p1LED) {
						p1score++;
						} else if(p2LED) {
						p2score++;
					}
					p1LED = !p1LED;
					p2LED = !p2LED;
				}
			}
			break;
		case menu_singlelose:
			count = 0;
			state = menu_waitfinish;
			break;
		case menu_multiwin:
			state = menu_waitfinish;
			count = 0;
			break;
		case menu_waitfinish:
			if(count < 100) {
				count++;
			} else state = menu_init;
		
		default:
		state = -1;
	} // Transitions

	switch(state) { // State actions
		case menu_init:
			LCD_DisplayString(1, "Single    Multi");
			break;
		case menu_single:
			p1LED = 0;
			playbackflag = 1;
			LCD_DisplayString(1, "Score: ");
			LCD_Cursor(8);
			LCD_WriteData(songcount + '0');
			break;
		case menu_multi:
			LCD_DisplayString(1, "P1:      P2:     HiScore: ");
			LCD_Cursor(5);
			LCD_WriteData(p1score + '0');
			LCD_Cursor(14);
			LCD_WriteData(p2score + '0');
			LCD_Cursor(27);
			LCD_WriteData(highscoreram + '0');
			break;
		case menu_waitcompare:
			break;
		case menu_singlewin:
			LCD_DisplayString(1, "Congratulations you win!!");
			break;
		case menu_wait2compare:
		//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			/*
			LCD_Cursor(18);
			LCD_WriteData('R');
			LCD_WriteData(recordflag + '0');
			LCD_WriteData('C');
			LCD_WriteData(compareflag + '0');
			*/
			break;
		case menu_singlelose:
			LCD_DisplayString(1, "You lose, inferior.");
			break;
		case menu_multiwin:
			LCD_ClearScreen();
			LCD_DisplayString(1, "P   Wins!!!^");
			LCD_Cursor(2);
			if(p1score > p2score) {
				LCD_WriteData('1');
			}
			else {
				LCD_WriteData('2');
			}
			break;
		default: // ADD default behaviour below
			break;
	} // State actions
	menu_State = state;
	return state;
}



//-----------------------------------------------------------------------------------------------

unsigned long gcd = 100;

// --------END User defined FSMs---------------------------// Implement scheduler code from PES.
int main(void)
{
	unsigned char i=0;
	tasks[i].state = rec_init;
	tasks[i].period = 1;
	tasks[i].elapsedTime = 1;
	tasks[i].TickFct = &TickFct_record;
	++i;
	
	tasks[i].state = play_init;
	tasks[i].period = 1;
	tasks[i].elapsedTime = 1;
	tasks[i].TickFct = &TickFct_playBack;
	++i;
	
	tasks[i].state = comp_init;
	tasks[i].period = 1;
	tasks[i].elapsedTime = 1;
	tasks[i].TickFct = &TickFct_compare;
	++i;
	
	tasks[i].state = menu_init;
	tasks[i].period = 4;
	tasks[i].elapsedTime = 1;
	tasks[i].TickFct = &TickFct_menu;
	
	DDRA = 0x03; PORTA = 0xFC; // LCD and buttons (a2 a3)
	DDRB = 0xFF; PORTB = 0x00; // LED's
	DDRD = 0xFF; PORTD = 0x00; // LCD
	DDRC = 0xF0; PORTC = 0x0F; // keypad
	pinna = PINA;
	porttb = 0x00;
	PORTB = porttb;
	
	
	highscoreram = read_eeprom_word(&highscore);
	
	LCD_init();
	// Starting at position 1 on the LCD screen
	//LCD_DisplayString(1, "button menu section");
	
	PWM_on();
	TimerOn();
	
	TimerSet(100);
	TimerOn();
	/*
	LCD_Cursor(2);
	my_ram_array = read_eeprom_word(&my_eeprom_array);
	LCD_WriteData(my_ram_array);
	*/
	while(1)
	{
		for (i = 0; i < numTasks; i++ ) {
			// Task is ready to tick
			if ( tasks[i].elapsedTime == tasks[i].period ) {
				// Setting next state for task
				tasks[i].state = tasks[i].TickFct(tasks[i].state);
				// Reset elapsed time for next tick.
				tasks[i].elapsedTime = 0;
			}
			tasks[i].elapsedTime += 1;
		}
		
		porttb = SetBit(porttb, 0, p1LED);
		porttb = SetBit(porttb, 1, p2LED);
		
		PORTB = porttb;
		pinna = PINA;
		pinna = ~pinna;
		
		if(GetBit(pinna, 2)) {
			button1 = 1;
		} else button1 = 0;
		if(GetBit(pinna, 3)) {
			button2 = 1;
		} else button2 = 0;
		
		if(p1score > highscoreram) {
			write_eeprom_word(&highscore, p1score);
			
		}
		if(p2score > highscoreram) {
			write_eeprom_word(&highscore, p1score);
		}
		
		
		while(!TimerFlag);
		TimerFlag = 0;
	}
}
