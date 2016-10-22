/*
 *  The Azerbaijan Project
 *
 *  Created on: Apr 6, 2016
 *  Finished on: Apr 14, 2016
 *  Authors: Nicholas Yahr and James Ward
 */

/*  compiling and screen calling
 * 
 *  -lconio -lserial -lshell
 *  -> flash
 *  start screen with; screen /dev/ttyACM0 -9600
 */

// MSP430 includes
#include <libemb/shell/shell.h>
#include <libemb/serial/serial.h>
#include <libemb/conio/conio.h>
#include <msp430.h>

// C includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// Prototypes
void getInput(char string[SHELL_MAX_CMD_LINE]);
int help_cmd(shell_cmd_args *args);
int args_cmd(shell_cmd_args *args);
int setc_cmd(shell_cmd_args *args);
int exit_cmd(shell_cmd_args *args);

// Shell Command Definitions
shell_cmds commands = { .count = 4, .cmds = { { .cmd = "help", .desc =
		"Displays the commands available to the shell. Syntax: help", .func =
		&help_cmd }, { .cmd = "args", .desc =
		"Displays the arguments input to the console. Syntax: args [ARGUMENTS]",
		.func = &args_cmd }, { .cmd = "setc", .desc =
		"Changes the RGB settings for the LED. Syntax: setc [r/g/b, w, A rgb] [0-99]",
		.func = &setc_cmd }, { .cmd = "exit", .desc =
		"Exits the shell and closes the MSP430. Syntax: exit", .func =
		&exit_cmd } }
};

// Light variable Definitions
int PWM = 0;
int rgb[] = { 70, 70, 0 };

int selectDig = 0;
int selectRGB = 0;
int wait = 0;

//choose digit to display
const char digit[] = { BIT5, BIT6, BIT7 };

//choose number/letter
const char segNums[] = {
BIT0 + BIT1 + BIT2 + BIT3 + BIT4 + BIT5, //0    
		BIT1 + BIT2,				 //1
		BIT0 + BIT1 + BIT3 + BIT4 + BIT6,	 //2
		BIT0 + BIT1 + BIT2 + BIT3 + BIT6,	 //3
		BIT1 + BIT2 + BIT5 + BIT6,		 //4
		BIT0 + BIT2 + BIT3 + BIT5 + BIT6,	 //5
		BIT0 + BIT2 + BIT3 + BIT4 + BIT5 + BIT6, //6
		BIT0 + BIT1 + BIT2,			 //7
		BIT0 + BIT1 + BIT2 + BIT3 + BIT4 + BIT5 + BIT6,//8
		BIT0 + BIT1 + BIT2 + BIT3 + BIT5 + BIT6, //9
		BIT4 + BIT6,				 //r
		BIT0 + BIT2 + BIT3 + BIT4 + BIT5, 	 //G
		BIT2 + BIT3 + BIT4 + BIT5 + BIT6	 //b
};

// Main Loop
int main() {

	WDTCTL = WDTPW | WDTHOLD;	// Disable Watchdog
	BCSCTL1 = CALBC1_1MHZ;		// Run @ 1MHz
	DCOCTL = CALDCO_1MHZ;		// Run @ 1MHz
	serial_init(9600);		// Initialize serial connection & listen

	//Cathodes & light
	P1DIR = -1;
	P1OUT = 0;
	//Anodes
	P2DIR = -1;
	P2OUT = 0;
	P2SEL &= ~(BIT6 | BIT7);	//allow use of 2.6 (XIN) and 2.7 (XOUT)

	//Light
	TA0CCTL0 = CCIE;		//CC Interrupt Enable
	TA0CCR0 = 100;			//10,000 interrupts / sec !?
	TA0CTL = TASSEL_2 | MC_1 | ID_0;

	//Display
	TA1CCTL0 = CCIE;		//CC Interrupt Enable
	TA1CCR0 = 500;			//250 interrups / sec
	TA1CTL = TASSEL_2 | MC_1 | ID_3;

	_BIS_SR(GIE);

	while (1) {
		char string[SHELL_MAX_CMD_LINE];// String space for command-line input
		cio_printf("\n\r$ ");					// Print shell prompt
		getInput(string);						// Get command-line input
		shell_process_cmds(&commands, string);	// Process/Run command
	}
	return 0;
}

// Functions
void getInput(char string[SHELL_MAX_CMD_LINE]) {
	/*
	 * Get the user's input from the command-line
	 */
	memset(string, '\0', SHELL_MAX_CMD_LINE); // Clear out any other data from the string
	int i = 0; // Loop counter
	char c = ' '; // Holds next character from the command-line
	while (i < SHELL_MAX_CMD_LINE && c != '\r') { // Runs until the end of the string is reached, or the user presses "enter"
		c = cio_getc(); // Get the command line character
		if (isalnum(c) | isspace(c) | iscntrl(c)) {
			if (c == 127) { // Check for backspace
				if (i != 0) {
					i--;	// Move back a character
					cio_printc('\b');
					cio_printc(' ');
					cio_printc('\b');
				}
				string[i] = '\0';// Replace the character in the array with a null character
			} else if (c != '\r') { // Check for "enter" press
				string[i] = c; // Put character in command-line string
				cio_printc(c); // Print out the character so the user knows it was accepted
				i++;	// Move to next position
			} else {
				cio_printf("\n\r"); // Print the newline so the user knows it was understood (does not need to be stored in the command-line string)
			}
		}
	}
}

