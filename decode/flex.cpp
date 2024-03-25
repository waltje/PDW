// flex.cpp
//
// This file uses the following functions:
//
//	  FLEX::FLEX()
//	  FLEX::~FLEX()
//	  int FLEX::xsumchk(long int l)
//	  void FLEX::show_addr(long int l)
//	  void FLEX::show_addr(long int l1, long int l2)
//	  void FLEX::show_phase_speed(int vt)
//	  void FLEX::showframe(int asa, int vsa)
//	  void FLEX::showblock(int blknum)
//	  void FLEX::showword(int wordnum)
//	  void FLEX::showwordhex(int wordnum)
//	  void frame_flex(char gin)
#include <windows.h>
#include "../pdw.h"
#include "../misc.h"
#include "../sound_in.h"
#include "../utils/debug.h"
#include "../utils/utils.h"
#include "decode.h"
#include "flex.h"


#define MODE_SECURE	0
#define MODE_SHORT_INST	1
#define MODE_SH_TONE	2
#define MODE_STNUM	3
#define MODE_SFNUM	4
#define MODE_ALPHA	5
#define MODE_BINARY	6
#define MODE_NUNUM	7

//#define SYNC0		0x870C
#define SYNC1		0xA6C6
#define SYNC2		0xAAAA
//#define SYNC3		0x78F3

#define EOT1		0xAAAA
#define EOT2		0xFFFF

#define min(a,b)	(((a) < (b)) ? (a) : (b))
#define max(a,b)	(((a) > (b)) ? (a) : (b))


/* Global data for module. */
int FLEX_9;				// if receiving 9-digit capcodes
int g_sps2;
int bEmpty_Frame;			// if FLEX-Frame=EMTPY / ERMES-Batch=0
int bFlexTIME_detected;			// if FlexTIME is detected
int bReflex;				// Reflex mode flag
int bFlexActive;			// mode flag
int flex_timer;				// active timer

/* Local data. */
static const char *vtype[8] = {
    "SECURE ", " INSTR ", "SH/TONE", " StNUM ",
    " SfNUM ", " ALPHA ", "BINARY ", " NuNUM "
};
static const int syncs[8] = {
    0x870C, 0x7B18, 0xB068, 0xDEA0,
#if 0
    0x22B4, 0xE9C4, 0x4C7C, 0x34DF
#else
    0x0000, 0x0000, 0x0000, 0x4C7C
#endif
};
static FLEX phase_A, phase_B, phase_C, phase_D;	// phases
static int flex_blk;
static int flex_bc;
static long capcode;
static int FlexTempAddress;		// set to corresponding groupaddress (0-15)
static int bFLEX_groupmessage;		// if receiving a groupmessage (2029568-2029583)
static int bFLEX_Frame_contains_SI;	// if frame contains Short instructions
static int bFlexTIME_not_used;		// if FlexTIME not used on this network
static SYSTEMTIME recFlexTime, recTmpTime;
static FILE *pFlexTIME = NULL;
static int flex_speed = STAT_FLEX1600;
static int g_sps = 1600;
static int level = 2;
static char phase;


FLEX::FLEX()
{
}


FLEX::~FLEX()
{
}


// Reset routine called when changing data mode or if
// switching between soundcard & serial port input.
void
flex_reset(void)
{
    flex_blk = 0;
    flex_bc = 0;
    flex_timer = 0;
    bReflex = FALSE;
    bFlexActive = FALSE;
}


void
flex_init(void)
{
    // tag each phase
    phase_A.ppp = 'A';
    phase_B.ppp = 'B';
    phase_C.ppp = 'C';
    phase_D.ppp = 'D';

    bFlexTIME_detected = FALSE;
    bFlexTIME_not_used = FALSE;
    FLEX_9 = 0;
    flex_speed = STAT_FLEX1600;
    g_sps = 1600;
    g_sps2 = 1600;
    level = 2;

    flex_reset();
}


// checksum check for BIW and vector type words
// returns: 0 if word passes test; 1 if test failed
int
FLEX::xsumchk(long l)
{
    int xs;

    // was word already marked as bad?
    if (l > 0x3fffffL)
	return 1;

    // 4 bit checksum is made by summing up remaining part of word
    // in 4 bit increments, and taking the 4 lsb and complementing them.
    // Therefore: if we add up the whole word in 4 bit chunks the 4 lsb
    // bits had better come out to be 0x0f
    xs = (int)(l & 0x0f);
    xs += (int)((l >> 4) & 0x0f);
    xs += (int)((l >> 8) & 0x0f);
    xs += (int)((l >>12) & 0x0f);
    xs += (int)((l >>16) & 0x0f);
    xs += (int)((l >>20) & 0x01);
    xs &= 0x0f;

    if (xs == 0x0f) {
	CountBiterrors(0);
	return 0;
    }

    CountBiterrors(1);
    return 1;
}


