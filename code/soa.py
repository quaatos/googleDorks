#!/usr/bin/python

# DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE 
#
#                    Version 2, December 2004 
#
# Copyright (C) 2004 Sam Hocevar  
#
# Everyone is permitted to copy and distribute verbatim or modified 
# copies of this license document, and changing it is allowed as long 
# as the name is changed. 
#
#            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE 
#   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION 
#
#  0. You just DO WHAT THE FUCK YOU WANT TO. 

import sys
import os
import dns.resolver
import dns.query
import argparse

# parse args
parser = argparse.ArgumentParser()
parser.add_argument("-4", "--ipv4",help="Query only IPv4 name servers",dest='v4only',action="store_true")
parser.add_argument("domain", help="Domain")

args = parser.parse_args()

domain = args.domain
dov6 = not args.v4only

print ('Checking soa for' + domain)

# Tupple name, ip
ns_domain = [] 

# Search NS for domain
try:
    answers = dns.resolver.query(qname = dns.name.from_text(domain),
                             rdtype='ns' , )
except Exception as e:
    print ("Error {0} Querying domain {1} NS : {2}".format(type(e).__name__.
           domain,e.message))
    exit(1)

# Search IP6/IP4 for NS
for ans in answers: 
    ns_name = ans.to_text()

if dov6:
    try:
        answer_aaaa = dns.resolver.query(qname = ns_name, rdtype = 'aaaa')
        for ip6 in answer_aaaa.rrset:
            ns_domain.append((ns_name,ip6.to_text()))

    except dns.resolver.NoAnswer:
        pass 
    except Exception as e:
        print ("Error {0} Querying host {1} : {2}".format(type(e).__name__,ns_name,e.message))
try:
    answer_a = dns.resolver.query(qname = ns_name, rdtype = 'a')
    for ip4 in answer_a.rrset:
       ns_domain.append((ns_name,ip4.to_text()));
except dns.resolver.NoAnswer:
    # No IPv4 and no IPV6
    pass
except Exception as e:
    print ("Error {0} Querying host {1} : {2}".format(type(e).__name__,ns_name,e.message))
#ns_domain.append(('dns google','8.8.8.8'))

# Query all nameservers for SOA
query_message = dns.message.make_query(qname = domain, rdtype = dns.rdatatype.SOA, use_edns = True, want_dnssec = True)
query_message.flags &= ~dns.flags.RD
query_message.payload = 4096

name_soa = dns.name.from_text(domain)

for (ns,ip) in ns_domain:
  try:
      answers = dns.query.udp( q = query_message,
	   where = ip,
	   timeout = 10,
      )
      soa = answers.get_rrset(name=name_soa,
                              section=answers.answer,
                              rdclass=dns.rdataclass.IN,
                              rdtype=dns.rdatatype.SOA
        )

      if (answers.rcode() != dns.rcode.NOERROR):
         print ("{name:<60s} : {err:s}".format(name=ns + ' ('+ip+')',err=dns.rcode.to_text(answers.rcode())))
      elif (not (answers.flags & dns.flags.AA)):
         print ("{name:<60s} : {soa:s}".format(name=ns + ' ('+ip+')',soa='Not Authoritative'))
      elif soa == None:
         print ("{name:<60s} : {soa:s}".format(name=ns + ' ('+ip+')',soa='AA set but no answer'))
      else:
         print ("{name:<60s} : {soa:d}".format(name=ns + ' ('+ip+')',soa=soa[0].serial))
  except EnvironmentError as e:
       print ("{name:<60s} : {error:s}".format(name=ns + ' ('+ip+')',error=e.strerror))
  except Exception as e:
       print ("{name:<60s} : {error:s}".format(name=ns + ' ('+ip+')',error=type(e).__name__))