int help_cmd(shell_cmd_args *args) {
	/*
	 * Displays a help screen to the terminal with the information for all accepted commands.
	 */
	cio_printf(
			"\n\r--------------------------------------------------------\n\r\t\t\tHelp\n\r--------------------------------------------------------\n\r");
	int i;
	for (i = 0; i < commands.count; i++) {
		cio_printf("%s: %s\n\r", commands.cmds[i].cmd, commands.cmds[i].desc);
	}
	cio_printf("--------------------------------------------------------\n\r");

	return SHELL_PROCESS_OK;
}

int args_cmd(shell_cmd_args *args) {
	cio_printf(
			"\n\r--------------------------------------------------------\n\r\t\t\tPrint Arguments\n\r--------------------------------------------------------\n\r");
	int i;
	for (i = 0; i < args->count; i++) {
		cio_printf("Argument: %s\n\r", (i, args->args[i].val));
	}
	cio_printf("--------------------------------------------------------\n\r");
	return SHELL_PROCESS_OK;
}

int setc_cmd(shell_cmd_args *args) {
	cio_printf(
			"\n\r--------------------------------------------------------\n\r\t\t\tSet Color\n\r--------------------------------------------------------\n\r");
	if (args->count < 2) {
		cio_printf(
				"Not enough arguments\n\r--------------------------------------------------------\n\r");
		return SHELL_PROCESS_ERR_ARGS_LEN;
	} else if (args->count > 4) {
		cio_printf(
				"Too many arguments\n\r--------------------------------------------------------\n\r");
		return SHELL_PROCESS_ERR_ARGS_LEN;
	}
	int val = atoi(args->args[1].val);
	int val2;
	int val3;
	if (val >= 0 && val < 100) {
		switch (args->args[0].val[0]) {
		case 'r':
			//cio_printf("%s", args->args[1]);
			rgb[0] = val;
			cio_printf("Changing r to: %i\n\r--------------------------------------------------------\n\r", val);
			return SHELL_PROCESS_OK;
			break;
		case 'g':
			rgb[1] = val;
			cio_printf("Changing g to: %i\n\r--------------------------------------------------------\n\r", val);
			return SHELL_PROCESS_OK;
			break;
		case 'b':
			rgb[2] = val;
			cio_printf("Changing b to: %i\n\r--------------------------------------------------------\n\r",	val);
			return SHELL_PROCESS_OK;
			break;
		case 'w':
			rgb[0] = val;
			rgb[1] = val;
			rgb[2] = val;
			cio_printf("Changing to white value: %i\n\r--------------------------------------------------------\n\r", val);	
			return SHELL_PROCESS_OK;
			break;
		case 'A':
			val2 = atoi(args->args[2].val);
			val3 = atoi(args->args[3].val);
			if (val2 >= 0 && val2 < 100 && val3 >= 0 && val3 < 100){//make sure they are in range
				rgb[0] = val;
				rgb[1] = val2;
				rgb[2] = val3;
				cio_printf(
					"Changing to r%i g%i b%i\n\r--------------------------------------------------------\n\r", 							val, val2, val3);	
				return SHELL_PROCESS_OK;
			}else
				cio_printf("Input Error:\n\r--------------------------------------------------------\n\r");
			break ;
		return SHELL_PROCESS_OK;
		break;
		default:
			cio_printf("Unknown color: %c\n\r--------------------------------------------------------\n\r",
					args->args[0].val[0]);
			return 1;
		}
	} else {
		cio_printf("Unusable size: %i\n\r--------------------------------------------------------\n\r",	val);
		return 1;
	}
}

int exit_cmd(shell_cmd_args *args) {
	cio_printf("Exiting...\n\r");
	exit(0);
}

#pragma vector=TIMER0_A0_VECTOR//light
__interrupt void TIMER0_A0_ISR(void) {

	//resets light
	if (PWM == 0) {
		P1OUT |= BIT0 | BIT3 | BIT4;
	}

	//turns off rgb values at set times
	if (PWM == rgb[0]) {
		P1OUT &= ~BIT0;
	}
	if (PWM == rgb[1]) {
		P1OUT &= ~BIT3;
	}
	if (PWM == rgb[2]) {
		P1OUT &= ~BIT4;
	}

	//increment and reset PWM
	PWM++;
	if (PWM == 100) {
		PWM = 0;
	}
}

#pragma vector=TIMER1_A0_VECTOR//display
__interrupt void TIMER1_A0_ISR(void) {

	//Display
	P1OUT |= BIT5 | BIT6 | BIT7; //preps from light
	P1OUT &= ~digit[selectDig]; //sets active digit

	switch (selectDig) { //displays num/letter at proper digit

	case (0):
		P2OUT = segNums[10 + selectRGB]; //r
		break;
	case (1):
		P2OUT = segNums[rgb[selectRGB] / 10]; //2
		break;
	case (2):
		P2OUT = segNums[rgb[selectRGB] % 10]; //3
		break;
	}

	if (selectDig < 2) //loops through all digits
		selectDig++;
	else
		selectDig = 0;

	if (wait == 500) { //2 seconds wait for each rgb display value
		wait = 0;
		if (selectRGB == 2)
			selectRGB = -1;
		selectRGB++;
	} else
		wait++;
}
