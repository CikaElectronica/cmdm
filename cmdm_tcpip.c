#ifdef CMDM_USETCPIP

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "cmdm.h"
#include "cmdmopts.h"
#include "cmdm_chatstrings_tcpip.h"

struct cmdm_APNstruct {
	char apn[APN_STRING_SIZE+1];		///< APN string,
	char user[USERPASS_STRING_SIZE+1];	///< username,
	char pass[USERPASS_STRING_SIZE+1];	///< and password 
};

typedef struct {
	unsigned char state;		///< handler state
	unsigned char connstate;	///< connection state
	struct cmdm_APNstruct apn;	///< APN info
#ifdef CMDM_TCPIP_USESENDRECV
	unsigned char datastate;	///< data transfer state
	unsigned char *txdataptr;	///< transmit data pointer, where to copy data from
	uint16_t txdatalen;		///< transmit data length, how many bytes to copy
	unsigned char *rxdataptr;	///< receive data pointer, where to store received data
	uint16_t rxdatasz;		///< max receive data length to copy to buffer
	void *datasentcback;		///< function to call when data has been sent
	void *datarecvdcback;		///< function to call when data has been copied to buffer
	void *dataerrorcback;		///< function to call when there was an error with data transfer
	volatile sys_timer_t *justask;	///< periodically ask the module for incoming data, in case an URC is lost in AT sea
#endif
} cmdm_tcpip_t;

static cmdm_tcpip_t tcpip;

enum CMDM_TCPIPstates {
	CMDM_TCPIP_STOP=0,		///< release IP address and disconnect
	CMDM_TCPIP_DOWN,		///< disconnected
	CMDM_TCPIP_START,		///< configure and get IP address
	CMDM_TCPIP_UP,			///< connected
};

static int cmdm_tcpip_connhandler(void);

int cmdm_tcpip_handler()
{
int ret;
cmdm_tcpip_t *t;

	t = &tcpip;
	switch(t->state){
	case CMDM_TCPIP_START:
		/*FALLTHROUGH*/
	case CMDM_TCPIP_STOP:
		if((ret=cmdm_cmd_status()) == CMDM_DONE)
			++(t->state);
		else if(ret == CMDM_ERROR)
			return CMDM_ERROR;
		break;
	case CMDM_TCPIP_UP:
		return cmdm_tcpip_connhandler();
	case CMDM_TCPIP_DOWN:
		return CMDM_DONE;
	default:
		return CMDM_ERROR;
	}
	return CMDM_AGAIN;
}


int cmdm_tcpip_status()
{
int ret = cmdm_status();

	if(ret != CMDM_ISUP)
		return ret;
	switch(tcpip.state){
	case CMDM_TCPIP_STOP:	// if it is stopping, it means it was up and not yet down...
		/*FALLTHROUGH*/
	case CMDM_TCPIP_UP:
		return CMDM_ISUP;
	}
	return CMDM_ISDOWN;
}

void cmdm_tcpip_setapn(char *apn, char *user, char *pass)
{
struct cmdm_APNstruct *ptr = &tcpip.apn;

	memcpy(ptr->apn,apn,APN_STRING_SIZE+1);
	memcpy(ptr->user,user,USERPASS_STRING_SIZE+1);
	memcpy(ptr->pass,pass,USERPASS_STRING_SIZE+1);
}

void cmdm_tcpip_start(void)
{
struct cmdm_APNstruct *apn = &tcpip.apn;

	sprintf(CMDM_LINEBUFFER,"%s,\"%s\",\"%s\",\"%s\"\r", CMDM_STR_SELCTX,
			apn->apn, apn->user, apn->pass);
	cmdm_mcmd_start((sendexpect_t *)CMDM_SX_TCPIPSTART, CMDM_CHAT_LONGTIME);
	tcpip.state = CMDM_TCPIP_START;
}
void cmdm_tcpip_stop(void)
{
	cmdm_cmd_start((char *)CMDM_STR_TCPIP_STOP, (char *)CMDM_STR_OK, CMDM_CHAT_LONGTIME);
	tcpip.state = CMDM_TCPIP_STOP;
}


enum CMDM_TCPIPconnstates {
	CMDM_TCPIP_CONNSTOP=0,		///< release IP address and disconnect
	CMDM_TCPIP_CONNDOWN,		///< disconnected
	CMDM_TCPIP_CONNSTART,		///< open
	CMDM_TCPIP_CONNWAIT,		///< wait for connect {remote end (TCP) / module (UDP)}
	CMDM_TCPIP_CONNUP,		///< connected
	CMDM_TCPIP_CONNSEND,		///< sending
	CMDM_TCPIP_CONNRECV		///< receiving
#ifdef CMDM_TCPIP_USETRANSPARENT
	,CMDM_TCPIP_TCONNSTOP		///< enter command mode, then go to CONNSTOP
#endif
};

