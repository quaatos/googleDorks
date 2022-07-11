#!/bin/sh

# Simple Nagios/Icinga plugin for whois
# Found on http://jon.netdork.net/2009/03/09/nagios-and-monitoring-whois/

WHOIS="/usr/bin/whois"
GREP="/usr/bin/grep"
WC="/usr/bin/wc"

W_HOST=$1
D_LOOK=$2
S_MATCH=$3

if [ -z "$S_MATCH" ]; then
    echo "Usage: $0 server domain string_to_test"
    exit 3
fi

#logger "Test Whois ${W_HOST} ${D_LOOK} $S_MATCH"
R_OUT=`${WHOIS} -h ${W_HOST} ${D_LOOK} 2>/dev/null`

if [ $? -ne 0 ];
then
  echo "WHOIS server '${W_HOST}' did not respond"
  exit 2
fi

L_CNT=`echo ${R_OUT} | ${GREP} "${S_MATCH}" | ${WC} -l`

if [ ${L_CNT} -eq 0 ];
then
  echo "WHOIS server '${W_HOST}' did not return a match on ${D_LOOK}"
  exit 1
else
  echo "OK - Found ${L_CNT} match"
  exit 0
fi