// converts a short flex address to a CAPCODE; shows it on screen
void
FLEX::show_address(long l, long l2, int bLongAddress)
{
    int len = bLongAddress ? FILTER_CAPCODE_LEN : FILTER_CAPCODE_LEN-2;

    if (! bLongAddress) {
	capcode = (l & 0x1fffffL) - 32768L;

	if (FLEX_9)
		FLEX_9--;
    } else {	
	// to get capcode: take second word, invert it...
	capcode = (l2 & 0x1fffffL) ^ 0x1fffffL;

	// multiply by 32768
	capcode <<= 15;

	// add in 2068480 and first word
	// NOTE : in the patent for FLEX, the number given was 2067456...
	//			 which is apparently not correct
	capcode = capcode + 2068480L + (l & 0x1fffffL);

	if (FLEX_9 < 91)
		FLEX_9 += 10;
    }

    // capcode is bad if it was derived from a word with uncorrectable
    // errors or if it is less than zero
    if ((l > 0x3fffffL) || (l2 > 0x3fffffL) || (capcode < 0)) {
	strcpy(Current_MSG[MSG_CAPCODE], bLongAddress ? "?????????" : "???????");
	capcode = 9999999;

	CountBiterrors(5);
    } else {
	// OK here!
	sprintf(Current_MSG[MSG_CAPCODE], bLongAddress ? "%09li" : "%07li", capcode);
	CountBiterrors(0);
    }

    if (Profile.convert_si && (capcode >= 2029568) && (capcode <= 2029583))
	 bFLEX_groupmessage = TRUE;
    else
	bFLEX_groupmessage = FALSE;

    /* Show Capcode */
    messageitems_colors[1] = COLOR_ADDRESS;

    /* Show Time/Date */
    Get_Date_Time();
    strcpy(Current_MSG[MSG_TIME], szCurrentTime);
    strcpy(Current_MSG[MSG_DATE], szCurrentDate);
    messageitems_colors[2] = COLOR_TIMESTAMP;
    messageitems_colors[3] = COLOR_TIMESTAMP;
}


/**************************/
// Return current bit rate based on flex_speed.
void
FLEX::show_phase_speed(int vt)
{
    int v;

    switch (flex_speed) {	// Add Bit Rate
	default:
	case STAT_FLEX1600:
		v = 1600;
		break;

	case STAT_FLEX3200:
		v = 3200;
		break;

	case STAT_FLEX6400:
		v = 6400;
		break;
    }

    /* Show FLEX-# */
    messageitems_colors[4] = COLOR_MODETYPEBIT;
    sprintf(Current_MSG[MSG_MODE], "FLEX-%c", phase);

    /* Show Type */
    messageitems_colors[5] = COLOR_MODETYPEBIT;

    if (vt == MODE_SHORT_INST && Profile.convert_si) {
	// Add "GROUP" instead of "INSTR".
	strcpy(Current_MSG[MSG_TYPE], " GROUP ");
    } else {
	// Add flex format.
	strcpy(Current_MSG[MSG_TYPE], vtype[vt]);
    }

    /* Show Bitrate */
    messageitems_colors[6] = COLOR_MODETYPEBIT;
    sprintf(Current_MSG[MSG_BITRATE], "%d", v);
}


#if 0
void
FLEX::CheckFlexTime(void)
{
    SYSTEMTIME recSystemTime;
    time_t lPCTime, lFlexTime;

    GetLocalTime(&recSystemTime);
    lPCTime = recSystemTime.wHour * 3600 + recSystemTime.wMinute * 60 + recSystemTime.wSecond;
    lFlexTime = recFlexTime.wHour * 3600 + recFlexTime.wMinute * 60 + recFlexTime.wSecond;
    lPCTime = abs(lPCTime - lFlexTime);

    if(recFlexTime.wYear != recSystemTime.wYear || recFlexTime.wMonth != recSystemTime.wMonth || recFlexTime.wDay != recSystemTime.wDay || lPCTime > 2) {
	OUTPUTDEBUGMSG((("TIME OUT OF SYNC! (%d seconds) \n"), lPCTime));		
	SetLocalTime(&recFlexTime);
	if (pFlexTIME)
		fwrite(" Systemtime has been corrected!", 31, 1, pFlexTIME);
    }
}
#endif


