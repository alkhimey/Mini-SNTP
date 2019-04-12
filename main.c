/**
 * @author  Artium Nihamkin <artium@nihamkin.com>
 * @date May 2016
 *
 * @section LICENSE
 *
 * The MIT License (MIT)
 * Copyright © 2016 Artium Nihamkin, http://nihamkin.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @section DESCRIPTION
 * 
 * Test program for the ntp library. It uses UNIX sockets.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include "ntp.h"



void printf_ts(char header[], ntp_timestamp_t ts) 
{
	uint32_t y,mo,d;
	uint32_t s  = NTP_GET_TS_SECONDS_AFTER_MINUTE(ts);
	uint32_t mi = NTP_GET_TS_MINUTES_AFTER_HOUR(ts);
	uint32_t h  = NTP_GET_TS_HOURS_SINCE_MIDNIGHT(ts);
   double   ms = NTP_GET_TS_MS_AFTER_SECOND(ts); // BIG QUESTION HERE!

	ntp_get_date(ts, &y, &mo, &d);
	printf("%s\t= ", header);
	printf("%02u:%02u:%02u.%03.0f %02u/%02u/%02u\n", h, mi, s, ms, d+1, mo+1, y);
}


int main() 
{
	const char const *host   = "ae.pool.ntp.org";
	const int         port = NTP_PORT;
	int i;          
	ntp_packet_t server_msg = {0};
	ntp_packet_t client_msg = NTP_REQUEST_MSG; 

	struct sockaddr_in servaddr;
	struct hostent *hp;     /* host information */
	int fd; 

	printf("will attemt to communicate with %s\n", host);


	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket");
		exit(-1);
	}

	/* fill in the server's address and data */
	memset((char*)&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);

	/* look up the address of the server given its name */
	hp = gethostbyname(host);
	if (!hp) {
		fprintf(stderr, "could not obtain address of %s\n", host);
		exit(-1);
	}

	/* put the host's address into the server address structure */
	memcpy((void *)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length);

	printf("sending data..\n");

	/* send a message to the server */
	if (sendto(fd, &client_msg, sizeof(client_msg), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("sendto failed");
		exit(-1);
	}

	printf("recieving data..\n");

	i=recv(fd, &server_msg, sizeof(server_msg), 0);

	printf("recieved: %u out of %lu bytes \n",i, sizeof(server_msg));

	printf("LI\t\t\t= %u\n",   NTP_GET_LI  (server_msg.byte_1));
	printf("VN\t\t\t= %u\n",   NTP_GET_VN  (server_msg.byte_1));
	printf("MODE\t\t\t= %u\n", NTP_GET_MODE(server_msg.byte_1));

	if (server_msg.stratum == KISS_O_DEATH_MESAGE) {
		printf("STRATUM\t\t\t= KISS-O'-DEATH MSG\n");
	} else if (server_msg.stratum == PRIMARY_REFERENCE) {
		printf("STRATUM\t\t\t= PRIMARY REFERENCE\n");
	} else if (server_msg.stratum >= PRIMARY_REFERENCE && server_msg.stratum <= LAST_SECONDARY_REFERENCE) {
		printf("STRATUM\t\t\t= SECONDARY REFERENCE(%u\n)\n", server_msg.stratum);
	} else {
		printf("STRATUM\t\t\t= %u\n", server_msg.stratum);
	}

	printf("POLL INTERVAL\t\t= %u\n",         server_msg.poll);
	printf("PRECISION\t\t= %d\n",             server_msg.precision);
	printf("ROOT DELAY\t\t= %d\n",      ntohl(server_msg.root_delay.raw));
	printf("ROOT DISPRESION\t\t= %d\n", ntohl(server_msg.root_dispersion.raw));
	printf("REF ID\t\t\t= %.4s\n",            server_msg.reference_identifier);

	printf_ts("ORIGINATE_TIMESTAMP", server_msg.originate_timestamp);
	printf_ts("REFERENCE TIMESTAMP", server_msg.reference_timestamp);
	printf_ts("RECIEVE TIMESTAMP"  , server_msg.recieve_timestamp);
	printf_ts("TRANSMIT TIMESTAMP" , server_msg.transmit_timestamp);

	return 0;
}