#ifdef CMDM_TCPIP_USESENDRECV
static int cmdm_tcpip_datahandler(void);
#endif

#ifdef CMDM_TCPIP_USETRANSPARENT
/// TCP functions assume CMDM_TCPIP_USETRANSPARENT
void cmdm_tcpip_tcpopen(char *host, unsigned int port)
{
//	if(tcpip.connstate != CMDM_TCPIP_CONNDOWN)
//		return CMDM_ERROR;
#ifdef CMDM_QUECTEL_GSM
	sprintf(CMDM_LINEBUFFER,"%s\"TCP\",\"%s\",%u\r", CMDM_STR_TCPIP_CONN, host, port);
	cmdm_cmd_start(CMDM_LINEBUFFER, (char *)CMDM_STR_OK, CMDM_CHAT_TIME);
	tcpip.connstate = CMDM_TCPIP_CONNSTART;
#else
#ifdef CMDM_QUECTEL_3G
	sprintf(CMDM_LINEBUFFER,"%s\"TCP\",\"%s\",%u,0,2\r", CMDM_STR_TCPIP_CONN, host, port);
	cmdm_cmdresp_start(CMDM_LINEBUFFER, CMDM_CHAT_LONGTIME);
	tcpip.connstate = CMDM_TCPIP_CONNWAIT;
	/** \todo: Should we add a way to select transparent mode for QUECTEL_3G,
	supporting both at connection open, this would remove the mutual exclusion
	between transparent and non-transparent modes for this module.
	QUECTEL_GSM requires mode selection before TCP/IP network connection
	*/
#endif
#endif
}

void cmdm_tcpip_tcpclose(void)
{
	cmdm_gocmd();							// request going into command mode
	tcpip.connstate = CMDM_TCPIP_TCONNSTOP;
}
#endif

/**
\todo Extensive testing of responses in disconnection phase
\todo SendRecv has not been tested
*/
static int cmdm_tcpip_connhandler(void)
{
int ret;
cmdm_tcpip_t *t = &tcpip;

	switch(t->connstate){
	case CMDM_TCPIP_CONNSTART:					// wait OK from open command
		if((ret=cmdm_cmd_status()) == CMDM_ERROR)		// timeout -> no response
			return CMDM_ERROR;				//  signal error condition
		if(ret == CMDM_DONE){					// got OK,
			cmdm_cmdresp_start("", CMDM_CHAT_LONGTIME);	// now wait for connection result
			t->connstate = CMDM_TCPIP_CONNWAIT;
		}
		break;
#ifdef CMDM_TCPIP_USETRANSPARENT
	case CMDM_TCPIP_CONNWAIT:						// wait for command response,
		if((ret=cmdm_cmd_status()) == CMDM_ERROR){			// timeout ->  error (no response)
			t->connstate = CMDM_TCPIP_CONNDOWN;			// ignore, if this is a modem problem, next command will fail
			// return CMDM_ERROR;					// signal error condition (will reset)
		} else if(ret == CMDM_DONE){					// proper response, either good or bad
			if(strstr(CMDM_LINEBUFFER, CMDM_STR_TCPIP_CONNOK)){	// connected ?
				if(!strstr(CMDM_LINEBUFFER, CMDM_STR_TCPIP_NOCONN))	// some modems answer CONNECT or CONNECT FAIL, for example...
					t->connstate = CMDM_TCPIP_CONNUP;	// yes
				else 
					t->connstate = CMDM_TCPIP_CONNDOWN;	// no, see below regarding this action
			} else {						// not connected or unknown response
				t->connstate = CMDM_TCPIP_CONNDOWN;		// let the app decide what to do
				/** \todo: there can be an URC... need to properly parse or keep waiting until timeout */
			}
		}
		break;
	case CMDM_TCPIP_TCONNSTOP:					// wait response from close command
		if((ret=cmdm_cmd_status()) == CMDM_ERROR){		// timeout -> error (no response)
			ret = CMDM_DONE;				// ignore (connection might have been dropped and we were already in command mode)
		}
		if(ret == CMDM_DONE){					// we are now in command mode,
			cmdm_cmdresp_start((char *)CMDM_STR_TCPIP_DISC, CMDM_CHAT_LONGTIME);
			t->connstate = CMDM_TCPIP_CONNSTOP;		// send cmd and wait
		}
		break;
#endif
#ifdef CMDM_TCPIP_USESENDRECV
	case CMDM_TCPIP_CONNWAIT:						// wait for command response,
		if((ret=cmdm_cmd_status()) == CMDM_ERROR){			// timeout -> short timeout for network, error (no response)
			t->connstate = CMDM_TCPIP_CONNDOWN;			// ignore (we should wait more time (retry cmdresp() with no send string) or set a proper timeout
			// return CMDM_ERROR;					//  signal error condition
		} else if(ret == CMDM_DONE){					// proper response, either good or bad
			if(strstr(CMDM_LINEBUFFER, CMDM_STR_TCPIP_CONNOK)){	// connected
				t->connstate = CMDM_TCPIP_CONNUP;
			} else {						// not connected or unknown response
				t->connstate = CMDM_TCPIP_CONNDOWN;		// is this safe ?, should we signal error condition (which will reset..., other signal ?)
				/** @todo: there can be an URC... need to properly parse or keep waiting */
			}
		}
		break;
#endif
	case CMDM_TCPIP_CONNSTOP:					// wait response from close command
		if((ret=cmdm_cmd_status()) == CMDM_ERROR){		// timeout -> short timeout for network, error (no response)
			t->connstate = CMDM_TCPIP_CONNDOWN;		// ignore (we should wait more time (retry cmdresp() with no send string) or set a proper timeout
			// return CMDM_ERROR;				//  signal error condition
		} else if(ret == CMDM_DONE){				// proper response, either good or bad
			t->connstate = CMDM_TCPIP_CONNDOWN;		// ignore, conn is down anyway
		}
		break;
	case CMDM_TCPIP_CONNUP:
#ifdef CMDM_TCPIP_USESENDRECV
		// following line only for non-transparent mode
		return cmdm_tcpip_datahandler();			// propagate returned status
#endif
		// transparent mode does nothing during a connection (can't anyway... unless it exits to command mode)
		/*FALLTHROUGH*/
	case CMDM_TCPIP_CONNDOWN:
		return CMDM_DONE;
	default:
		return CMDM_ERROR;
	}
	return CMDM_AGAIN;
}

