#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "../pdw.h"
#include "../misc.h"
#include "../gfx.h"
#include "../utils/utils.h"
#include "decode.h"
#include "pocsag.h"


#define TYPE_TONE_ONLY	0x01
#define TYPE_NUMERIC	0x02
#define TYPE_ALPHA	0x04

#define MSB		0


POCSAG	pocsag;
int	pocsag_baud_rate,
	pocbit;

static int mode;
static int iType;		// Current Message type (ALPHA / NUMERIC)
static int iNumScore = 0;	// Contains possible numeric percentage
static int iAlpScore = 0;	// Contains possible alphanumeric percentage

static unsigned int sr = 0;

#define REST_ALPHA_BITS_LEN	6
static char szRestAlphaBits[REST_ALPHA_BITS_LEN] = "";

static BYTE message_color_alp[MAX_STR_LEN];		// buffer for message colors (ALPHA)
static BYTE message_color_num[MAX_STR_LEN];		// buffer for message colors (NUMERIC)

// general purpose buffers for text conversions
static char ptr1[LINE_SIZE] = "";
static char ptr2[LINE_SIZE] = "";


POCSAG::POCSAG()
{
    wordc = 0;
    nalp = 0;
    nnum = 0;
    srcn = 0;
    srca = 0;
    bAddressWord = FALSE;
    pocsag_baud_rate = STAT_POCSAG1200;
}


POCSAG::~POCSAG()
{
}


// reset in preperation of next message
void
POCSAG::reset(void)
{
    // have we just shown a message? then do crlf; flush message if it passed filtering test...

    iType = 0;
    srcn = 0;
    srca = 0;
    sr = 0;
    nalp = 0;
    nnum = 0;
    wordc = 0;
    bAddressWord = FALSE;
}


void
POCSAG::frame(int bit)
{
    static unsigned iWordNumber = 0, cc, sr0 = 0, sr1 = 0;
    static int bSynced = FALSE;
//  unsigned fs0 = 0x7CD2, fs1 = 0x15D8;
    int nh;

    // update register
    sr0 <<= 1;
    if ((sr1 & 0x8000) != 0)
	sr0 ^= 0x01;
    sr1 <<= 1;

    if (bit == 1) {
	sr1 = sr1 ^ 0x01;
    } else if (bit == -1) {
	// reset
	bSynced = FALSE;

	// reset in preperation of next message
	reset();

	// just in case we finish a message, don't get any more address
	// codewords, and drop out of POCSAG mode...
	sr0 = 0;
	sr1 = 0;
    }

    if (! bSynced) {
	// find pocsag sync sequence - sync up if there are less than 3 mismatched bits
	nh = nOnes(sr0 ^ 0x7CD2) + nOnes(sr1 ^ 0x15D8);

	if (nh < 5) {
		bSynced = TRUE;

		iWordNumber = 0;
		cc = 0;
	} else if (nh == 32) {
		// 32 errors, so must be inverted
		InvertData();	// Invert receive polarity

		bSynced = TRUE;

		iWordNumber = 0;
		cc = 0;
	}
    } else {
	// format, process 16 by 32 bit paging block
	ob[cc] = bit;
	cc++;

	if (cc == 32) {
		// find idle codeword; strip it out
		nh = nOnes(sr0 ^ 0x7A89) + nOnes(sr1 ^ 0xC197);

		if (nh < 5) {
			// keep from switching back to flex if idle
			pocbit = 170;

			// at this point it is possible we still have a
			// short message in that we haven't even begun to
			// show on screen yet...
			// This will be true for all messages with wordc < 8.
			// Next line finds them and flushes them out of
			// the woodwork
			if (bAddressWord)
				show_message();

			// IDLE means message is terminated
			reset();
		} else {
			// dump one block through with word position
			// relative to sync word (determines frame
			// number which is the 3 lsb bits of POCSAG capcode).
			process_word(iWordNumber);
		}

		// Number of Words
		iWordNumber++;
		cc = 0;
	}

	if (iWordNumber == 16) {
		// if block count is zero go back to look for sync word
		bSynced = FALSE;
	}
    }
}


