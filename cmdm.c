#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "sys_timers.h"

#include "cmdmopts.h"
#ifndef CHAT_FULLCHAT
	#error "This library uses full chat, please define CHAT_FULLCHAT"
#endif

#include "cmdm.h"
char CMDM_LINEBUFFER[CMDM_LINEBFR_SIZE]; ///< internal buffer to build strings for chatting and get parseable responses
#include "cmdm_hwmacros.h"
#include "cmdm_chatstrings.h"

typedef struct {
	unsigned char state;		///< handler state
	unsigned char next_state;	///< next state for shared wait loops and stuff
} state_t;

typedef struct {
	const cmdmfunctions_t *func;	///< pointer to I/O functions
	cmdm_timers_t *timers;		///< pointer to timers
	unsigned char state;		///< handler state
	unsigned char next_state;	///< next state for shared wait loops and button presses
	unsigned char ledstate;		///< guess connection state by measuring LED blinking rate
	state_t cmdsubstate;		///< status handler substates
	state_t regsubstate;		///< registered handler substates
	fullchat_t chat;
	char PIN[4];			///< SIMcard PIN
	unsigned char lbpos;		///< offset inside linebfr
#ifdef CMDM_INCOMING
	unsigned char RIactivity;	
	char who[CMDM_WHO_SIZE+1];	///< who's calling/sending SMS
#endif
} cmdm_t;

static cmdm_t cmdm;

void cmdm_setpin(char *pin)
{
	memcpy(&cmdm,pin,4);
}

// network LED indication states
enum CMDMLEDstates {
	CMDMLED_NONE=0,
	CMDMLED_NONET,
	CMDMLED_NETWORK
};

// CMDM handler states
enum CMDMstates {
	CMDM_START=0,			///< clear buffers, poweron
	CMDM_BUTTPOWON,			///< press button
	CMDM_BUTTOFF,			///< release button
	CMDM_CHECKON,			///< wait for signs of life
	CMDM_ECHOOFF,			///< remove local echo
	CMDM_WAITSIM,			///< wait if SIM is busy starting
	CMDM_ASKPIN,			///< check if PIN configured
	CMDM_CHECKPIN,			///< get and parse response
	CMDM_SENDPIN,			///< send PIN if needed
	CMDM_CONFIGURE,			///< send more AT commands
	CMDM_REGISTERING,		///< wait for network registration
	CMDM_REGISTERED,		///< module is registerd

	// next_state is here used as a superstate to share stop and reset code
	CMDM_STOP,			///< stop module, press button
	CMDM_BUTTPOWOFF,		///< release button
	CMDM_UNREGISTERING,		///< wait for network logoff
	CMDM_POWEROFF,			///< remove power
	CMDM_OFF,			///< stay off

	CMDM_RESET,			///< stop module, then start again
	CMDM_BUTTPOWOFFRESET,		///< release button
	CMDM_UNREGRESET,		///< wait for network logoff
	CMDM_POWEROFFRESET,		///< remove power, and go to start

	CMDM_CHATTING,			///< chat (single dialog)
	CMDM_FCHATTING,			///< chat (multiple dialogs)
	CMDM_SHORTWAIT			///< guardtimes and button presses
};

static int cmd_handler(void);
static void cmd_init(void);
static int registered_handler(void);
static void registered_init(void);