void
FLEX::FlexTIME(void)
{
    static int FLEX_time = 0, FLEX_date = 0, count = 0;
    char temp[MAX_PATH], szFlexTIME[128];
    float frame_seconds = (iCurrentFrame & 0x1f) * 1.875f;
    int seconds = frame_seconds;
    int bTime = FALSE, bDate = FALSE;
    int i;

    if (strstr(szPath, "DEBUG"))
	return;

//  OUTPUTDEBUGMSG((("Frame[0] = 0x%08X\n"), frame[0]));
//  OUTPUTDEBUGMSG((("Priority addresses %d\n"), (frame[0] >> 4) & 0x0f));
//  OUTPUTDEBUGMSG((("End Block %d\n"), (frame[0] >> 8) & 0x03));
//  OUTPUTDEBUGMSG((("Vector %d\n"), (frame[0] >> 10) & 0x07));
//  OUTPUTDEBUGMSG((("Frame Id %d\n"), (frame[0] >> 16) & 0x07));

    for (i = 0; i <= ((frame[0] >> 8) & 0x03); i++) {
	if (xsumchk(frame[i]) != 0) {
//		OUTPUTDEBUGMSG((("CRC error in BIW[%d]! (0x%08X)\n"), i, frame[i]));
		return;
	}

	if (i) {
		switch((frame[i] >> 4) & 0x07) {
			case 0:
//				OUTPUTDEBUGMSG((("frame[i]: Type == SSID/Local ID’s (i8-i0)(512) & Coverage Zones (c4-c0)(32)\n")));		
				break;

			case 1:
				frame[i] >>= 7;
				recFlexTime.wYear = (frame[i] & 0x1f) + 1994;
				frame[i] >>= 5;
				recFlexTime.wDay = frame[i] & 0x1f;
				frame[i] >>= 5;
				recFlexTime.wMonth = (frame[i] & 0x0f);
				bDate = TRUE;
				FLEX_date = 1;
//				OUTPUTDEBUGMSG((("BIW DATE: %d-%d-%d\n"), recFlexTime.wDay, recFlexTime.wMonth, recFlexTime.wYear));		
				break;

			case 2:
				frame[i] >>= 7;
				recFlexTime.wHour = frame[i] & 0x1f;
				frame[i] >>= 5;
				recFlexTime.wMinute = frame[i] & 0x3f;
				frame[i] >>= 6;
				recFlexTime.wSecond = seconds;
				bTime = TRUE;
				FLEX_time = 1;
//				OUTPUTDEBUGMSG((("BIW TIME: %02d:%02d:%02d\n"), recFlexTime.wHour, recFlexTime.wMinute, recFlexTime.wSecond));
				break;

			case 5:
//				OUTPUTDEBUGMSG((("frame[i]: Type == System Information (I9-I0. A3-A0) - related to NID roaming\n")));		
				break;

			case 7:
//				OUTPUTDEBUGMSG((("frame[i]: Type == Country Code & Traffic Management Flags (c9-c0, T3-T0)\n")));		
			break;

			case 6:
			case 3:
			case 4:
//				OUTPUTDEBUGMSG((("frame[i]: Type == Reserved\n")));		
				break;
		}
	}
    }

    if (FLEX_time && FLEX_date && !bFlexTIME_detected)
	bFlexTIME_detected = TRUE;

    if (iCurrentFrame == 0) {
	count++;

	if (count == 15 && !bFlexTIME_detected) {
		bFlexTIME_not_used = TRUE;
	} else if (Profile.FlexTIME) {
		if (bTime || bDate) {
			if (bTime) {
				GetLocalTime(&recTmpTime);
				recTmpTime.wHour = recFlexTime.wHour;
				recTmpTime.wMinute = recFlexTime.wMinute;
				recTmpTime.wSecond = recFlexTime.wSecond;
				recTmpTime.wMilliseconds = 0;
				SetLocalTime(&recTmpTime);
			}

			if (bDate) {
				GetLocalTime(&recTmpTime);
				recTmpTime.wDay = recFlexTime.wDay;
				recTmpTime.wMonth = recFlexTime.wMonth;
				recTmpTime.wYear  = recFlexTime.wYear;
				SetLocalTime(&recTmpTime);
			}
		}
	}
    }
}