void
POCSAG::process_word(int fn2)
{
    static unsigned du;
    int restbits, startbit;
    int i, errl;

    // run error correcting routine
    errl = ecd();
    if (errl < 2)
	pocbit = 170;

    if (ob[MSB] == 1) {
	// MSB=1 means message
	for (i = 1; i <= 20; i++) {
		sr >>= 1;
		if (ob[i] == 1)
			sr += 0x40;

		if (srca++ == 6) {
		        // store alpha char (7 bits)
			if (errl > 2) {
				// keep error correcting info also
//				sr ^= 0x1000;
				message_color_alp[nalp] = COLOR_BITERRORS;
			} else
				message_color_alp[nalp] = COLOR_MESSAGE;

				alp[nalp++] = sr;

				srca = 0;
			}

			if (wordc < 7) {
				if (srcn++ == 3) {
					// store numeric char (4 bits)
					du = (sr >> 3) & 0x0f;

					if (errl > 2) {
						message_color_num[nnum] = COLOR_BITERRORS;
					} else
						message_color_num[nnum] = COLOR_NUMERIC;

					num[nnum++] = aNumeric[du];

					srcn = 0;
				}
			}
		}

		wordc++;

		restbits = (20*wordc) % REST_ALPHA_BITS_LEN;
		startbit = 21-restbits;
		strncpy(szRestAlphaBits, &ob[startbit], restbits);
		szRestAlphaBits[restbits] = '\0';
	} else {
		// MSB bit = 0 means address
		// message with more than a word in them are OK; If we had a Tone Only
		// address then wordc is zero but lwad would be set - so we call show_short
		// to display the tone only address. Previously this routine was always
		// called when wordc was zero which made the "fake" tone only display
		if (bAddressWord)
			show_message();

		// reset in preperation of next message
		reset();

		// the way I have it, MSB of address is bit 1; address info in the word is 18 bits;
		pocaddr = 0L;

		for (i = 18; i > 0; i--) {
		        // get 18 MSB bits of address
			pocaddr = pocaddr >> 1;
			if (ob[i] == 1)
				pocaddr ^= 0x100000L;
		}

		// to complete the capcode, we put the frame number into the 3 lsb bits
		pocaddr += (long) ((fn2 >> 1) & 0x07);

		// tag capcode as bad if uncorrectable error in address word
		if (errl > 2)
			pocaddr ^= 0x400000L;

		// get function number --- unfortunately doesn't seem
		// to tell you whether message is alpha or numeric
		function = ((ob[19] << 1) | ob[20]) + 1;

		// We have an address word
		bAddressWord = TRUE;
	}
}


// show POCSAG address in decimal along with function number
void
POCSAG::show_addr(int bAlpha)
{
    int speed, baud;

    switch (pocsag_baud_rate) {
	case STAT_POCSAG512:
		speed = 512;
		baud = 3;
		mode = MODE_P512;
		break;

	case STAT_POCSAG2400:
		speed = 2400;
		baud = 1;
		mode = MODE_P2400;
		break;

	default:
		case STAT_POCSAG1200:
		speed = 1200;
		baud = 2;
		mode = MODE_P1200;
		break;
    }

    pocaddr = pocaddr & 0x1fffffL;

    if (pocaddr > 0x3fffffL) {
	// If error in capcode don`t display it.
	strcpy(Current_MSG[MSG_CAPCODE], "???????");
	function = 0;
	CountBiterrors(5);
    } else {
	// Add capcode
	sprintf(Current_MSG[MSG_CAPCODE], "%07li", pocaddr);
	CountBiterrors(0);
    }

    /* Show Capcode */
    messageitems_colors[1] = COLOR_ADDRESS;

    /* Show Time / Date */
    Get_Date_Time();
    strcpy(Current_MSG[MSG_TIME], szCurrentTime);
    strcpy(Current_MSG[MSG_DATE], szCurrentDate);
    messageitems_colors[2] = COLOR_TIMESTAMP;
    messageitems_colors[3] = COLOR_TIMESTAMP;

    /* Show POCSAG-# */
    sprintf(Current_MSG[MSG_MODE], "POCSAG-%1i",function);
    messageitems_colors[4] = COLOR_MODETYPEBIT;

    /* Show message type */
    sprintf(Current_MSG[MSG_TYPE], bAlpha ? " ALPHA " : "NUMERIC");
    messageitems_colors[5] = COLOR_MODETYPEBIT;

    sprintf(Current_MSG[MSG_BITRATE], "%4d", speed);
    messageitems_colors[6] = COLOR_MODETYPEBIT;
}