int cmdm_tcpip_connstatus(void)
{
int ret = cmdm_tcpip_status();

	if(ret != CMDM_ISUP)
		return ret;
	switch(tcpip.connstate){
	case CMDM_TCPIP_CONNDOWN:
		return CMDM_ISDOWN;
	case CMDM_TCPIP_CONNUP:
		return CMDM_ISUP;
	}
	return CMDM_ISPENDING;
}


void cmdm_tcpip_init(
#ifdef CMDM_TCPIP_USESENDRECV
			volatile sys_timer_t *justask
#else
			void
#endif
)
{
	tcpip.state = CMDM_TCPIP_DOWN;
	tcpip.connstate = CMDM_TCPIP_CONNDOWN;
#ifdef CMDM_TCPIP_USESENDRECV
	tcpip.datasentcback = NULL;		// no callbacks until the user sets them up
	tcpip.datarecvdcback = NULL;
	tcpip.dataerrorcback = NULL;
	tcpip.justask = justask;
#endif
}


#ifdef CMDM_TCPIP_USESENDRECV
#warning "'Send-Recv' mode is incomplete, cmdm_tcpip_datahandler(), the data handler function,\
 is under development, you might probably want to check later or read the docs and collaborate"

static void cmdm_tcpip_datainit(void);

/// UDP functions assume CMDM_TCPIP_USESENDRECV
void cmdm_tcpip_udpopen(char *host, unsigned int port)
{
//	if(tcpip.connstate != CMDM_TCPIP_CONNDOWN)
//		return CMDM_ERROR;
	sprintf(CMDM_LINEBUFFER,"%s\"UDP\",\"%s\",%u\r",
		CMDM_STR_TCPIP_CONN, host, port);
	cmdm_cmd_start(CMDM_LINEBUFFER, (char *)CMDM_STR_OK, CMDM_CHAT_TIME);
	tcpip.connstate = CMDM_TCPIP_CONNSTART;
	cmdm_tcpip_datainit();
}

void cmdm_tcpip_udpclose(void)
{
	cmdm_cmdresp_start((char *)CMDM_STR_TCPIP_DISC, CMDM_CHAT_TIME);
	tcpip.connstate = CMDM_TCPIP_CONNSTOP;
}

