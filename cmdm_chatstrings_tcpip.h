#ifndef __CMDM_CHATSTRINGS_H
#define __CMDM_CHATSTRINGS_H

extern const char CMDM_STR_OK[];
extern const char CMDM_STR_CONNECT[];

#define CMDM_SX_EOF {0,0}


#ifdef CMDM_QUECTEL_GSM
const char CMDM_STR_SELCTX[]="at+qicsgp=1";
const char CMDM_STR_REGCTX[]="at+qiregapp\r";
const char CMDM_STR_ACTCTX[]="at+qiact\r";
const char CMDM_STR_TCPIP_CONN[]="at+qiopen=";
const char CMDM_STR_TCPIP_DISC[]="at+qiclose\r";
const char CMDM_STR_TCPIP_STOP[]="at+qideact\r";
#ifdef CMDM_TCPIP_USETRANSPARENT
const char CMDM_STR_TRANSP[]="at+qimode=1\r";
const char CMDM_STR_TCPIP_CONNOK[]="CONN";
const char CMDM_STR_TCPIP_NOCONN[]="FAIL";
#endif
#ifdef CMDM_TCPIP_USESENDRECV
const char CMDM_STR_TCPIP_BUFF[]="at+qindi=1\r";
const char CMDM_STR_TCPIP_SEND[]="at+qisend=\r";
const char CMDM_STR_TCPIP_RECV[]="at+qird=0,1,0,1500\r";
const char CMDM_STR_TCPIP_CHKRD[]="at+qird=0,1,0,0\r";
const char CMDM_STR_TCPIP_HASRDI[]="+QIRDI:";
const char CMDM_STR_TCPIP_HASRDR[]="+QIRD:";
const char CMDM_STR_TCPIP_CONNOK[]="OK";
#endif
#else
#ifdef CMDM_QUECTEL_3G
const char CMDM_STR_SELCTX[]="at+qicsgp=1,1";
const char CMDM_STR_ACTCTX[]="at+qiact=1\r";
const char CMDM_STR_TCPIP_CONN[]="at+qiopen=1,1,";
const char CMDM_STR_TCPIP_DISC[]="at+qiclose=1\r";
const char CMDM_STR_TCPIP_STOP[]="at+qideact=1\r";
#ifdef CMDM_TCPIP_USETRANSPARENT
const char CMDM_STR_TCPIP_CONNOK[]="CONN";
const char CMDM_STR_TCPIP_NOCONN[]="ERROR";
#endif
#ifdef CMDM_TCPIP_USESENDRECV
const char CMDM_STR_TCPIP_SEND[]="at+qisend=1,\r";
const char CMDM_STR_TCPIP_RECV[]="at+qird=1\r";
const char CMDM_STR_TCPIP_CHKRD[]="at+qird=1,0\r";
const char CMDM_STR_TCPIP_HASRDI[]="+QIURC:";
const char CMDM_STR_TCPIP_HASRDR[]="+QIRD:";
const char CMDM_STR_TCPIP_CONNOK[]="1,0";		// +QIOPEN: ...con,0
#endif
#endif
#endif

#if defined(CMDM_QUECTEL_GSM) || defined(CMDM_QUECTEL_3G)
#endif

#ifdef CMDM_QUECTEL_GSM
const sendexpect_t CMDM_SX_TCPIPSTART[]={
#ifdef CMDM_TCPIP_USETRANSPARENT
	{(char *)CMDM_STR_TRANSP, (char *)CMDM_STR_OK},
#else
	{(char *)CMDM_STR_TCPIP_BUFF, (char *)CMDM_STR_OK},
#endif
	{CMDM_LINEBUFFER, (char *)CMDM_STR_OK},
	{(char *)CMDM_STR_REGCTX, (char *)CMDM_STR_OK},
	{(char *)CMDM_STR_ACTCTX, (char *)CMDM_STR_OK},
	CMDM_SX_EOF
};
#else
#ifdef CMDM_QUECTEL_3G
const sendexpect_t CMDM_SX_TCPIPSTART[]={
	{CMDM_LINEBUFFER, (char *)CMDM_STR_OK},
	{(char *)CMDM_STR_ACTCTX, (char *)CMDM_STR_OK},
	CMDM_SX_EOF
};
#endif
#endif

#endif