int
POCSAG::GetMessageType(void)
{
    static const char *szSpaces[7] = {
	"0201804", "2018040", "0180402", "1804020",
	"8040201", "0402018", "4020180"
    };
    int nBadnum = 0, nBadalp = 0, nBadnumchars[6] = { 1, 1, 1, 1, 1, 1 };
    int i;

    iType = 0;
    iNumScore = 0;
    iAlpScore = 0;

    for (i = nnum-1; i >= 0; i--) {
	if ((num[i] == ' ') && (i == nnum-1)) {
		// Skip/remove spaces at end of numeric message
		nnum--;
	} else
		break;
    }

    for (i = nalp-1; i >= 0; i--) {
	if (i == nalp-1) {
		if ((alp[i] == 0)  ||	// NULL
		    (alp[i] == 3)  ||	// ETX	- End Of Text
		    (alp[i] == 4)  ||	// EOT	- End Of Transmission
		    (alp[i] == 10) ||   // LINEFEED
		    (alp[i] == 13) ||   // Carriage Return
		    (alp[i] == 23) ||	// ETB	- End of transmission Block
		    (alp[i] == 25)) {	// EM	- End of Medium
			// Skip/remove non-printable ASCII at end of message
			nalp--;
		}
	} else
		break;
    }

    if (wordc < 7) {
	// If we have less than 7 messagewords
//	restbits = (20*wordc) % 7;
//	startbit = 21-restbits;
//	strncpy(szRestAlphaBits, &ob[startbit], restbits);
//	szRestAlphaBits[restbits] = '\0';
//	int test = szRestAlphaBits[0] + szRestAlphaBits[1] + szRestAlphaBits[2] + szRestAlphaBits[3] + szRestAlphaBits[4] + szRestAlphaBits[5] + szRestAlphaBits[6];
	if (strchr(szRestAlphaBits, char(1)))
//	if (strstr(szRestAlphaBits, "1")) 
	{
		// Last (wordc % 7) bits != 0, so this is Numeric
		return TYPE_NUMERIC;
	}
    } else {
	// More than 6 messagewords, must be alphanumeric
	return TYPE_ALPHA;
    }

    if (Profile.pocsag_fnu) {
	return (function == 4) ? TYPE_ALPHA : TYPE_NUMERIC;
    }

    // Store bits as numeric characters in array num[]
    // Penalize "bad" numeric characters 'U','[',']','*','-' and ' '
    // The characters stored in array num[] have already the correct ASCII format
    for (i = 0; i < nnum; i++) {
	// penalize for "bad" characters
	if (num[i] == '[') {
		nBadnum += (20 * nBadnumchars[0]++);
	} else if (num[i] == ']') {
		nBadnum += (20 * nBadnumchars[1]++);
	} else if (num[i] == 'U') {
		// Only if 'U' isn't the first character
		if (i)
			nBadnum += (50  *nBadnumchars[2]++);
		else
			nBadnumchars[2]++;
	} else if (num[i] == '*') {
		nBadnum += (200 * nBadnumchars[3]++);
	} else if (num[i] == '-') {
		nBadnum += (10 * nBadnumchars[4]++);
	} else if (num[i] == ' ') {
		// Only "1 2345" and "12 345" don't get penalized
		if (((i == 1) && (num[0] > '0')  && (num[0] <= '9')) ||
		    ((i == 2) && (num[0] > '0')  && (num[0] <= '3') && (num[1] >= '0') && (num[1] <= '9')) ||
		    ((i == 2) && (num[0] == '3') && (num[1] >= '0') && (num[1] <= '2'))) {
			nBadnumchars[5]++;
		} else
			nBadnum+=(10*nBadnumchars[5]++);
	}

	if ((nnum - i) >= 7) {
		// Also penalize string "0201804",
		// as they could be alphanumeric spaces
		for (int j = 0; j < 7; j++) {
			if ((num[i] == szSpaces[j][0]) && (num[i+1] == szSpaces[j][1]) &&
			    (num[i+2] == szSpaces[j][2]) && (num[i+3] == szSpaces[j][3]) &&
			    (num[i+4] == szSpaces[j][4]) && (num[i+5] == szSpaces[j][5]) &&
			    (num[i+6] == szSpaces[j][6])) {
				nBadnum += 25;
			}
		}
	}
    }

    if ((nnum > 0) && (nBadnum >= 0)) {
	iNumScore = 100 - (nBadnum / nnum);
	if (iNumScore < 0)
		iNumScore = 0;
    } else
	iNumScore = 0;

    // Store bits as alphanumeric characters in array alp[]
    // and penalize "bad" alphanumeric characters
    for (i = 0; i < nalp; i++) {
	if (((alp[i] > 122) && (alp[i] < 127)) || (alp[i] == 96)) {
		nBadalp += (100 / nalp);	// {|}~ `
	} else if (((alp[i] > 90) && (alp[i] < 96)) || (alp[i] == 10)) {
		nBadalp += (50 / nalp);		// [\]^_ || <linefeed>
	} else if ((alp[i] > 57) && (alp[i] < 65)) {
		nBadalp += (50 / nalp);		//:;<=>?@
	} else if ((alp[i] > 34) && (alp[i] < 45)) {
		nBadalp += (50 / nalp);		// #$%&'()*+,
 	} else if ((alp[i] > 32) && (alp[i] < 35)) {
		nBadalp += (10 / nalp);		// !"
 	} else if ((alp[i] < 32) || (alp[i] == 127)) {
		nBadalp += (1000 / nalp);	// Non printable characters
	}
    }

    if ((nalp > 0) && (nBadalp >= 0)) {
	iAlpScore = 100 - (nBadalp / nalp);
	if (iAlpScore < 0)
		iAlpScore = 0;
    } else
	iAlpScore = 0;

    // Both less than 50% -> Bad decoded?
    if ((iNumScore < 50) && (iAlpScore < 50)) {
		// Let's not display this message
		return 0;
    }

    // If character scores are equal
    if (iNumScore == iAlpScore) {	
	if (Profile.pocsag_showboth) {
		bDoubleDisplay = true;
		return TYPE_ALPHA + TYPE_NUMERIC;
	}
    }

    // If character scores are not equal or if iNumScore > iAlpScore,
    if (!nBadnum || (iNumScore > iAlpScore)) {
	// If no bad numeric characters,
	return TYPE_NUMERIC;	// guess NUMERIC
    } else if (!nBadalp || (iAlpScore > iNumScore)) {
	// If no bad alpha characters, or if iAlpScore > iNumScore,
	return TYPE_ALPHA;	// guess ALPHA
    }

    // IF we ever reach this point, don't display anything
    return 0;
}