// This routine is called when a complete phase of information is
// collected. First, the BIW is used to determine the length of the
// address and vector field blocks. Each address field is then
// processed according to the information in the vector field.
void
FLEX::showframe(int asa, int vsa)
{
    char szTemp[128];
    int vb, vt, tt, w1, w2, j, k, l, m, n = 0, i, c = 0;
    long cc, cc2, cc3;
    int bLongAddress = FALSE, bXsumError = FALSE;
    int iFragmentNumber, iAssignedFrame;

    // Current temporary address(bit)
    FlexTempAddress = -1;

    // make sure we start out with valid BIW
    if (xsumchk(frame[0]) == 0) {
	// run through whole address field
	for (j = asa; j < vsa; j++, c = 0, bLongAddress = FALSE, bXsumError = FALSE) {
		// Check if this can be the low part of a long address
		cc2 = frame[j] & 0x1fffffL;

		// check for long addresses (bLongAddress indicates long address)
		if (cc2 < 0x008001L)
			bLongAddress = TRUE;
		else if ((cc2 > 0x1e0000L) && (cc2 < 0x1f0001L))
			bLongAddress = TRUE;
		else if (cc2 > 0x1f7FFEL)
			bLongAddress = TRUE;


		// this is the vector word number associated with the address word j
		vb = vsa + j - asa;

		// get message vector type
		vt = (frame[vb] >> 4) & 0x07;

		if (xsumchk(frame[vb]) != 0) {
			// screwed up vector fields are not processed
			continue; 
		}

		if (Profile.FlexGroupMode && bLongAddress) {
			// Don't process long addresses if FlexGroupMode
			continue;
		}

		strcpy(szWindowText[4], "");

		switch (vt) {
			case MODE_ALPHA:
			case MODE_SECURE:
				show_address(frame[j], frame[j+1], bLongAddress);	// show address
				show_phase_speed(vt);

				// get start and stop word numbers
				w1 = frame[vb] >> 7;
				w2 = w1 >> 7;
				w1 = w1 & 0x7f;
				w2 = (w2 & 0x7f) + w1 - 1;

				// get message fragment number (bits 11 and 12) from first header word
				// if != 3 then this is a continued message
				if (! bLongAddress) {
					iFragmentNumber = (int) (frame[w1] >> 11) & 0x03;
					w1++;
				} else {
					iFragmentNumber = (int) (frame[vb+1] >> 11) & 0x03;
					w2--;
				}

				// dump all message characters onto screen
				for (k = w1; k <= w2; k++) {
					if (frame[k] > 0x3fffffL)
						display_color(&Pane1, COLOR_BITERRORS);
					else
						display_color(&Pane1, COLOR_MESSAGE);

					// skip over header info (depends on fragment number)
					if ((k > w1) || (iFragmentNumber != 0x03)) {
						c = (int)frame[k] & 0x07fl;
						if (c != 0x03) {
							display_show_char(&Pane1, c);
							hourly_char[flex_speed][STAT_ALPHA]++;
							daily_char [flex_speed][STAT_ALPHA]++;
						}
					}

					cc = (long) frame[k] >> 7;
					c = (int) cc & 0x7fl;
					if (c != 0x03) {
						display_show_char(&Pane1, c);
						hourly_char[flex_speed][STAT_ALPHA]++;
						daily_char [flex_speed][STAT_ALPHA]++;
					}

					cc = (long) frame[k] >> 14;
					c = (int) cc & 0x7fl;
					if (c != 0x03) {
						display_show_char(&Pane1, c);
						hourly_char[flex_speed][STAT_ALPHA]++;
						daily_char [flex_speed][STAT_ALPHA]++;
					}
				}

				// Change last 0 of bitrate into fragmentnumber
				if (iFragmentNumber < 3) {
					Current_MSG[MSG_BITRATE][3] = '1' + iFragmentNumber;
				}

				hourly_stat[flex_speed][STAT_ALPHA]++;
				daily_stat [flex_speed][STAT_ALPHA]++;

				break;

			case MODE_SHORT_INST:

				// RAH/PH: Short instruction for temporary address in group messaging
				if (! Profile.showinstr)
					continue;

				if (Profile.convert_si)
					bFLEX_Frame_contains_SI = TRUE;

				strcpy(szWindowText[4], "Groupcall");

				// show address
				show_address(frame[j], frame[j+1], bLongAddress);
				if (bFLEX_groupmessage)
					continue;
				show_phase_speed(vt);

				iAssignedFrame  = (frame[vb] >> 10) & 0x7f;	// Frame with groupmessage
				FlexTempAddress = (frame[vb] >> 17) & 0x7f;	// Listen to this groupcode

				if (! Profile.convert_si) {
					display_color(&Pane1, COLOR_INSTRUCTIONS);
					display_show_str(&Pane1, "TEMPORARY ADDRESS");

					// See page 90 and 107
					w1 = (frame[vb] >> 7) & 0x3;

					if (w1 == 0)
						display_show_str(&Pane1, ": ");
					else if (w1 == 7)
						display_show_str(&Pane1, " (TEST): ");
					else
						display_show_str(&Pane1, " (RESERVED): ");

					sprintf(szTemp, "%07li -> FRAME %03i", FlexTempAddress+2029568, iAssignedFrame);
					display_show_str(&Pane1, szTemp);
				}
				break;

			case MODE_STNUM:
			case MODE_SFNUM:
			case MODE_NUNUM:
				// standard / special format numeric / numbered numeric message

				if (! Profile.shownumeric)
					continue;

				show_address(frame[j], frame[j+1], bLongAddress);	// show address
				show_phase_speed(vt);

				w1 = frame[vb] >> 7;
				w2 = w1 >> 7;
				w1 = w1 & 0x7f;
				w2 = (w2 & 0x07) + w1;	// numeric message is 7 words max

				// load first message word into cc
				if (! bLongAddress) {
					cc = frame[w1];		// if short adress first message word @ w1
					w1++;
					w2++;
				} else {
					// long address - first message word in second vector field
					cc = frame[vb+1];
				}

				// skip over first 10 bits for numbered numeric, otherwise skip first 2

				if (vt == 7)
					m = 14;
				else
					m = 6;

				for (k = w1; k <= w2; k++) {
					if (cc < 0x400000L)
						display_color(&Pane1, COLOR_NUMERIC);
					else
						display_color(&Pane1, COLOR_BITERRORS);

					for (l = 0; l < 21; l++) {
						c = c >> 1;

						if ((cc & 0x01) != 0l)
							c ^= 0x08;
						cc = cc >> 1;
						m--;

						if (m == 0) {
							display_show_char(&Pane1, aNumeric[c & 0x0f]);
							hourly_char[flex_speed][STAT_NUMERIC]++;
							daily_char [flex_speed][STAT_NUMERIC]++;
							m = 4;
						}
					}
					cc = (long) frame[k];
				}
				hourly_stat[flex_speed][STAT_NUMERIC]++;
				daily_stat [flex_speed][STAT_NUMERIC]++;
				break;

			case MODE_SH_TONE:
				tt = (frame[vb] >> 7) & 0x03;  // message type
				if ((Profile.showtone && tt) || (Profile.shownumeric && !tt)) {
					show_address(frame[j], frame[j+1], bLongAddress);	// show address
					show_phase_speed(vt);

					display_color(&Pane1, COLOR_NUMERIC);

					if (tt) {
						display_show_str(&Pane1, "TONE-ONLY");
					} else {
						 // short numeric (3 or 8 numeric chars)
						for (i = 9; i <= 17; i += 4) {
							cc = (frame[vb] >> i) & 0x0f;
							display_show_char(&Pane1, aNumeric[cc]);
						}

						hourly_char[flex_speed][STAT_NUMERIC] += 3;
						daily_char [flex_speed][STAT_NUMERIC] += 3;

						if (bLongAddress) {
							// long addresses have 2 vector words...
							for (i = 0; i <= 16; i += 4) {
								cc = (frame[vb+1] >> i) & 0x0f;
								display_show_char(&Pane1, aNumeric[cc]);
							}
							hourly_char[flex_speed][STAT_NUMERIC] += 4;
							daily_char [flex_speed][STAT_NUMERIC] += 4;
						}
					}
					hourly_stat[flex_speed][STAT_NUMERIC]++;
					daily_stat [flex_speed][STAT_NUMERIC]++;
				} else
					continue;
				break;

			case MODE_BINARY:

				if (! Profile.showmisc)
					continue;

				show_address(frame[j], frame[j+1], bLongAddress);	// show address
				show_phase_speed(vt);

				w1 = frame[vb] >> 7;
				w2 = w1 >> 7;
				w1 = w1 & 0x7f;
				w2 = (w2 & 0x7f) + w1 - 1;

				if (! bLongAddress) {
					iFragmentNumber = (int) (frame[w1] >> 13) & 0x03;

					if (iFragmentNumber == 3)
						w1 += 2;
					else
						w1++;
				} else {
					iFragmentNumber = (int) (frame[vb+1] >> 13) & 0x03;

					if (iFragmentNumber == 3)
						w1++;
					w2--;
				}

				display_color(&Pane1, COLOR_MISC);

				for (k = w1, n = 0, m = 0; k <= w2; k++) {
					cc3 = frame[k];

					for (l = 0; l < 21; l++) {
						m = m >> 1;

						if ((cc3 & 0x01l) != 0)
							m = m ^ 0x08;

						cc3 = cc3 >> 1;
						n++;

						if (n == 4) {
							if (m < 10)
								display_show_char(&Pane1, 48+m);
							else
								display_show_char(&Pane1, 55+m);

							n = 0;
							m = 0;
						}
					}
				}
				break;
		}

		if (Profile.convert_si && ((vt == MODE_SHORT_INST) || bFLEX_groupmessage)) {
			if (bFLEX_groupmessage) {
				ConvertGroupcall(capcode-2029568, vtype[vt], capcode);
			} else
				AddAssignment(iAssignedFrame, FlexTempAddress, capcode);
		} else
			ShowMessage();

		// if long address then make sure we skip over both parts
		if (bLongAddress)
			j++;
	}

	// At this point, no more addresses/messages should follow, so check for missed groupcalls
	Check4_MissedGroupcalls();
    }
}