void cmdm_handler(void)
{
int ret = CMDM_DONE;
cmdm_t *s = &cmdm;
cmdm_timers_t *timers = s->timers;;

	s = &cmdm;
	timers = s->timers;
	switch(s->state){
	case CMDM_START:
		if(*(timers->powbut) != 0)			// wait for reset guardtime
			break;
		s->func->init();				// flush buffers, init pointers, includes flushrx
		CMDM_PSCTL_ON;					// turn power on
		CMDM_DTR_ON;					// raise DTR
		*(timers->powbut) = CMDM_POWERON_GUARDTIME;	// and wait for a while
		s->next_state = CMDM_BUTTPOWON;
		s->state = CMDM_SHORTWAIT;
		break;
	case CMDM_REGISTERING:
		ret = cmd_handler();					// handle command chats
		if((*(timers->network) == 0)				// wait for a while
			|| (ret == CMDM_ERROR)				// handle cmd chat timeouts
			|| (*(timers->led) == 0)){			// check LED pulse presence
			s->state = CMDM_RESET;
		} else if(s->ledstate >= CMDMLED_NETWORK) {		// check registration
			s->state = CMDM_REGISTERED;			// the net is with us
			s->next_state = CMDM_REGISTERED;
			registered_init();				// init children handler
		}
		break;
	case CMDM_REGISTERED:
		if((ret=cmd_handler()) == CMDM_DONE)			// handle command chats
			ret = registered_handler();			// handle children (if not already chatting)
		if((*(timers->led) == 0)				// check LED pulse presence
			|| (ret == CMDM_ERROR)){			// handle children reported errors (including cmd chat timeouts)
			s->state = CMDM_RESET;
		} else if(s->ledstate < CMDMLED_NETWORK) {		// check registration
			*(timers->network) = CMDM_REGISTER_TIME; 	// start network registration again
			s->state = CMDM_REGISTERING;
		}
		break;

	case CMDM_RESET:
		s->next_state = CMDM_RESET;
		/*FALLTHROUGH*/
	case CMDM_STOP:
		CMDM_PWRKEY_ON;					// press the button
		*(timers->powbut) = CMDM_POWERKEY_PRESSOFF;	// for a while
		if(s->next_state == CMDM_RESET)
			s->next_state = CMDM_BUTTPOWOFFRESET;
		else
			s->next_state = CMDM_BUTTPOWOFF;
		s->state = CMDM_SHORTWAIT;
		break;
	case CMDM_BUTTPOWOFFRESET:
		s->next_state = CMDM_RESET;
		/*FALLTHROUGH*/
	case CMDM_BUTTPOWOFF:
		CMDM_PWRKEY_OFF;				// release the button
		if(s->next_state == CMDM_RESET)
			s->state = CMDM_UNREGRESET;
		else
			s->state = CMDM_UNREGISTERING;
		*(timers->network) = CMDM_UNREGISTER_TIME;	// then wait for network logoff
		break;
	case CMDM_UNREGRESET:
		/*FALLTHROUGH*/
	case CMDM_UNREGISTERING:
		if((*(timers->network) == 0)				// wait for a while
			|| (*(timers->led) == 0)){			// or LED pulse absence
			if(s->next_state == CMDM_RESET)
				s->next_state = CMDM_POWEROFFRESET;
			else
				s->next_state = CMDM_POWEROFF;
			*(timers->powbut) = CMDM_POWEROFF_GUARDTIME;	// wait a bit before removing power
			s->state = CMDM_SHORTWAIT;
		}
		break;
	case CMDM_POWEROFFRESET:
		s->next_state = CMDM_RESET;
		/*FALLTHROUGH*/
	case CMDM_POWEROFF:
		CMDM_LED_DISABLEINTS;				// disable LED ints
		CMDM_RI_DISABLEINTS;				// disable RI ints
		s->ledstate = CMDMLED_NONE;			// stop state
		if(s->next_state == CMDM_RESET)
			s->next_state = CMDM_START;		// power on again
		else
			s->next_state = CMDM_OFF;		// stay off
		*(timers->powbut) = CMDM_RESTART_GUARDTIME;	// after power off, wait a bit before doing anything
		s->state = CMDM_SHORTWAIT;
		/*FALLTHROUGH*/
	case CMDM_OFF:
		CMDM_PWRKEY_OFF;
		CMDM_PSCTL_OFF;
		/** \warning: DTE modem signals should (must) be also put to 0V to avoid "powering" the module from them
		TD should be put low, although this can be complicated for a macro ? Function call ? */
		CMDM_DTR_ON;					// ensure \DTR is on (0V) when powered off
		break;

	case CMDM_BUTTPOWON:
		*(timers->led) = CMDMLED_TIMEOUT;		// restart LED timer
		s->ledstate = CMDMLED_NONE;			// init state
		CMDM_LED_CLRINTFLG;
		CMDM_LED_INTONFALLING;
		CMDM_LED_ENABLEINTS;				// enable LED ints
		CMDM_PWRKEY_ON;					// press the button
		*(timers->powbut) = CMDM_POWERKEY_PRESSON;	// for a while
		s->next_state = CMDM_BUTTOFF;			// then release button and check for life signs
		s->state = CMDM_SHORTWAIT;
		break;
	case CMDM_BUTTOFF:
		CMDM_PWRKEY_OFF;				// release the button
		s->state = CMDM_CHECKON;
		*(timers->powbut) = CMDM_ALIVE_TIMEOUT;		// then wait for signs of life
		/*FALLTHROUGH*/
	case CMDM_CHECKON:
		if(s->ledstate != CMDMLED_NONE){		// wait for a pulse, timeout => no pulse
			s->next_state = CMDM_ECHOOFF;		// if we get power on confirmation 
			*(timers->powbut) = CMDM_BUTTON_GUARDTIME;	// then wait and configure module
			s->state = CMDM_SHORTWAIT;
			s->ledstate = CMDMLED_NONE;
			break;
		}
		if(*(timers->powbut) == 0)			// if timeout, reset module
			s->state = CMDM_POWEROFFRESET;		// oops, reset module (without pressing power button, just remove power)
		break;
	case CMDM_ECHOOFF:
		s->next_state = CMDM_WAITSIM;
		s->state = CMDM_CHATTING;
		chat_start(&s->chat.chat,(char *)CMDM_STR_ECHOOFF, (char *)CMDM_STR_OK, CMDM_CHAT_TIME);		// use single dialog chat with no expect as a "send string and start timeout"
		break;
	case CMDM_WAITSIM:
		*(timers->powbut) = CMDM_ASKSIM_GUARDTIME;	// wait an extra time for the SIM to stop being busy, apparently responding too fast
		s->next_state = CMDM_ASKPIN;
		s->state = CMDM_SHORTWAIT;
		break;
	case CMDM_ASKPIN:
		s->next_state = CMDM_CHECKPIN;
		s->state = CMDM_CHATTING;
		chat_start(&s->chat.chat,(char *)CMDM_STR_CHKPIN, "", CMDM_CHAT_TIME);	// use single dialog chat with no expect as a "send string and start timeout"
		cmdm_data_recvstr_start();	// not using LINEBUFFER, can clean
		break;
	case CMDM_CHECKPIN:					// response is two lines: PIN status and OK
		if(cmdm_data_recvstr()){			// got something
			if(strstr(CMDM_LINEBUFFER, (char *)CMDM_STR_NOPIN)){
				s->next_state = CMDM_CONFIGURE;	// no PIN needed
			} else if(strstr(CMDM_LINEBUFFER, (char *)CMDM_STR_HASPIN)){
				s->next_state = CMDM_SENDPIN;	// PIN needed
			} else {
				s->state = CMDM_RESET;		// unknown response, reset module
				break;
			}
			s->state = CMDM_CHATTING;		// now retrieve the OK at the end
			chat_start(&s->chat.chat, "", (char *)CMDM_STR_OK, CMDM_CHAT_TIME);			
		} else if(*(timers->chat) == 0){		// this timer has been initialized by chat
			s->state = CMDM_RESET;			// no response, reset module
		}
		break;
	case CMDM_SENDPIN:
		s->next_state = CMDM_CONFIGURE;
		s->state = CMDM_CHATTING;
		sprintf(CMDM_LINEBUFFER,"%s%c%c%c%c\r", CMDM_STR_SETPIN,
			s->PIN[0], s->PIN[1], s->PIN[2], s->PIN[3]);
		chat_start(&s->chat.chat, CMDM_LINEBUFFER, (char *)CMDM_STR_OK, CMDM_CHAT_TIME);		// use single dialog chat
		break;
	case CMDM_CHATTING:
		if((ret=chat_tick(&s->chat.chat)) == CHAT_DONE)
			s->state = s->next_state;		// handle next step
		else if(ret == CHAT_ERROR)
			s->state = CMDM_RESET;			// timeout/oops, reset module
		// else // still chatting
		break;
	case CMDM_CONFIGURE:
		chat_fullstart(&s->chat,(sendexpect_t *)&CMDM_SX_CONFIG, CMDM_CHAT_TIME); // use multiple dialog chat
		*(timers->network) = CMDM_REGISTER_TIME;	// then wait for network registration
		s->state = CMDM_FCHATTING;
		s->next_state = CMDM_REGISTERING;
		cmd_init();					// initialize command chatter to a fresh start
		/*FALLTHROUGH*/
	case CMDM_FCHATTING:
		if((ret=chat_fulltick(&s->chat)) == CHAT_DONE)
			s->state = s->next_state;		// handle next step
		else if(ret == CHAT_ERROR)
			s->state = CMDM_RESET;			// timeout/oops, reset module
		// else // still chatting
		break;
	case CMDM_SHORTWAIT:
		if(*(timers->powbut) == 0)			// wait for a short while
			s->state = s->next_state;		// handle next step
		break;
	default:
		s->state = CMDM_RESET;				// reset module
		break;
	}
}


