/**
 * @author  Artium Nihamkin <artium@nihamkin.com>
 * @date May 20145
 *
 * @section LICENSE
 *
 * The MIT License (MIT)
 * Copyright © 2015 Artium Nihamkin, http://nihamkin.com
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
 * This libaray is an implementation of SNTP protocol as described by 
 * [rfc4330](https://tools.ietf.org/html/rfc4330).
 *
 *
 * It provides a convinient way of constructing SNTP headers and parsing the
 * headers recieved from a time server. It does not provide the ability to 
 * transmit or recieve those message. 
 * 
 * The goal of this library is to be lightweight and portable so it can be used 
 * by different microcontrollers.
 * 
 * Please read the license. Dealing with time is a delicate buisiness. The author 
 * of this library strongly advice against using it in safety critical applications.
 */

#ifndef NTP_H_
#define NTP_H_


#ifdef __GNUC__
	#define PACKED __attribute__((packed))
#else
	#define PACKED
	#error "ntp.h: Your compiler does not support packed attribute"
#endif


typedef union  {
	struct {
		uint8_t integer;
		uint8_t fraction;
	};	
	uint32_t raw;
} ufixed_16_16_t;

typedef union {
	struct {
		int8_t  integer;
		uint8_t fraction;
	};
	int32_t raw;
} fixed_16_16_t;

typedef union  {
	struct {
		uint32_t integer;
		uint32_t fraction;
	};	
	uint64_t raw;
} ufixed_32_32_t;

typedef ufixed_32_32_t ntp_timestamp_t; // Notice: the value will be in network byte order


typedef enum  {
	NO_WARNING                 = 0,
	LAST_MINUTE_HAS_61_DECONDS = 1,
	LAST_MINUTE_HAS_59_DECONDS = 2,
	ALARM_CONDITION            = 3,
} ntp_leap_indicator_t;


typedef enum  {
	RESERVED          = 0,
	SYMMETRIC_ACTIVE  = 1,
	SYMMETRIC_PASSIVE = 2,
	CLIENT            = 3,
	SERVER            = 4,
	BROADCAST         = 5,
	RESERVED_6        = 6,
	RESERVED_7        = 7,	
} ntp_mode_t;




/*
 * This is the main NTP structure that represents an NTP message.
 * The structure is the same for every message that is being sent
 * both by the server and by the client.
 * The content of the message may differ, some fields are unused when 
 * the client sends the message.
 * 
 * Notice: values are in network byte order.
 *
 */
typedef struct PACKED {
	uint8_t         byte_1;                   // consist of multiple fields
	uint8_t         stratum;
	uint8_t         poll;
        int8_t          precision;
	fixed_16_16_t   root_delay;
	ufixed_16_16_t  root_dispersion;
	char            reference_identifier[4];  // see figure 2 in rfc4330
	ntp_timestamp_t reference_timestamp;
	ntp_timestamp_t originate_timestamp;
	ntp_timestamp_t recieve_timestamp;
	ntp_timestamp_t transmit_timestamp;
	uint32_t        key_identifier;           // not used by sntp
        uint32_t        mesage_digest[4];         // not uses by sntp
} ntp_packet_t;



#define NTP_PORT 123

/*
 * This is the message that is sent from client to server.
 *
 * It's definition is:
 *    mode - CLIENT (4)
 *    vn   - 4
 *
 * @notice Transmit timestamp is optional.
 *   
 */
#define NTP_REQUEST_MSG { .byte_1 = 0b000100011 } ; 

#define NTP_ORIGIN_YEAR 1900 /// aka epoch

#define NTP_GET_LI(b1)   ((ntp_leap_indicator_t)(((b1) & 0b11000000) >> 6)) 
#define NTP_GET_VN(b1)                          (((b1) & 0b00111000) >> 3) 
#define NTP_GET_MODE(b1) ((ntp_mode_t)          (((b1) & 0b00000111)))

#define NTP_GET_TS_SECONDS_AFTER_MINUTE(ts)  ((ntohl((ts).integer)      )  % 60)
#define NTP_GET_TS_MINUTES_AFTER_HOUR(ts)    ((ntohl((ts).integer) /  60)  % 60)
#define NTP_GET_TS_HOURS_SINCE_MIDNIGHT(ts)  ((ntohl((ts).integer) / 3600) % 24)

#define NTP_GET_TS_DAYS_SINCE_JAN_1_1900(ts) (ntohl((ts).integer) / 86400)

#define NTP_IS_LEAP_YEAR(y) ((((y)%4 == 0) && ((y)%100 != 0)) || (y)%400 == 0) 

/*
 * @brief Extracts date from the timestamp. 
 *
 * The following note from the RFC is not implemented:
 * "If bit 0 is set, the UTC time is in the range 1968-
 *  2036, and UTC time is reckoned from 0h 0m 0s UTC on 1 January
 *  1900.  If bit 0 is not set, the time is in the range 2036-2104 and
 *  UTC time is reckoned from 6h 28m 16s UTC on 7 February 2036.  Note
 *  that when calculating the correspondence, 2000 is a leap year, and
 *  leap seconds are not included in the reckoning."
 *
 * @param ts[in]     time stamp in network byte order	
 * @param year[out]  1900 to 2037, no correction if bit 0 is not set.
 * @param month[out] 0 to 11.
 * @param day[out]   0 to 30.
 */
void ntp_get_date(ntp_timestamp_t ts, uint32_t* year, uint32_t* month, uint32_t* day);


void ntp_get_date(ntp_timestamp_t ts, uint32_t* year, uint32_t* month, uint32_t* day)
{
	uint32_t days_in_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	/* initial value of day, updated throughout the computation of year and month */
	*day  = NTP_GET_TS_DAYS_SINCE_JAN_1_1900(ts);

	/* year */
	for(*year = NTP_ORIGIN_YEAR; *year < 2037; (*year)++) {

		if ( NTP_IS_LEAP_YEAR(*year)) {
			if (*day < 366) {
				break;
			} else {
				*day-= 366;
			}
		} else {
			if (*day < 365) {
				break;
			} else {
				*day-= 365;
			}
		} 	
	}

	/* month */
	if (NTP_IS_LEAP_YEAR(*year)) {
		days_in_month[1] = 29; // february, index starts at 0
	}

	for(*month =0; *month<12; (*month)++) {
		if (*day >= days_in_month[*month]) {
			*day -= days_in_month[*month];
		} else {
			break;
		}	
	}
}

#endif