// format a received frame
void
FLEX::showblock(int blknum)
{
    static int last_frame;
    int i, j, k, err, asa, vsa;
    long cc;
    int bNoMoreData = FALSE;	// Speed up frame processing

    // format 32 bit frame into output buffer to do error correction
    for (i = 0; i < 8; i++) {
	for (j = 0; j < 32; j++) {
		k = (j*8) + i;
		ob[j] = block[k];
	}

	// do error correction
	err = ecd();
	CountBiterrors(err);

	k = (blknum << 3) + i;
	cc = 0x0000l;

	for (j = 0; j < 21; j++) {
		cc = cc >> 1;
		if (ob[j] == 0)
			cc ^= 0x100000L;
	}

	// flag uncorrectable errors
	if (err == 3)
		cc ^= 0x400000L;

	frame[k] = cc;
    }

    if ((flex_speed == STAT_FLEX1600) && ((cc == 0L) || (cc == 0x1fffffL))) {
	// Speed up frame processing
	bNoMoreData = TRUE;
    }

    // get word where vector field starts (6 bits)
    vsa = (int) ((frame[0] >> 10) & 0x3f);

    // get word where address field starts (2 bits)
    asa = (int) ((frame[0] >> 8)  & 0x03) + 1;

    if (blknum == 0) {
	if (vsa == asa) {
		// Assuming no messages in current frame.
		bEmpty_Frame = TRUE;
	} else {
		bEmpty_Frame = FALSE;
	}

	if (!bFlexTIME_detected && !bFlexTIME_not_used) {
		FlexTIME();
	} else if ((iCurrentFrame == 0) && (last_frame == 127)) {
		if (Profile.FlexTIME) {
			FlexTIME();
		}
	}
	last_frame = iCurrentFrame;
    } else if (((blknum == 10) || bNoMoreData) && !bReflex) {
	// show messages in frame if last block was processed and we're not in reflex mode
	showframe(asa, vsa);
	if (bNoMoreData)
		flex_blk = 1;
    }
}