/* command-response handler states, send a command and wait for a response using either chat or recvstr.
This provides a minimum of simplification to other handlers and user commands, which will themselves handle
any error condition arising from this chat, as un unexpected response or a timeout.
The handler will just return "busy-idle" to the main handler */
enum CMDMcmdstates {
	CMDM_CMD_IDLE,			///< idle waiting 
	CMDM_CMD_ERROR,			///< last command ended in error, now idle waiting
	CMDM_CMD_SNDSTR,		///< sending command
	CMDM_CMD_RCVSTR,		///< getting response in line buffer
	CMDM_CMD_CHATTING,		///< chat (single dialog)
	CMDM_CMD_FCHATTING		///< chat (multiple dialogs)
};

static int cmd_handler()
{
int ret;
state_t *s = &cmdm.cmdsubstate;
fullchat_t *chat = &cmdm.chat;

	switch(s->state){
	case CMDM_CMD_ERROR:
		/*FALLTHROUGH*/
	case CMDM_CMD_IDLE:
		CMDM_DTR_ON;					// ensure DTR is on while exchanging commands
		return CMDM_DONE;
	case CMDM_CMD_RCVSTR:
		if(*(chat->chat.timer) == 0)			// this timer has been initialized by chat
			s->state = CMDM_CMD_ERROR;		// no response, signal error condition
		else if(cmdm_data_recvstr())			// got end of line
			s->state = CMDM_CMD_IDLE;
		break;
	case CMDM_CMD_SNDSTR:
	case CMDM_CMD_CHATTING:
		if((ret=chat_tick(&chat->chat)) == CHAT_DONE){	// chat finished
			if(s->state == CMDM_CMD_SNDSTR){	// are we running cmdresp ?
				cmdm_data_recvstr_start();	// then point to start of clean buffer
				s->state = CMDM_CMD_RCVSTR;
			} else {
				s->state = CMDM_CMD_IDLE;
			}
		} else if(ret == CHAT_ERROR)
			s->state = CMDM_CMD_ERROR;		// no response, signal error condition
		break;
	case CMDM_CMD_FCHATTING:
		if((ret=chat_fulltick(chat)) == CHAT_DONE)
			s->state = CMDM_CMD_IDLE;		// chat finished
		else if(ret == CHAT_ERROR)
			s->state = CMDM_CMD_ERROR;		// no response, signal error condition
		break;
	default:
		return CMDM_ERROR;
	}
	return CMDM_AGAIN;
}