void
POCSAG::show_message()
{
    int i;

    if (! wordc) {
	// If no MSG-words => Tone-Only
	iType = TYPE_TONE_ONLY;
    } else
	iType = GetMessageType();

    if (iType & TYPE_ALPHA) {
	show_addr(TRUE);

	for (i = 0; i < nalp; i++) {
		// print out stored words
		display_color(&Pane1, message_color_alp[i]);
		display_show_char(&Pane1, alp[i]);
		hourly_char[pocsag_baud_rate][STAT_ALPHA]++;
		daily_char[pocsag_baud_rate][STAT_ALPHA]++;
	}
	hourly_stat[pocsag_baud_rate][STAT_ALPHA]++;
	daily_stat[pocsag_baud_rate][STAT_ALPHA]++;

	ShowMessage();
    }

    if ((iType == TYPE_TONE_ONLY) || (iType & TYPE_NUMERIC)) {
	if ((iType == TYPE_TONE_ONLY) && !Profile.showtone)
		return;
	if ((iType & TYPE_NUMERIC) && !Profile.shownumeric)
		return;

	show_addr(FALSE);

	display_color(&Pane1, COLOR_NUMERIC);

	hourly_stat[pocsag_baud_rate][STAT_NUMERIC]++;
	daily_stat[pocsag_baud_rate][STAT_NUMERIC]++;

	if (iType == TYPE_TONE_ONLY) {
		display_show_str(&Pane1, "TONE ONLY");
		ShowMessage();
		return;
	}

	for (i = 0; i < nnum; i++) {
		display_color(&Pane1, message_color_num[i]);
		display_show_char(&Pane1, num[i]);
		hourly_char[pocsag_baud_rate][STAT_NUMERIC]++;
		daily_char[pocsag_baud_rate][STAT_NUMERIC]++;
	}

	ShowMessage();
    }

    if (bDoubleDisplay)
	bDoubleDisplay = FALSE;
}
