#ifndef __CMDM_CHATSTRINGS_H
#define __CMDM_CHATSTRINGS_H

const char CMDM_STR_ECHOOFF[]="ate0\r";
const char CMDM_STR_CHKPIN[]="at+cpin?\r";
const char CMDM_STR_SETPIN[]="at+cpin=";
const char CMDM_STR_HASPIN[]="SIM PIN";
const char CMDM_STR_NOPIN[]="READY";
const char CMDM_STR_INIT[]="at&d1\r";

const char CMDM_STR_OK[]="ok";
#ifdef CMDM_ESCAPE
const char CMDM_STR_ATO[]="ato\r";
const char CMDM_STR_CONNECT[]="connect";
#endif

#define CMDM_SX_EOF {0,0}

const sendexpect_t CMDM_SX_CONFIG[]={
	{(char *)CMDM_STR_INIT, (char *)CMDM_STR_OK},
	CMDM_SX_EOF
};

#endif