void cmdm_cmd_start(char *send, char *expect, unsigned int timeout)
{
//	if((cmdm.cmdsubstate.state != CMDM_CMD_IDLE) && (cmdm.cmdsubstate.state != CMDM_CMD_ERROR))
//		return CMDM_ERROR;
	chat_start(&cmdm.chat.chat, send, expect, timeout);
	cmdm.cmdsubstate.state = CMDM_CMD_CHATTING;
}
void cmdm_cmdresp_start(char *send, unsigned int timeout)
{
	chat_start(&cmdm.chat.chat, send, "", timeout);		// use single dialog chat with empty expect as "send string and start timeout"
	cmdm.cmdsubstate.state = CMDM_CMD_SNDSTR;		// send cmd (chat will handle empty strings)
}
void cmdm_mcmd_start(sendexpect_t *strings, unsigned int timeout)
{
	chat_fullstart(&cmdm.chat, strings, timeout); 		// use multiple dialog chat
	cmdm.cmdsubstate.state = CMDM_CMD_FCHATTING;
}
int cmdm_cmd_status(void)
{
	if(cmdm.cmdsubstate.state == CMDM_CMD_IDLE)
		return CMDM_DONE;
	if(cmdm.cmdsubstate.state == CMDM_CMD_ERROR)
		return CMDM_ERROR;
	return CMDM_AGAIN;
}

// command handler, initialization
static void cmd_init()
{
	cmdm.cmdsubstate.state = CMDM_CMD_IDLE;
}

	
// registered handler substates, children handling
enum CMDMregsubstates {
	CMDM_REG_INIT			///< 
};

