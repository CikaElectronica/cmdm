#ifndef __CMDM_OPTIONS
#define __CMDM_OPTIONS

// Time before power applied and power button pressure, in ticks
#define CMDM_POWERON_GUARDTIME	5
// Time to hold the power key pressed, when turning on
#define CMDM_POWERKEY_PRESSON	15
// Time to wait for signs of life (LED pulses), in ticks
#define CMDM_ALIVE_TIMEOUT	60
// Time before module on detection and module initialization, in ticks
#define CMDM_BUTTON_GUARDTIME	5
// Time to wait before asking for the SIM PIN (some modules/SIM cards report CME ERROR SIM BUSY)
#define CMDM_ASKSIM_GUARDTIME	30
// Time to wait for registering to the network, in seconds
#define CMDM_REGISTER_TIME	60
// Time window to check for LED pulses, in ticks
#define CMDMLED_TIMEOUT		100
// Time to hold the power key pressed, when turning off
#define CMDM_POWERKEY_PRESSOFF	15
// Time to wait for network logout, in seconds
#ifdef CMDM_QUECTEL_GSM
#define CMDM_UNREGISTER_TIME	61
#else
#ifdef CMDM_QUECTEL_3G
#define CMDM_UNREGISTER_TIME	13
#endif
#endif
// Time before network logout and power removed, in ticks
#define CMDM_POWEROFF_GUARDTIME	10
// after powering off the module, wait this lapse before doing anyhing else, in ticks
#define CMDM_RESTART_GUARDTIME	20

// Buffer size for string I/O. Response is truncated if longer than this
#ifdef CMDM_USETCPIP
#define CMDM_LINEBFR_SIZE	90
#else
#define CMDM_LINEBFR_SIZE	80
#endif

// Time to wait for an answer, standard "built-in" response, in ticks
#define CMDM_CHAT_TIME		5
// Time to wait for an answer, action depends on the network, in ticks
#define CMDM_CHAT_LONGTIME	1550

#ifdef CMDM_USETCPIP
	#ifdef CMDM_TCPIP_USESENDRECV
/* Safety timeout to ask for outstanding data.
If there has been no unsolicited strings announcing incoming data when this timer expires,
the library will ask the modem if it has some. In ticks */
#define CMDM_TCPIP_ASKRD_TIME	20
	#endif
#endif


/* CONFIGURATION PARAMETERS, string sizes, number of elements */

#define E164_NUMBER_SIZE	15
#define APN_STRING_SIZE		36
#define USERPASS_STRING_SIZE	16
#define IP_STRING_SIZE		15
#define SMS_NUMBER_SIZE		E164_NUMBER_SIZE+1
#define VPHONE_NUMBER_SIZE	E164_NUMBER_SIZE+1
// this one should accomodate both SMS and VPHONE
#define CMDM_WHO_SIZE		VPHONE_NUMBER_SIZE

#ifdef CMDM_USETCPIP
	#if (CMDM_LINEBFR_SIZE < (APN_STRING_SIZE+2*USERPASS_STRING_SIZE+20))
		#error "CMDM_LINEBFR_SIZE too small, can't accomodate apn + user + pwd + at syntax"
	#endif
#endif

#ifdef CMDM_QUECTEL_GSM
/* LED blink periods, higher than this value is accepted as such,
 in this priority order */
#define CMDMLED_TIME_TOOLONG	25	// longer than longest recognized blink period
#define CMDMLED_TIME_NETWORK	19	// registered blink period
#define CMDMLED_TIME_SEARCHING	7	// no network blink period
#define CMDMLED_TIME_PPPGPRS 	5	// PPP GPRS blink period
#else
	#ifdef CMDM_QUECTEL_3G
// higher than this means no pulse
#define CMDMLED_TIME_TOOLONG	25	// longer than longest recognized blink period
/* LED off periods, lower than this value is accepted as such,
 in this priority order */
#define CMDMLED_TIME_SEARCHING	16	// no network LED off period
#define CMDMLED_TIME_NETWORK	4	// registered LED off period
	#endif
#endif

#endif