enum CMDM_TCPIPdatastates {
	CMDM_TCPIP_DATA_GOIDLE,		///< start, resume idle state after data flow
	CMDM_TCPIP_DATA_IDLE,		///< check module received data indication and user actions
	CMDM_TCPIP_DATA_ASKRCV,		///< ask the module for received data
	CMDM_TCPIP_DATA_SEND,		///< sending send command
	CMDM_TCPIP_DATA_SENDING,	///< sending data to the module
	CMDM_TCPIP_DATA_SENDEND,	///< expecting result for send operation
	CMDM_TCPIP_DATA_RECEIVE,	///< sending receive command
	CMDM_TCPIP_DATA_RECEIVING,	///< getting data from the module
	CMDM_TCPIP_DATA_RECEIVEND,	///< expecting result for receive operation
	CMDM_TCPIP_DATA_ERROR		///< oops
};

static void cmdm_tcpip_datainit(void)
{
	tcpip.datastate = CMDM_TCPIP_DATA_GOIDLE;
	tcpip.txdatalen = 0;		// no data waiting to be transmitted
	tcpip.rxdatasz = 0;		// no received data to be copied until destination is set
}

void cmdm_tcpip_data_setsentcback (void *cback(void))
{
	tcpip.datasentcback = cback;
}

void cmdm_tcpip_data_setrecvdcback(void *cback(void))
{
	tcpip.datarecvdcback = cback;
}

void cmdm_tcpip_data_seterrorcback(void *cback(void))
{
	tcpip.dataerrorcback = cback;
}

void cmdm_tcpip_data_setrecvdbfr(unsigned char *address, unsigned int size)
{
	tcpip.rxdataptr = address;
	tcpip.rxdatasz = size;
}

int cmdm_tcpip_data_send(unsigned char *address, unsigned int len)
{
	if(tcpip.txdatalen)
		return CMDM_ERROR;
	// if state is SEND or SENDING
	//	return CMDM_ERROR;
	tcpip.txdataptr = address;
	tcpip.txdatalen = len;
	return CMDM_DONE;
}

static int cmdm_tcpip_datahandler(void)
{
int ret;
cmdm_tcpip_t *t = &tcpip;

	switch(t->datastate){
#warning "see documentation for accurate description of this state machine before jumping into freerun coding"
	case CMDM_TCPIP_DATA_GOIDLE:
		*(t->justask) = CMDM_TCPIP_ASKRD_TIME;
		t->datastate = CMDM_TCPIP_DATA_IDLE;
		/*FALLTHROUGH*/
	case CMDM_TCPIP_DATA_IDLE: 						// need to stay waiting for unsolicited result codes
		if(cmdm_data_recvstr()) {					// got something ? then parse it
			if(strstr(CMDM_LINEBUFFER, CMDM_STR_TCPIP_HASRDI)){	// got rx indication ?
				cmdm_cmdresp_start((char*)CMDM_STR_TCPIP_RECV, CMDM_CHAT_TIME);
				t->datastate = CMDM_TCPIP_DATA_RECEIVE;
			} else {						// got other string
				// parse for errors ? then t->datastate = CMDM_TCPIP_DATA_ERROR
				// should we restart the timer ? don't think so; otherwise change state to GOIDLE
				return CMDM_UNKRC;				// pass unknown strings to other handlers
			}
		} else if(*(t->justask) == 0){					// no rx data for a while, check if the URC got lost in the AT flow
			cmdm_cmdresp_start((char*)CMDM_STR_TCPIP_CHKRD, CMDM_CHAT_TIME);
			t->datastate = CMDM_TCPIP_DATA_ASKRCV;
		} else if(t->txdatalen){					// is there data waiting to be sent ?
			sprintf(CMDM_LINEBUFFER,"%s%u\"\r",
				CMDM_STR_TCPIP_SEND, t->txdatalen);
			cmdm_cmdresp_start(CMDM_LINEBUFFER, CMDM_CHAT_TIME);
			t->datastate = CMDM_TCPIP_DATA_SEND;
		}
		break;
	case CMDM_TCPIP_DATA_ASKRCV:
		if((ret=cmdm_cmd_status()) == CMDM_ERROR){			// timeout -> short timeout for network, error (no response)
			// return CMDM_ERROR;					//  signal error condition
		} else if(ret == CMDM_DONE){					// proper response, either good or bad
			if(strstr(CMDM_LINEBUFFER, CMDM_STR_TCPIP_CONNOK)){	// connected
			} else {						// not connected or unknown response
			}
		}
		break;
	case CMDM_TCPIP_DATA_SEND:
	case CMDM_TCPIP_DATA_SENDING:
		
	case CMDM_TCPIP_DATA_RECEIVE:
	case CMDM_TCPIP_DATA_RECEIVING:
		
	case CMDM_TCPIP_DATA_ERROR:
	default:
		return CMDM_ERROR;
	}
	return CMDM_AGAIN;
}
#endif

#endif

