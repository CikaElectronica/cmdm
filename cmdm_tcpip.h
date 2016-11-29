/// set APN information for network provider
/** just copies the info to be used later when starting the connection
\param1	apn name
\param2	user name
\param2	password
*/
void cmdm_tcpip_setapn(char *apn, char *user, char *pass);

/// start TCP/IP networking
/** Connects to the network and obtains IP address
\note	Call cmdm_tcpip_status() to know when this is done
*/
void cmdm_tcpip_start(void);

/// stop TCP/IP networking
/** Releases IP address and disconnects from the network
\note	Call cmdm_tcpip_status() to know when this is done
*/
void cmdm_tcpip_stop(void);

/// Return TCP/IP and modem operational state
/** It is a superset of cmdm_status()
\retval	CMDM_ISOFF if the modem is off
\retval	CMDM_ISDOWN if the modem is unregistered or TCP/IP is down
\retval	CMDM_ISUP if the modem is registered and TCP/IP is up (we do have an IP address)
*/ 
int cmdm_tcpip_status(void);

/// Return connection state
/** It is a superset of cmdm_tcpip_status()
\retval	CMDM_ISOFF if the modem is off
\retval	CMDM_ISDOWN if the connection is down (or the modem is unregistered or TCP/IP is down)
\retval	CMDM_ISUP if the connection is up (which also means the modem is registered and TCP/IP is up)
\retval	CMDM_ISPENDING if the module is in the connect/disconnect process
*/ 
int cmdm_tcpip_connstatus(void);

#ifdef CMDM_TCPIP_USETRANSPARENT

/// start TCP communication with an IP address
/** starts connection process
\note	check connection is established by calling cmdm_tcpip_connstatus()
\param1	host IP (or name, depends on module configuration)
\param2	TCP port number
*/
void cmdm_tcpip_tcpopen(char *host, unsigned int port);

/// terminate current TCP communication
/** starts connection process
\note	check connection is closed by calling cmdm_tcpip_connstatus()
\param1	cmdm_handler structure
*/
void cmdm_tcpip_tcpclose(void);

#endif

#ifdef CMDM_TCPIP_USESENDRECV
/// start UDP communication with an IP address
/** starts connection process
\note	check connection is established by calling cmdm_tcpip_connstatus()
\param1	host IP (or name, depends on module configuration)
\param2	UDP port number
*/
void cmdm_tcpip_udpopen(char *host, unsigned int port);

/// terminate current UDP communication
/** starts connection process
\note	check connection is closed by calling cmdm_tcpip_connstatus()
\param1	cmdm_handler structure
*/
void cmdm_tcpip_udpclose(void);

/// start UDP data send on an open communication
/** starts data send process
\param1	source address, where the data resides
\param2	number of bytes to send
\retval	CMDM_ERROR	connection (or etc.) is down
\retval CMDM_DONE	data will be sent
*/
int cmdm_tcpip_udpsend(unsigned char *data, unsigned int len);

/// start UDP data receive on an open communication
/** starts data send process
\param1	destination address, where the data will be copied
\param2	max number of bytes to copy
\retval	0	no data is available, no data will be copied
\retval	CMDM_ERROR	connection (or etc.) is down
\retval >0	number of bytes to copy in buffer
*/
int cmdm_tcpip_udprecv(unsigned char *data, unsigned int len);

#endif

#define CMDM_ISPENDING	2	///< not up nor down

// internal library use
int cmdm_tcpip_handler(void);
void cmdm_tcpip_init(
#ifdef CMDM_TCPIP_USESENDRECV
			volatile sys_timer_t *
#else
			void
#endif
);

#if !defined(CMDM_TCPIP_USETRANSPARENT) && !defined(CMDM_TCPIP_USESENDRECV)
#error "\
At least one of CMDM_TCPIP_USETRANSPARENT or CMDM_TCPIP_USESENDRECV \
must be defined (assuming the module you use supports transparent mode...)"
#endif

#if defined(CMDM_QUECTEL_GSM) && defined(CMDM_TCPIP_USETRANSPARENT) && defined(CMDM_TCPIP_USESENDRECV)
#error "\
Choose only one of CMDM_TCPIP_USETRANSPARENT or CMDM_TCPIP_USESENDRECV,\
although the module can use anyone of them, this library so far only \
supports using one or the other for GSM modules, feel free to modify \
it yourself if you need this feature."
#endif

#if defined(CMDM_QUECTEL_3G) && defined(CMDM_TCPIP_USETRANSPARENT) && defined(CMDM_TCPIP_USESENDRECV)
#error "\
Choose only one of CMDM_TCPIP_USETRANSPARENT or CMDM_TCPIP_USESENDRECV,\
although the module supports both modes on a connection basis, this library \
so far only handles one case at a time for 3G modules, feel free to modify \
it yourself if you need this feature."
#endif