static int registered_handler()
{
int ret;
//state_t *s = &cmdm.regsubstate;

#ifdef CMDM_USETCPIP
	switch(ret=cmdm_tcpip_handler()){	// handle tcpip stuff
	case CMDM_AGAIN:
		return CMDM_AGAIN;		// skip other handlers while busy
	case CMDM_ERROR:
		return CMDM_ERROR;
	case CMDM_UNKRC:			// pass unknown result codes to other children
		break;
	}
#endif
	/* other checks can be done when there is a TCP/IP connection,
	but not if transparent mode is in use, so... */
#ifdef CMDM_INCOMING
	// check RI activity for received SMSs or incoming calls
	// probably parse CMDM_UNKRC ?
#endif
#ifdef CMDM_USESMS
	// handle sms stuff
	// probably parse CMDM_UNKRC ?
#endif
#ifdef CMDM_USECALL
	// handle call stuff
	// probably parse CMDM_UNKRC ?
#endif
	/* priorities, exclusion, simultaneity:
	   the application (read: YOU) must take care of this when asking for
	   something to be done */
	if(ret == CMDM_UNKRC){ // children handlers did not understand the result code in line buffer
		// parse it (in case we know what to do...),
		cmdm_data_recvstr_start();	// and cleanup after handling so children can get their expected stuff
	}
	return CMDM_DONE;
}

// registered handler, children initialization
static void registered_init()
{
state_t *s = &cmdm.regsubstate;

	s->state = CMDM_REG_INIT;
#ifdef CMDM_INCOMING
	CMDM_RI_CLRINTFLG;
	CMDM_RI_ENABLEINTS;		// enable RI ints
#endif
#ifdef CMDM_USETCPIP
	// init some tcpip stuff ?
#endif
#ifdef CMDM_USESMS
	// check for new sms or believe RI ?
	// should RI int be enabled at start so we can't lose incoming msg indication ?
#endif
#ifdef CMDM_USECALL
	// nothing to do here ?
#endif
#ifdef CMDM_ESCAPE
	// nothing to do here ?
#endif
}

void cmdm_init(const cmdmfunctions_t *func, cmdm_timers_t *timers, char *pin)
{
	cmdm.state = CMDM_OFF;
	cmdm.timers = timers;
	cmdm.func = func;
	cmdm.ledstate = CMDMLED_NONE;
#ifdef CMDM_INCOMING
	cmdm.RIactivity = 0
#endif
	memcpy(cmdm.PIN, pin, 4);
	chat_fullinit(&cmdm.chat, &cmdm.func->chat, cmdm.timers->chat);
#ifdef CMDM_USETCPIP
	cmdm_tcpip_init(
	#ifdef CMDM_TCPIP_USESENDRECV
			cmdm.timers->tcpip
	#endif
	);
#endif
#ifdef CMDM_USESMS
	cmdm_sms_init();
#endif
#ifdef CMDM_USECALL
	cmdm_call_init();
#endif
}

void cmdm_start(void)
{
	cmdm.state = CMDM_START;
}

void cmdm_stop(void)
{
	cmdm.state = CMDM_STOP;
}

int cmdm_status(void)
{
	if(cmdm.state == CMDM_OFF)
		return CMDM_ISOFF;
	if(cmdm.state == CMDM_REGISTERED)
		return CMDM_ISUP;
	return CMDM_ISDOWN;
}

static int recvstring(char *sentence, unsigned char len, unsigned char *strpos)
{
int input_char;
int pos = (int) *strpos;

	while((input_char=cmdm.func->chat.listen()) != -1){	// process chars in serial input
		if(input_char == 0x0A){				// end string on LF
			if(pos < 3) {				// too short ? (3 chars (OK\r) at least)
				pos = 0;			// ignore short/empty lines 
				break;
			}
			sentence[pos] = '\0'; 			// add null termination (replace \r)
			return 1;
		} else if(input_char > '\0'){
			sentence[pos++] = (char) input_char;
			if(pos == len)
				--pos;				// truncate string if too large
		}
	}
	*strpos = pos;
	return 0;
}
int cmdm_data_recvstr(void)
{
	return recvstring(CMDM_LINEBUFFER, CMDM_LINEBFR_SIZE, &cmdm.lbpos);
}

