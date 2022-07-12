#include <stdio.h>
#include <pcap.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>

/* Ethernet addresses are 6 bytes */
#define ETHER_ADDR_LEN	6

/* Ethernet headers are always exactly 14 bytes */
#define SIZE_ETHERNET 14

#define SIZE_IPv6 40

#define SIZE_UDP 8

#define IPv6_ETHERTYPE 0x86DD

#define UDP 17

#define DNS_PORT 53

#define IP_VERSION(ip)		(ntohl((ip)->ip_vtcfl) >> 28)

/* Ethernet header */
struct sniff_ethernet {
	uint8_t         ether_dhost[ETHER_ADDR_LEN];	/* Destination host address */
	uint8_t         ether_shost[ETHER_ADDR_LEN];	/* Source host address */
	uint16_t        ether_type;	/* IP? ARP? RARP? etc */
};

	/* IPv6 header. RFC 2460, section3. Reading /usr/include/netinet/ip6.h is
	 * interesting */
struct sniff_ip {
	uint32_t        ip_vtcfl;	/* version << 4 then traffic class and flow
					 * label */
	uint16_t        ip_len;	/* payload length */
	uint8_t         ip_nxt;	/* next header (protocol) */
	uint8_t         ip_hopl;	/* hop limit (ttl) */
	struct in6_addr ip_src, ip_dst;	/* source and dest address */
};

/* UDP header */
struct sniff_udp {
	uint16_t        sport;	/* source port */
	uint16_t        dport;	/* destination port */
	uint16_t        udp_length;
	uint16_t        udp_sum;	/* checksum */
};

struct sniff_dns {
	/* RFC 1035, section 4.1 */
	uint16_t        query_id;
	uint16_t        codes;
	uint16_t        qdcount, ancount, nscount, arcount;
};
#define DNS_QR(dns)		((ntohs(dns->codes) & 0x8000) >> 15)
#define DNS_OPCODE(dns)	((ntohs(dns->codes) >> 11) & 0x000F)
#define DNS_RCODE(dns)	(ntohs(dns->codes) & 0x000F)

int
main(int argc, char *argv[])
{
	char           *filename, *nameserver, errbuf[PCAP_ERRBUF_SIZE];
	struct addrinfo *ip_ns;
	const struct sockaddr_in6 *addr;
	pcap_t         *handle;
	struct addrinfo hints_numeric;
	const uint8_t  *packet;	/* The actual packet */
	struct pcap_pkthdr header;	/* The header that pcap gives us */
	const struct sniff_ethernet *ethernet;	/* The ethernet header */
	const struct sniff_ip *ip;	/* The IP header */
	const struct sniff_udp *udp;	/* The UDP header */

	const struct sniff_dns *dns;
	u_int           size_ip;
	u_short         source_port, dest_port;
	char           *source, *destination;
	int             result;
	if (argc < 3) {
		fprintf(stderr, "Usage: readfile filename nameserver\n");
		return (2);
	}
	filename = argv[1];
	nameserver = argv[2];
	memset(&hints_numeric, 0, sizeof(hints_numeric));
	hints_numeric.ai_family = AF_INET6;
	hints_numeric.ai_flags = AI_NUMERICHOST;
	result = getaddrinfo(nameserver, NULL, &hints_numeric, &ip_ns);
	if (result != 0) {
		fprintf(stderr, "Invalid name server address %s\n", nameserver);
	}
	handle = pcap_open_offline(filename, errbuf);
	if (handle == NULL) {
		fprintf(stderr, "Couldn't open file %s: %s\n", filename, errbuf);
		return (2);
	}
	while (1) {
		/* Grab a packet */
		packet = pcap_next(handle, &header);
		if (packet == NULL) {	/* End of file */
			break;
		}
		ethernet = (struct sniff_ethernet *) (packet);
		if (ntohs(ethernet->ether_type) == IPv6_ETHERTYPE) {
			ip = (struct sniff_ip *) (packet + SIZE_ETHERNET);
			size_ip = SIZE_IPv6;
			if (IP_VERSION(ip) == 6) {	/* Yes, that's an IP packet */
				addr = (struct sockaddr_in6 *) ip_ns->ai_addr;
				if (memcmp(&ip->ip_src, &addr->sin6_addr, 16) == 0 ||
				    memcmp(&ip->ip_dst, &addr->sin6_addr, 16) == 0) {
					source = malloc(INET6_ADDRSTRLEN);
					destination = malloc(INET6_ADDRSTRLEN);
					inet_ntop(AF_INET6, &ip->ip_src, source,
						  INET6_ADDRSTRLEN);
					inet_ntop(AF_INET6, &ip->ip_dst, destination,
						  INET6_ADDRSTRLEN);
					if (ip->ip_nxt == UDP) {	/* TODO:
									 * handle
									 * the case
									 * where
									 * there are 
									 * other
									 * headers
									 * before
									 * the
									 * transport 
									 * * layer */
						udp =
						    (struct sniff_udp *) (packet +
									  SIZE_ETHERNET
									  + size_ip);
						source_port = ntohs(udp->sport);
						dest_port = ntohs(udp->dport);
						if (source_port == DNS_PORT
						    || dest_port == DNS_PORT) {
							dns =
							    (struct sniff_dns
							     *) (packet +
								 SIZE_ETHERNET +
								 size_ip + SIZE_UDP);
							printf
							    ("[%s]:%d -> [%s]:%d (QID %d, %s, Opcode %d, resp. code %d, answer(s) %d)\n",
							     source, source_port,
							     destination, dest_port,
							     ntohs(dns->query_id),
							     (DNS_QR(dns) ==
							      0 ? "Query" :
							      "Response"),
							     DNS_OPCODE(dns),
							     DNS_RCODE(dns),
							     ntohs(dns->ancount));
						}
					}
				}
			}
		}
	}
	/* And close the session */
	pcap_close(handle);

	return (0);
}
