/* include your hardware definitions file, e.g.:
#include <HT32F125x.h>  */
#include "myhardwareinclude.h"


/* Define your I/O ports and access macros */
#define CMDM_PSCTLb	0		// I/O port bit number
#define CMDM_PSCTLp 	FAKEPORT	// I/O port name
#define CMDM_PSCTL_ON	(CMDM_PSCTLp |=  (1<<CMDM_PSCTLb))
#define CMDM_PSCTL_OFF	(CMDM_PSCTLp &= ~(1<<CMDM_PSCTLb))

#define CMDM_DTRb 	1
#define CMDM_DTRp 	FAKEPORT
#define CMDM_DTR_OFF	(CMDM_DTRp |=  (1<<CMDM_DTRb))
#define CMDM_DTR_ON	(CMDM_DTRp &= ~(1<<CMDM_DTRb))

#define CMDM_PWRKEYb 	2
#define CMDM_PWRKEYp 	FAKEPORT
#define CMDM_PWRKEY_ON	(CMDM_PWRKEYp |=  (1<<CMDM_PWRKEYb))
#define CMDM_PWRKEY_OFF	(CMDM_PWRKEYp &= ~(1<<CMDM_PWRKEYb))

/* Define some macros to access interrupt flags and enable/disable specific interrupts */
#define	CMDM_RI_ENABLEINTS
#define CMDM_RI_DISABLEINTS
// remove traces from prior interrupts
#define	CMDM_RI_CLRINTFLG

#define	CMDM_LED_ENABLEINTS
#define CMDM_LED_DISABLEINTS
// remove traces from prior interrupts
#define	CMDM_LED_CLRINTFLG

// you may define this to empty, depending on your module, see cmdm_ledtick() in cmdm.c
#define CMDM_LED_INTONFALLING	(FAKEPORT |=  (1<<7))
// these might not be needed, depending on your module, see cmdm_ledtick() in cmdm.c
#define CMDM_LED_INTONRISING	(FAKEPORT &= ~(1<<7))
#define CMDM_LED_INTISFALLING	(FAKEPORT &  (1<<7))
#define CMDM_LEDb 	
#define CMDM_LEDp 	
#define CMDM_LED_ISON	