// displays given three character word... used when displaying a
// REFLEX message where the characters are spread over multiple phases.
void
FLEX::showword(int wordnum)
{
    long cc = (long) frame[wordnum];
    int c;

    if (cc > 0x200000L)
	display_color(&Pane1, COLOR_BITERRORS);
    else
	display_color(&Pane1, COLOR_MESSAGE);

    if ((cc != 0L) && (cc != 0x1fffffL)) {
	c = (int) cc & 0x7fL;
	display_show_char(&Pane1, c);
	cc = (long) frame[wordnum] >> 7;
	c = (int) cc & 0x7fL;
	display_show_char(&Pane1, c);
	cc = (long) frame[wordnum] >> 14;
	c = (int) cc & 0x7fL;
	display_show_char(&Pane1, c);
    }
}


void
FLEX::showwordhex(int wordnum)
{
    display_show_char(&Pane1, '[');
    display_show_hex21(&Pane1, frame[wordnum]);
    display_show_char(&Pane1, ']');
}


void
frame_flex(char gin)
{
    static unsigned short slr[4] = { 0, 0, 0, 0 };
    static int cy, fr;
    static int bct, hbit;
    int cer, ihd, i, nh, j, hd = 0, reflex_flag = 0;
    double aver = 0.0;
#if 0
    // Main (1600) sync-code
    unsigned sup[4] = { 0x870C, 0xA6C6, 0xAAAA, 0x78F3 };
#endif
    char sync[64];

    // update bit buffer
    // sync up signal is 1600 BPS 2 level FSK signal
    for (i = 0; i < 3; i++) {
	slr[i] = slr[i] << 1;
	if (slr[i+1] & 0x8000)
		slr[i] |= 0x0001;
    }
    slr[3] = slr[3] << 1;

    if (gin < 2)
	slr[3] |= 0x0001;

#if 0
    {
	FILE *pTest;
	if ((pTest = fopen("test.txt", "a")) != NULL) {
		fwrite(gin < 2 ? "1" : "0", 1, 1, pTest);
		fclose(pTest);
	}
    }
#endif

    // Need sync-up, or just end of transmission?
    if (flex_blk == 0) {
	if (flex_timer) {
		// End of transmission?
		if (nOnes(slr[2] ^ EOT1) + nOnes(slr[3] ^ EOT2) == 0) {
			flex_timer = 0;
			display_showmo(MODE_IDLE);
			return;
		}
	}

	// center portion always the same
	nh = nOnes(slr[1] ^ SYNC1) + nOnes(slr[2] ^ SYNC2);
	if (nh == 32) {
		// 32 errors, so must be inverted
		if (((slr[0] ^ slr[3]) & 0xffff) == 0xffff) {
			// Invert buffer so current frame can be decoded.
			slr[0] ^= 0xffff;
			slr[3] ^= 0xffff;

			// Invert receive polarity
			InvertData();

			// 32 inverted errors => 0 errors.
			nh = 0;
		}
	}

	// NOTE: STILL MISSING SEVERAL REFLEX SYNC UPS
	// sync up with 1 or less mismatched bits out of center 32
	// AND 2 or less mismatched bits out of outside 32
	if (nh < 2) {
		// guessing we've gotten sync up...
//		nh = nOnes(slr[0] ^ slr[3]);
		flex_bc = 89;

		if (nOnes(slr[0] ^ slr[3] ^ 0xFFFF) < 2) {
			int speed;
			for (speed = 0; speed < 8; speed++) {
				if ((nOnes(slr[0] ^ syncs[speed]) + nOnes(slr[3] ^ ~syncs[speed])) < 2) {
					if ((speed & 0x03) == 0) {
						// FLEX-1600
						if (! Profile.flex_1600)
							return;
						flex_speed = STAT_FLEX1600;
					} else if ((speed & 0x03) == 0x03) {
						// FLEX-6400
						if (! Profile.flex_6400)
							return;
						flex_speed = STAT_FLEX6400;
					} else {
						// FLEX-3200
						if (! Profile.flex_3200)
							return;
						flex_speed = STAT_FLEX3200;
					}

					bFlexActive = TRUE;
					flex_timer = 20;

					g_sps = (speed & 0x01) ? 3200 : 1600;
					level = (speed & 0x02) ? 4 : 2;
					bReflex = (speed & 0x04) ? TRUE : FALSE;

					if (g_sps == 3200)
						g_sps2 = 3200;

					break;
				}
			}

			// At this point, we should have a valid FLEX-sync, but no match
			if (speed == 8) {
				// in loop above. So let's show as unknown sync header :
				display_color(&Pane1, COLOR_MISC);
				display_line(&Pane1);
				sprintf(sync, " UNKNOWN SYNC HEADER : %hX %hX %hX %hX", slr[0], slr[1], slr[2], slr[3]);
				display_show_strV2(&Pane1, sync);
				display_line(&Pane1);

				return;
			}
		} else
			return;

#if 0
		if ((nOnes(slr[0] ^ 0x870C) + nOnes(slr[3] ^ 0x78F3)) < 3) {
			// FLEX 2-level 1600
			if (Profile.flex_1600) {
				// 1000011100001100-0111100011110011
				bFlexActive = TRUE;
				flex_timer = 21;
				g_sps = 1600;
				flex_speed = STAT_FLEX1600;
				level = 2;
				bReflex = FALSE;
			} else
				return;
		} else if ((nOnes(slr[0] ^ 0xB068) + nOnes(slr[3] ^ 0x4F97)) < 3) {
			// FLEX 4-level 3200
			if (Profile.flex_3200) {
				// 1011000001101000-0100111110010111
				bFlexActive = TRUE;
				flex_timer = 20;  // 2 seconds
				g_sps = 1600;
				flex_speed = STAT_FLEX3200;
				level = 4;
				bReflex = FALSE;
			} else
				return;
		} else if ((nOnes(slr[0] ^ 0xDEA0) + nOnes(slr[3] ^ 0x215F)) < 3) {
			// FLEX 4-level 6400
			if (Profile.flex_6400) {
				// 1101111010100000-0010000101011111
				bFlexActive = TRUE;
				flex_timer = 20;  // 2 seconds
				g_sps = 3200;
				g_sps2 = 3200;
				flex_speed = STAT_FLEX6400;
				level = 4;
				bReflex = FALSE;
			} else
				return;
		} else if ((nOnes(slr[0] ^ 0x7B18) + nOnes(slr[3] ^ 0x84E7)) < 3) {
			// FLEX 2-level 3200
			if (Profile.flex_3200) {
				// 0111101100011000-1000010011100111
				bFlexActive = TRUE;
				flex_timer = 20;	// 2 seconds
				g_sps = 3200;
				g_sps2 = 3200;
				flex_speed = STAT_FLEX3200;
				level = 2;
				bReflex = FALSE;
			} else
				return;
		} else if ((nOnes(slr[0] ^ 0x4C7C) + nOnes(slr[3] ^ 0xB383)) < 3) {
			// REFLEX 4-level 6400
			if (Profile.flex_3200) {
				// 0100110001111100-1011001110000011
				bFlexActive = TRUE;
				flex_timer = 20;  // 2 seconds
				g_sps = 3200;
				g_sps2 = 3200;
				flex_speed = STAT_FLEX6400;
				level = 4;
				bReflex = TRUE;
			} else
				return;
		} else if (((slr[0] ^ slr[3]) & 0xFFFF) == 0xFFFF) {
			bFlexActive = TRUE;
			flex_timer = 20;  // 2 seconds
			g_sps = 3200;
			g_sps2 = 3200;
			flex_speed = STAT_FLEX3200;
			level = 2;
			bReflex = TRUE;

			display_color(&Pane1, COLOR_MISC);
			display_line(&Pane1);
			sprintf(sync, " UNKNOWN SYNC HEADER : %X %X %X %X", slr[0], slr[1], slr[2], slr[3]);
			display_show_strV2(&Pane1, sync);
			display_line(&Pane1);
			display_line(&Pane1);
		} else
			return;
#endif

		// FINE DIDDLE RCV CLOCK RIGHT HERE****************************
		// idea - take average error over last 64 bits; subtract it off
		// this allows us to go to slower main rcv loop clock response

		// get average rcv clock error over last 64 bits
		for (j = 0; j < 64; j++)
			aver = aver + rcver[j];
		aver *= 0.015625;

		// divide by two - just for the heck of it
		aver *= 0.5;
		exc += aver;

		// go to slower main rcv loop clock
		rcv_clkt = rcv_clkt_fl;
	}

	if (flex_bc > 0) {
		flex_bc--;

		// pick off cycle info word (bit numbers 71 to 40 in sync holdoff)
		if ((flex_bc < 72) && (flex_bc > 39)) {
			if (gin < 2)
				ob[71-flex_bc] = 1;
			else
				ob[71-flex_bc] = 0;
		} else if (flex_bc == 39) {
			if (g_sps2 == 3200) {
				// Tell soundcard routines we are changing
				cross_over = 1;		// from 1600 to 3200 baud rate, and
				BaudRate = 3200;	// also need to skip some sync-up-data.
			}

			// process cycle info word when its been collected

			// do error correction
			cer = ecd();
			if (cer < 2) {
				for (ihd = 4; ihd < 8; ihd++) {
					hd >>= 1;
					if (ob[ihd] == 1)
						hd ^= 0x08;
				}
				cy = (hd & 0x0f) ^ 0x0f;

				for (ihd = 8; ihd <= 14; ihd++) {
					hd >>= 1;
					if (ob[ihd] == 1)
						hd ^= 0x40;
				}
				fr = (hd & 0x7f) ^ 0x7f;
			}
			display_cfstatus(cy, fr);

			bFLEX_Frame_contains_SI = FALSE;

			if (bFlexActive) {
				if (bReflex)
					reflex_flag = 0x10;
				
				if (level == 2) {
					if (g_sps == 1600) {
						display_showmo(MODE_FLEX_A + reflex_flag);
					} else if (g_sps == 3200) {
						display_showmo(MODE_FLEX_A + MODE_FLEX_C + reflex_flag);
					}
				} else if (level == 4) {
					if (g_sps == 1600) {
						display_showmo(MODE_FLEX_A + MODE_FLEX_B + reflex_flag);
					} else if (g_sps == 3200) {
						display_showmo(MODE_FLEX_A + MODE_FLEX_B + MODE_FLEX_C + MODE_FLEX_D + reflex_flag);
					}
				}
			}
		}

		if (flex_bc == 0) {
			flex_blk = 11;
			bct = 0;
			hbit = 0;

			// at this point data rate could become either 1600 or 3200 SPS
			if (g_sps == 1600) {
				ct_bit = ct1600; // ct_bit = serial port baudrate
				BaudRate = 1600; // BaudRate = soundcard baudrate
			} else {
				// Note we don't change baud rate for soundcard here,
				// as we have already changed baud rate. 
				ct_bit = ct3200;
			}
			rcv_clkt = rcv_clkt_hi;	// loosen up rcv loop clock constant again
		}
	}
    } else {
	// update phases depending on transmission speed
	if (g_sps == 1600) {
		// always have PHASE A
		if (gin < 2)
			phase_A.block[bct] = 1;
		else
			phase_A.block[bct] = 0;

		// if 4 level FSK - do PHASE B also
		if (level == 4) {
			if ((gin == 0) || (gin == 3))
				phase_B.block[bct] = 1;
			else
				phase_B.block[bct] = 0;
		}
		bct++;
	} else {
		// split out bits
		if (hbit == 0) {
			if (gin < 2)
				phase_A.block[bct] = 1;
			else
				phase_A.block[bct] = 0;

			if (level == 4) {
				if ((gin == 0) || (gin == 3))
					phase_B.block[bct] = 1;
				else
					phase_B.block[bct] = 0;
			}
			hbit++;
		} else {
			if (gin < 2)
				phase_C.block[bct] = 1;
			else
				phase_C.block[bct] = 0;

			if (level == 4) {
				if ((gin == 0) || (gin == 3))
					phase_D.block[bct] = 1;
				else
					phase_D.block[bct] = 0;
			}

			hbit = 0;
			bct++;
		}
	}

	if (bct == 256) {
		// also pass on block # (0 - 10)
		bct = 0;

		phase = 'A';
		phase_A.showblock(11 - flex_blk);

		if (level == 4) {
			phase = 'B';
			phase_B.showblock(11 - flex_blk);
		}

		if (g_sps == 3200) {
			phase = 'C';
			phase_C.showblock(11 - flex_blk);

			if (level == 4) {
				phase = 'D';
				phase_D.showblock(11 - flex_blk);
			}
		}

		flex_blk--;
		if (flex_blk == 0) {
			// if finished set speed back to 1600 sps
			ct_bit = ct1600;  // ct_bit = serial port baudrate
			BaudRate = 1600;  // BaudRate = soundcard baudrate

			bFlexActive = FALSE;

			// if in reflex mode: display raw message if BIW != 0x1fffff
			if (bReflex && (phase_A.frame[0] != 0x1fffffL)) {
				phase_A.showwordhex(0);

				for (i = 0; i < 88; i++) {
					phase_A.showword(i);
					phase_B.showword(i);
					phase_C.showword(i);
					phase_D.showword(i);
				}
				ShowMessage();
			}
		}
	}
    }
}


void
display_cfstatus(int cycle, int frame)
{
    static int oldframe = -1;

    if (oldframe == frame) {
	if (Profile.convert_si && bFLEX_Frame_contains_SI) {
		if (frame++ == 128) {
			frame = 0;
			if (cycle++ == 15)
				cycle = 0;
		}
	}
    }
    oldframe = frame;

    if (cycle == 15) {
	// Sometimes PDW seems to display cycle "15", which
	iCurrentCycle = 99;

	// does not exist, so let's display 99/999
	iCurrentFrame = 999;

	CountBiterrors(5);
    } else {
	iCurrentCycle = cycle;
	iCurrentFrame = frame;
    }
}
