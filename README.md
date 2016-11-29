# cmdm
Cell modem control for embedded (read: resource constrained) systems

define CHAT_FULLCHAT, this library requires that functionality from the chat module
define CMDM_USETCPIP to include support for data connection
define CMDM_TCPIP_USETRANSPARENT to use TCP in transparent mode
there is also a non-transparent mode using UDP, but it has not been finished.

define your (supported) cell modem:
	CMDM_QUECTEL_GSM,	tested on M10
	CMDM_QUECTEL_3G,	tested on UC15

###Quick start:
Read the documentation in the header files and follow the example
cmdm_init() initializes the handler structure
cmdm_start() powers the modem on and starts the registration process
cmdm_handler() handles the entire process, call frequently
the _status() functions return the corresponding status
####E.g.:
```
static const cmdmfunctions_t myfuncs = {UART_initflush,{UART_tx, UART_rx}};

const static cmdm_timers_t mycmdmtimers = {
	&cmdm,
	&cmdmled,
	&cmdm,
	&chat
};

	cmdm_init(&myfuncs, (cmdm_timers_t *)&mycmdmtimers, (char *)SIM_PIN);
	cmdm_start();
	while(1){	
		cmdm_handler();
		switch(state){
		case COMM_WAITREG:
			if(cmdm_status() == CMDM_ISUP){
				state = COMM_WAITIP;
				cmdm_tcpip_setapn(APNHOST, APNUSER, APNPASS);
				cmdm_tcpip_start();
			}
			break;
		case COMM_WAITIP:
			if(cmdm_tcpip_status() == CMDM_ISUP){
				state = COMM_WAITSTART;
				sys_stimers.one.app = 0;	// do not wait on start
			}
			break;
		case COMM_WAITSTART:
			if(sys_stimers.one.app == 0){
				state = COMM_WAITTCP;
				cmdm_tcpip_tcpopen(TCPHOST, TCPPORT);
			}
			break;
		case COMM_WAITTCP:
			switch(cmdm_tcpip_connstatus()){
			case CMDM_ISUP:
				state = COMM_UP
				break;			
			case CMDM_ISDOWN:
				// oops
				break;
			}
			break;
		case COMM_UP:
			// use cmdm_data_send() and cmdm_data_recv()
			break;
		case COMM_WAITTCPDISC:
			if(cmdm_tcpip_connstatus() == CMDM_ISDOWN)
				state = COMM_WAITIPDISC;
			break;
		case COMM_WAITIPDISC:
			if(cmdm_tcpip_status() == CMDM_ISDOWN)
				cmdm_stop();
			break;
		case COMM_WAITSTOP:
			if(cmdm_status() == CMDM_ISOFF)
				state = COMM_DOWN;
			break;
		case COMM_DOWN:
			break;
		}
	}
```