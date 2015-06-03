#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "./path/io.c"
#include "./path/io.h"

//-------------------------------------------------------------------------------------
// macro for easier usage
#define read_eeprom_word(address) eeprom_read_word ((const uint16_t*)address)
#define write_eeprom_word(address,value) eeprom_write_word ((uint16_t*)address,(uint16_t)value)
#define update_eeprom_word(address,value) eeprom_update_word ((uint16_t*)address,(uint16_t)value)

//declare an eeprom array
unsigned char EEMEM  my_eeprom_array;

// declare a ram array
unsigned char my_ram_array;



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

void LCD_WriteCommand (unsigned char Command) {
	CLR_BIT(CONTROL_BUS,RS);
	DATA_BUS = Command;
	SET_BIT(CONTROL_BUS,E);
	asm("nop");
	CLR_BIT(CONTROL_BUS,E);
	delay_ms(2); // ClearScreen requires 1.52ms to execute
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





//----------------------------------------------------------------------------------------

char arr[50];
double notes[13] = {440, 466.16, 493.88, 523.25, 554.37, 587.33, 622.25, 698.46, 739.99, 783.99, 830.61, 880.00};
char p1LED = 0;
char p2LED = 0;
char recordflag = 0;
char compareflag = 0;
char endchar = 'D';
double song1[1];
double song2[2];
double song3[3];
double song4[4];
double *thesong;
unsigned char songcount;

unsigned char p1score = 0;
unsigned char p2score = 0;
char winflag = 0;
char gameflag = 1;
char singleflag = 1;


double getfreq(char fig) {
	if(fig == '1') {
		return 440;
	} else if(fig == '2') {
		return 466.16;
	} else if(fig == '3') {
		return 493.88;
	} else if(fig == 'A') {
		return 523.25;
	} else if(fig == '4') {
		return 554.37;
	} else if(fig == '5') {
		return 587.33;
	} else if(fig == '6') {
		return 622.25;
	} else if(fig == 'B') {
		return 659.25;
	} else if(fig == '7') {
		return 698.46;
	} else if(fig == '8') {
		return 739.99;
	} else if(fig == '9') {
		return 783.99;
	} else if(fig == 'C') {
		return 830.61;
	} else if(fig == '*') {
		return 880.00;
	} else if(fig == '0') {
		return 932.33;
	} else if(fig == '#') {
		return 987.77;
	} else if(fig == 'D') {
		return -1.00;
	}
		
		
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
	static char i = 0;

	static char i = 0;
	switch(state) { // Transitions
		case -1:
			state = rec_init;
			break;
		case rec_init:
			if (!recordflag) {
				state = rec_init;
				} else if (recordflag) {
				state = rec_waitrecorder;
				p1LED = !p1LED;
				p2LED = !p2LED;
			}
			break;
		case rec_waitrecorder:
			if (GetKeypadKey() != '\0') {
				state = rec_record;
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
			if (GetKeypadKey() == '\0' && arr[i-1] != endchar && i < 50) {
				state = rec_waitrecorder;
				} else if (GetKeypadKey() == '\0' && (arr[i-1] == endchar || i > 49)) {
				state = rec_goCompare;
			} else state = rec_waitRelease;
			break;
		case rec_goCompare:
			break;
		default:
		state = -1;
	} // Transitions

	switch(state) { // State actions
		case rec_init:
			i = 0;
			break;
		case rec_waitrecorder:
			break;
		case rec_record:
			arr[i] = GetKeypadKey();
			i++;
		break;
		case rec_waitRelease:
			setPWM(getfreq(GetKeypadKey()));
		break;
		case rec_goCompare:
			recordflag = 0;
			compareflag = 1;
			p1LED = !p1LED;
			p2LED = !p2LED;
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
			LCD_DisplayString(0, "   note");
			LCD_Cursor(0);
			LCD_WriteData(songcount);
			//TimerSet(500);
			for(unsigned j = 0; j < songcount; j++) {
				setPWM(getfreq(thesong[j]));
			}
			songcount++;
			//SET TIMER BACK TO THE ORIGINAL
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
				i = 0;
				winflag = 0;
			}
			break;
		case comp_waitPress:
			if (GetKeypadKey() == '\0';) {
				state = comp_waitPress;
			}
			else if (GetKeypadKey() == arr[i];) {
				state = comp_waitRelease;
				i++;
			}
			else if (GetKeypadKey() != arr[i];) {
				state = comp_youLose;
			}
			break;
		case comp_waitRelease:
			if (arr[i] == endchar || i > 49) {
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
			break;
		case comp_waitRelease:
			break;
		case comp_youWin:
			winflag = 1;
			compareflag = 0;
			break;
		case comp_youLose:
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
			if (!gameflag) {
				state = menu_init;
			}
			else if (//BUTTON PRESS) {
				state = menu_single;
				singleflag = 1;
				winflag = 1;
				songcount = 0;
				p2LED = 0;
			}
			else if (//BUTTON PRESS) {
				state = menu_multi;
				singleflag = 0;
				winflag = 0;
				p1LED = 0;
				p2LED = 1;
				thesong = arr;
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
			if (p1score < 5 && p2score < 5) {
				state = menu_wait2compare;
			}
			else if (p1score > 4 || p2score > 4) {
				state = menu_multiwin;
			}
			break;
		case menu_waitcompare:
			if (compareflag) {
				state = menu_waitcompare;
			}
			else if (!compareflag) {
				state = menu_single;
			}
			break;
		case menu_singlewin:
			count = 0;
			state = menu_waitfinsh;
			break;
		case menu_wait2compare:
			if (compareflag) {
				state = menu_wait2compare;
			}
			else if (!compareflag) {
				state = menu_multi;
			}
			break;
		case menu_singlelose:
			count = 0;
			state = menu_waitfinsh;
			break;
		case menu_multiwin:
			count = 0;
			state = menu_waitfinsh;
			break;
		case menu_waitfinish:
			if(count < 50) {
				count++;
			} else state = menu_init;
		
		default:
		state = -1;
	} // Transitions

	switch(state) { // State actions
		case menu_init:
			LCD_DisplayString(0, "Single      Multi");
			break;
		case menu_single:
			p1LED = 0;
			playbackflag = 1;
			LCD_DisplayString(0, "Score: ");
			LCD_Cursor(7);
			LCD_WriteData(songcount + '0');
			break;
		case menu_multi:
			recordflag = 1;
			LCD_DisplayString(0, "P1:        P2:  ");
			LCD_Cursor(4);
			LCD_WriteData(p1score + '0');
			LCD_Cursor(16);
			LCD_WriteData(p2score + '0');
		
			if(winflag) {
				if(p1LED) {
					p1score++;
					} else if(p2LED) {
					p2score++;
				}
			}
			break;
		case menu_waitcompare:
			break;
		case menu_singlewin:
			LCD_ClearScren();
			LCD_DisplayString("Congratulations you win!!");
			break;
		case menu_wait2compare:
			break;
		case menu_singlelose:
			LCD_ClearScren();
			LCD_DisplayString("You lose, inferior.");
			break;
		case menu_multiwin:
			LCD_ClearScreen();
			LCD_DisplayString("P   Wins!!!);
			LCD_Cursor(1);
			if(p1score > 4) {
				LCD_WriteData("1");
			}
			else if(p2score > 4) {
				LCD_WriteData("2");
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
	
	DDRA = 0x00; PORTA = 0xFF; // buttons
	DDRB = 0xFF; PORTB = 0x00; // LED's
	DDRD = 0xFF; PORTD = 0x00; // LCD
	DDRC = 0xF0; PORTC = 0x0F; // keypad
	LCD_init();
	// Starting at position 1 on the LCD screen
	//LCD_DisplayString(1, "button menu section");
	
	PWM_on();
	TimerOn();
	
	TimerSet(50);
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
		
		while(!TimerFlag);
		TimerFlag = 0;
	}
}
