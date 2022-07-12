#include <stdio.h>
#include <pcap.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>

/* Ethernet addresses are 6 bytes */
#define ETHER_ADDR_LEN	6

/* ethernet headers are always exactly 14 bytes */
#define SIZE_ETHERNET 14

#define IPv6_ETHERTYPE 0x86DD

#define IP_V(ip)		(ntohl((ip)->ip_vtcfl) >> 28)

/* Ethernet header */
struct sniff_ethernet {
	u_char          ether_dhost[ETHER_ADDR_LEN];	/* Destination host address */
	u_char          ether_shost[ETHER_ADDR_LEN];	/* Source host address */
	u_short         ether_type;	/* IP? ARP? RARP? etc */
};

	/* IPv6 header. RFC 2460, section3. Reading /usr/include/netinet/ip6.h is
	 * interesting */
struct sniff_ip {
	uint32_t        ip_vtcfl;	/* version then traffic class and flow label 
					 */
	uint16_t        ip_len;	/* payload length */
	uint8_t         ip_nxt;	/* next header (protocol) */
	uint8_t         ip_hopl;	/* hop limit (ttl) */
	struct in6_addr ip_src, ip_dst;	/* source and dest address */
};

int
main(int argc, char *argv[])
{
	char           *filename, errbuf[PCAP_ERRBUF_SIZE];
	pcap_t         *handle;
	const u_char   *packet;	/* The actual packet */
	struct pcap_pkthdr header;	/* The header that pcap gives us */
	const struct sniff_ethernet *ethernet;	/* The ethernet header */
	const struct sniff_ip *ip;	/* The IP header */
	u_int           size_ip;
	char           *source, *destination;
	if (argc < 2) {
		fprintf(stderr, "Usage: readfile filename\n");
		return (2);
	}
	filename = argv[1];
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
			size_ip = ip->ip_len * 4;
			if (IP_V(ip) == 6) {	/* Yes, that's an IP packet */
				/* Print its length */
				printf
				    ("Got an IPv6 packet of protocol [%d]: ",
				     ip->ip_nxt);
				source = malloc(INET6_ADDRSTRLEN);
				destination = malloc(INET6_ADDRSTRLEN);
				inet_ntop(AF_INET6, &ip->ip_src, source,
					  INET6_ADDRSTRLEN);
				inet_ntop(AF_INET6, &ip->ip_dst, destination,
					  INET6_ADDRSTRLEN);
				printf("%s -> %s\n", source, destination);
			}
		}
	}
	/* And close the session */
	pcap_close(handle);

	return (0);
}