void cmdm_data_recvstr_start()
{
	memset(CMDM_LINEBUFFER, '\0', CMDM_LINEBFR_SIZE);	// empty buffer
	cmdm.lbpos = 0;
	// flush rx ?	cmdm.func->chat.ignore(); // need to also define CHAT_FLUSHINPUTAFTERSEND
}

#ifdef CMDM_ESCAPE
/**
\warning Escape to command mode has only been simulated (not actually tested on real hardware)
*/
void cmdm_gocmd(void)
{
	CMDM_DTR_OFF;						// drop DTR
	cmdm_cmd_start("",(char *)CMDM_STR_OK, CMDM_CHAT_TIME);	// wait for OK
}

void cmdm_godata(void)
{
	cmdm_cmd_start((char *)CMDM_STR_ATO,			// send ATO
		(char *)CMDM_STR_CONNECT, CMDM_CHAT_TIME);	// wait for CONNECT
}
#endif

int cmdm_data_sendbyte(unsigned char data)
{
	if((*cmdm.func->chat.talk)(data) == -1)
		return CMDM_ERROR;
	return CMDM_DONE;
}

int cmdm_data_send(unsigned char *address, unsigned int bytes)
{
unsigned int count=0;
	
	while(bytes){						// pump while there are bytes
		if((*cmdm.func->chat.talk)(*(address++)) == -1)	// and room to put them
			break;
		++count;
		--bytes;
	}
	return count;	
}

int cmdm_data_recvbyte(void)
{
int data;

	if((data=(*cmdm.func->chat.listen)()) == -1)
		return CMDM_ERROR;
	return data;	
}
int cmdm_data_recv(unsigned char *address, unsigned int maxbytes)
{
unsigned int count=0;
int data;
	
	while(maxbytes){					// pump while there is room
		if((data=(*cmdm.func->chat.listen)()) == -1)	// and bytes to put inside
			break;
		*(address++) = (unsigned char)data;
		++count;
		--maxbytes;
	}
	return count;	
}

void cmdm_ledtick(void)
{
volatile sys_timer_t *timerp = cmdm.timers->led;
unsigned char *state = &cmdm.ledstate;
sys_timer_t timer = *timerp;

#ifdef CMDM_QUECTEL_GSM
/* LED changes periods */
	if(timer < (CMDMLED_TIMEOUT - CMDMLED_TIME_TOOLONG))		// longer than longest recognized blink period
		*state = CMDMLED_NONET;
	else if(timer < (CMDMLED_TIMEOUT - CMDMLED_TIME_NETWORK))	// registered
		*state = CMDMLED_NETWORK;
	else if(timer < (CMDMLED_TIMEOUT - CMDMLED_TIME_SEARCHING))	// no network
		*state = CMDMLED_NONET;
	// faster blinks = NONET (PPP is not used)
	*timerp = CMDMLED_TIMEOUT;					// restart timer
#else
	#ifdef CMDM_QUECTEL_3G
/* LED changes ratios, and periods...
measure off period (between falling and rising interrupts),
first interrupt has been intentionally setup for falling edge at module startup in handler */
	if(!CMDM_LED_INTISFALLING){	// rising
		if(timer < (CMDMLED_TIMEOUT - CMDMLED_TIME_TOOLONG))		// longer than longest recognized blink period
			*state = CMDMLED_NONET;
		else if(timer > (CMDMLED_TIMEOUT - CMDMLED_TIME_NETWORK))	// registered
			*state = CMDMLED_NETWORK;
		else if(timer < (CMDMLED_TIMEOUT - CMDMLED_TIME_SEARCHING))	// no network
			*state = CMDMLED_NONET;
		// faster blinks = NETWORK (data transfer)
		CMDM_LED_INTONFALLING;
	} else {			// falling
		CMDM_LED_INTONRISING;
	}
	*timerp = CMDMLED_TIMEOUT;						// restart timer
	/** \warning: A voice call state will cause a timeout, this needs to be addressed
	outside of this function, which will not be called (LED will stay on during call) */
	#endif
#endif
}


#ifdef CMDM_DEBUG
// Undocumented obscure functions used for testing purposes
int cmdm_getcur(void)
{
	return cmdm.state;
}
int cmdm_getnext(void)
{
	return cmdm.next_state;
}
#endif
