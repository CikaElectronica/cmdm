#ifdef CMDM_USECALL
/// answer incoming voice call
/** starts voice connect process, call cmdm_tick() repeatedly
until success
\param1	cmdm_handler structure
*/
void cmdm_VoiceAnswer(void);

/// answer incoming data call
/** starts CSD connect process, call cmdm_tick() repeatedly
until success
\param1	cmdm_handler structure
*/
void cmdm_DataAnswer(void);

/// disconnect current CSD/voice call, reject incoming call
/** starts process, call cmdm_tick() repeatedly
until success
\param1	cmdm_handler structure
*/
void cmdm_Hangup(void);

/// dial voice call
/** starts voice connect process, call cmdm_tick() repeatedly until success.
Status will be available at gsm->data (CPASstates)
Once connected, check status with cmdm_CPAS() 
\param1	cmdm_handler structure
*/
void cmdm_VoiceConnect(char *num);

/// dial data call
/** starts CSD connect process, call cmdm_tick() repeatedly until success.
Status will be available at gsm->data (CPASstates)
\param1	cmdm_handler structure
*/
void cmdm_DataConnect(char *num);

enum CPASstates {
	CMDM_CPAS_IDLE=0,
	CMDM_CPAS_ERROR=2,
	CMDM_CPAS_RINGING=3,
	CMDM_CPAS_ONCALL=4,
	CMDM_DIAL_TIMEOUT,
	CMDM_DIAL_ERROR
};

/// call status
/** starts call status process, call cmdm_tick() repeatedly until success.
Status will be available at gsm->data (CPASstates)
\param1	cmdm_handler structure
*/
void cmdm_CPAS(void);

enum CMDMcalltypes {
CMDM_NOCALL=0,CMDM_INCCALL=CMDM_DONE,CMDM_VOICECALL,CMDM_DATACALL
};

/// handle RI activity
/** starts monitoring RI activity, call cmdm_tick() repeatedly until success.
Call type will be available at gsm->data (CMDM_NOCALL,CMDM_VOICECALL,CMDM_DATACALL)
Caller number will be at gsm->who (operator dependent)
\param1	cmdm_handler structure
*/
void cmdm_handleRI(void);
#endif

/// handle dial/answer/cmd mode/data mode/hangup/cpas chat
/** call repeatedly 
\param1	cmdm_handler structure
\retval	CMDM_AGAIN, call again
\retval	CMDM_DONE, mission accomplished
\retval	CMDM_ERROR, Houston, we have a problem...
*/
int cmdm_tick(void);

/// check RI activity
/** returns whether or not there was any RI activity since last call to cmdm_checkRI()
\param1	cmdm_handler structure
\retval 1, there was RI activity since last call
\retval 0, no RI activity
\warning RI activity is ignored during calls, only falling edges are detected;
RI activity can be due to incoming calls or SMS messages
*/
int cmdm_checkRI(void);


#ifdef CMDM_USETCPIP
/// start UDP connection on IP GPRS
/** starts connection process, call cmdm_GPRStick() repeatedly until connection
is established.
\param1	cmdm_handler structure
\param2	host IP (or name, depends on module configuration)
\param3	UDP port number
*/
void cmdm_GPRSUDPconnect(unsigned char *host, unsigned int port);

/// disconnect current connection
/** starts connection process, call cmdm_GPRStick() repeatedly until connection
is terminated.
\param1	cmdm_handler structure
*/
void cmdm_GPRSUDPdisc(void);

enum CIPstates {
	CMDM_GPRS_CONNECTED,
	CMDM_GPRS_IDLE
};

/// get current connection status
/** starts CIPSTATUS process, call cmdm_GPRStick() repeatedly until success.
Status will be available at gsm->data (CIPstates)
\param1	cmdm_handler structure
*/
void cmdm_GPRScipstatus(void);

/// handle connect/disconnect chat
/** call repeteadly until connection is established or terminated.
\param1	cmdm_handler structure
\retval	CMDM_AGAIN, call again
\retval	CMDM_DONE, mission accomplished
\retval	CMDM_ERROR, Houston, we have a problem...
*/
int cmdm_GPRStick(void);

/// escape to command mode
/** starts escaping into command mode process, call cmdm_GPRSconnection() repeatedly
until success
\param1	cmdm_handler structure
*/
void cmdm_goCMD(void);

/// return to data mode
/** starts returning to data mode process, call cmdm_GPRSconnection() repeatedly
until success
\param1	cmdm_handler structure
*/
void cmdm_goDATA(void);

#endif


#ifdef CMDM_USESMS
/// start sending an SMS
/** starts message sending process, call cmdm_SMStick() until it's done.
\param1	cmdm_handler structure
\param2	phone number to send SMS to
\param3	pointer to the function that actually sends the data, NULL if not used.
This function must return 0 if sending, 1 if done
\param4	parameter passed to the data handler or string address, if data handler is not used
*/
void cmdm_SMSsend(char *num,void *handler,void *data);

/// handle SMS send/receive/delete
/** call repeteadly
\param1	cmdm_handler structure
\retval	CMDM_AGAIN, call again
\retval	CMDM_DONE, mission accomplished
\retval	CMDM_ERROR, Houston, we have a problem...
*/
int cmdm_SMStick(void);

/// start deleting an SMS
/** starts message delete process, call cmdm_SMStick() until it's done.
\param1	cmdm_handler structure
\param2	message number to be deleted
*/
void cmdm_SMSdelete(unsigned char msg);

/// start reading an SMS
/** starts message read process, call cmdm_SMStick() until it's done.
\param1	cmdm_handler structure
\param2	message number to be read
\param3	pointer to the function that actually reads the data, NULL if not used
This function must return 0 if collecting, 1 if done
\param4	parameter passed to the data handler or string address, if data handler is not used.
In this case, buffer must hold max SMS size +1: 161 bytes. Msg text is appended by
<CR><LF>OK<CR><LF>
\warning Any messages the module might send during msg read are also appended, e.g.
if there is an incoming call, the +CRING and +CLIP messages will be appended (believe me,
it happened to me once when testing...)  
*/
void cmdm_SMSread(unsigned char msg,void *handler,void *data);

/// start checking for SMS
/** starts message check process, call cmdm_SMStick() until it's done.
Sender number is in gsm->who
Message number at gsm->data
\param1	cmdm_handler structure
*/
void cmdm_SMSpeek();
#endif

#endif

