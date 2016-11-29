#ifndef __CMDM_H
#define __CMDM_H

/** define the following macros for functionality:
CMDM_USETCPIP
CMDM_USESMS (not yet implemented)
CMDM_USECALL (not yet implemented)
*/

extern char CMDM_LINEBUFFER[];		///< internal buffer to build strings for chatting and get parseable responses
#if (defined(CMDM_USETCPIP) && defined(CMDM_TCPIP_USETRANSPARENT)) || defined (CMDM_USECALL)
	#define CMDM_ESCAPE
#endif
#if defined(CMDM_USESMS) || defined (CMDM_USECALL)
	#define CMDM_INCOMING
#endif

#include "chat.h"

typedef struct {
	void (*init)(void); 		///< pointer to function to reset serial (usually also flushes rx buffers, see chat.h)
	chatfunctions_t chat;		///< see chat.h
} cmdmfunctions_t;

/** pointers to timers (decremented outside of this function) */
typedef struct {
	volatile sys_timer_t *powbut;
	volatile sys_timer_t *led;
	volatile sys_timer_t *network;
	volatile sys_timer_t *chat;
#ifdef CMDM_TCPIP_USESENDRECV
	volatile sys_timer_t *tcpip;
#endif
} cmdm_timers_t;



/** Initializes control data
\param1 pointer to I/O functions
\param2 structure of pointers to externally handled time variables (timers), counting down to zero
\param3 pointer to PIN, 4 chars, text; good-practice: "0000" if not used (although this is a valid pin...)
*/
void cmdm_init(const cmdmfunctions_t *func, cmdm_timers_t *timers, char *pin);

/// set SIM PIN
/** just copies the info to be used later when starting the module
\param1	PIN, text
*/
void cmdm_setpin(char *pin);

/** Cell modem control: start
\note	The module must register to the network within CMDM_REGISTER_TIME seconds, and pulse the LED within CMDMLED_TIMEOUT ticks, otherwise it is reset 
*/ 
void cmdm_start(void);

/** Cell modem control: stop
*/ 
void cmdm_stop(void);

/** Cell modem handler process
Handles all "manual" steps involved in turning on/off
the cell modem module, checks LED for network status, etc.
\note	Must be called as frequently as possible, does not busy wait nor waste time
*/ 
void cmdm_handler(void);

/** LED tick handler
Measures LED blink time to detect network status.
\note	Call when LED turns on
\note	Safe to call within interrupts, no C-library functions called
*/ 
void cmdm_ledtick(void);

/** Return modem operational state
\retval	CMDM_ISOFF if the modem is off
\retval	CMDM_ISDOWN if the modem is unregistered (either registering or going off)
\retval	CMDM_ISUP if the modem is registered
*/ 
int cmdm_status(void);


/** Send a command, expect a fixed and invariant string
\param1 pointer to send string
\param2 pointer to expect string
\param3 time to wait (in timer units, depends on system handling)
\note see chat.h for strings rules
*/
void cmdm_cmd_start(char *send, char *expect, unsigned int timeout);

/** Send a command, expect a variable response string that will be parsed by the user
\param1 pointer to send string
\param2 time to wait (in timer units, depends on system handling)
\note see chat.h for strings rules
\note returned string is available at a global buffer, the address is CMDM_LINEBUFFER, whose room is
CMDM_LINEBFR_SIZE, user defined at cmdmopts.h
*/
void cmdm_cmdresp_start(char *send, unsigned int timeout);

/** Send a series of commands, expecting fixed and invariant strings as responses
\param1 pointer to sendexpect type
\param2 time to wait (in timer units, depends on system handling)
\note See chat.h for details on sendexpect type and strings rules
*/
void cmdm_mcmd_start(sendexpect_t *strings, unsigned int timeout);

/** Return command execution status
\retval	CMDM_AGAIN if the modem did not answer yet
\retval	CMDM_DONE if there is a modem response, as expected
\retval	CMDM_ERROR if chatting timed out before the expected response arrived
\note Handling of the error condition is responsability of the one issuing the respective command.
\note For cmdm_cmdresp_start(), CMDM_DONE means there is a response in the buffer, you will know better
when you parse the buffer. Conversely, CMDM_ERROR means whatever was received (if anything) was not a
whole line (no line end) when the timer expired.
*/
int cmdm_cmd_status(void);

#ifdef CMDM_ESCAPE
/** Request enter to command state
\note Handling of the error condition is responsability of the one issuing the respective command.
Check this by calling cmdm_cmd_status()
\note CMDM_DONE means the modem answered OK
\note CMDM_ERROR means it did not. This can be because the modem was already in command mode, because connection was lost
*/

void cmdm_gocmd(void);
/** Request returning to data transfer state
\note Handling of the error condition is responsability of the one issuing the respective command.
Check this by calling cmdm_cmd_status()
\note CMDM_DONE means the modem answered CONNECT
\note CMDM_ERROR means it did not. This can be because the modem was already in data mode (?)
*/
void cmdm_godata(void);
#endif

/** Send one byte to the modem
\retval CMDM_ERROR	if couldn't send (usually means no room in tx buffer or UART busy, or both...)
\retval	CMDM_DONE	if OK
*/
int cmdm_data_sendbyte(unsigned char data);

/** Send as many bytes as possible
\param1	pointer to where to copy the data from
\param2	desired number of bytes to send
\returns	actual number of bytes sent
*/
int cmdm_data_send(unsigned char *address, unsigned int bytes);

/** Receive one byte from the modem
\retval CMDM_ERROR	if there is no data
\returns	received data
*/
int cmdm_data_recvbyte(void);

/** Receive as many bytes as available, but not more than a maximum
\param1	pointer to where to copy the data
\param2	max number of bytes to copy (usually the buffer size)
\returns	number of bytes received (copied)
*/
int cmdm_data_recv(unsigned char *address, unsigned int maxbytes);

#define CMDM_AGAIN	0
#define CMDM_DONE	1
#define CMDM_ERROR	-1

#define CMDM_ISOFF	0
#define CMDM_ISDOWN	-1
#define CMDM_ISUP	1

// internal library use
// the function returning this has got an unknown atring and left it in the line buffer for others to parse
#define CMDM_UNKRC	2
// return 1 when a string is in the line buffer
int cmdm_data_recvstr(void);
// restart string collection
void cmdm_data_recvstr_start(void);


#ifdef CMDM_USETCPIP
#include "cmdm_tcpip.h"
#endif
#ifdef CMDM_USESMS
#include "cmdm_sms.h"
#endif
#ifdef CMDM_USECALL
#include "cmdm_call.h"
#endif

#endif

