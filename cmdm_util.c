#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#if 0
// send a string, return 0 if sending, 1 if done
static int sendstring(char *sentence);
// receive a CR/LF terminated string; return 0 if working, 1 if done, truncate at len
static int recvstring(char *sentence,unsigned char len);


static int sendstring(char *sentence)
{
register char c;

	while((c=sentence[string_pos]) != '\0'){	// pump while there are chars
		if(CMDM_TX(c) == -1)			// and room to put them
			return 0;			// wait till room
		++string_pos;
	}
	return 1;
}
static int sendstring(char **pptr)
{
char c, *ptr = *pptr;

	while((c=*ptr) != '\0'){	// pump while there are chars
		if(cmdm.func->chat.talk(c) == -1)			// and room to put them
			return 0;			// wait till room
		++ptr;
	}
	*pptr = ptr;
	return 1;
}


/// external interrupt on INT1 -> RI falling edge
void int1_isr(void)
{
	RIactivity=1;
}


/// external interrupt on INT0 -> CMDM_LED pulse	
/** Each LED pulse is an interrupt, and we reset timers->led to CMDMLED_TIMEOUT,
so the number of ticks elapsed between pulses is CMDMLED_TIMEOUT-CMDMLED_TIMER
*/
void int0_isr(void)
{
	if(*(timers->led) < (CMDMLED_TIMEOUT-55))			// > 55 ticks, too long
		CMDMLEDstate = CMDMLED_NONET;
	else if(*(timers->led) < (CMDMLED_TIMEOUT-25))		// > 25 ticks => registered
		CMDMLEDstate = CMDMLED_NETWORK;
	else if(*(timers->led) < (CMDMLED_TIMEOUT-12))		// > 13 ticks => PDP
		CMDMLEDstate = CMDMLED_PDP;
	else if(*(timers->led) < (CMDMLED_TIMEOUT-7))		// > 7 ticks => no network
		CMDMLEDstate = CMDMLED_NONET;
	else if((CMDMLEDstate == CMDMLED_NETWORK) &&
		(*(timers->led) < (CMDMLED_TIMEOUT-2))) 		// > 2 ticks => GPRS (must be in NETWORK before)
		CMDMLEDstate = CMDMLED_TCPIP;			// ignore less than 2 ticks (no change)
	*(timers->led) = CMDMLED_TIMEOUT;				// restart timer
}

#endif

int recvstring(char *sentence,unsigned char len)
{
register char input_char;

	while((input_char=CMDM_RX()) != -1){  // process chars in serial input
		if(input_char == 0x0A){
			sentence[string_pos] = 0; //add null
			return 1;
		} else if(input_char > 0){
			sentence[string_pos++] = input_char;
			if(string_pos == len)
				--string_pos;	// truncate string if too large
		}
	}
	return 0;
}

