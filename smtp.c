/*
 *	SMTP routines for mailsend - a simple mail sender via SMTP
 *
 */
#include <windows.h>
#include <stdio.h>
#ifdef USE_SSL
# include <openssl/ssl.h>
# include <openssl/err.h>
#endif
#include "utils/debug.h"
#include "pdw.h"
#include "smtp_int.h"
#include "smtp.h"


#define MY_BUFF_SIZE	1024

// how long client will wait for server response in non-blocking mode
#define TIME_IN_SEC	3*60

#define SEND_RECIEVE_TO 5*60


int nSMTPsessions = 0;
int nSMTPemails = 0;
int nSMTPerrors = 0;
int iSMTPlastError = 0;


static SOCKET smtp_socket = INVALID_SOCKET;
static char buf[MY_BUFF_SIZE];

static HANDLE MailThread;
static THEMAIL mail;
static int nMaxLen;
static BOOL keepbusy = TRUE;
static BOOL bWsaStartup;

#define MAX_MAIL	100
#define MAX_MAIL_LEN	1024

static char szMailBuffer[MAX_MAIL][MAX_MAIL_LEN];
static int  nBufferdMailStart;
static int  nBufferdMailEnd;
static BYTE dtable[256];

#ifdef USE_SSL
static SSL_CTX *m_ctx;
static SSL *m_ssl;
#endif


const char *szSmtpCharSets[] = {
    "us-ascii     (Standard)",
    "iso-8859-1   (West European)",
    "iso-8859-2   (East European)",
    "iso-8859-3   (South European)", 
    "iso-8859-4   (North European)",
    "iso-8859-5   (Cyrillic)",
    "iso-8859-6   (Arabic)", 
    "iso-8859-7   (Greek)", 
    "iso-8859-8   (Hebrew)",
    "iso-8859-9   (Turkish)",
    "iso-8859-10  (Nordic)",
    "iso-2022-kr  (Korean)",
    "KOI8-R       (Russian)",
    "EUC-KR       (Korean)",
    "Shift_JIS    (Japanese)",
    "ISO-2022-JP  (Japanese)",
    "EUC-JP       (Japanese)",
    "GB2312       (Chinese)",
    "Big5         (Traditional Chinese)",
    "windows-1250 (Central Europ Windows)",
    "windows-1251 (Cyrillic Windows)",
    "windows-1252 (Western Europ Windows)",
    "windows-1253 (Greek Windows)",
    "windows-1254 (Turkish (Windows)",
    "windows-1255 (Hebrew Windows)",
    "windows-1256 (Arabic Windows)",
    "windows-1257 (Baltic Windows)",
    "windows-1258 (Vietnamese Windows)"
};


#ifdef USE_SSL
typedef enum {
    CSMTP_NO_ERROR = 0,
    WSA_STARTUP = 100, // WSAGetLastError()
    WSA_VER,
    WSA_SEND,
    WSA_RECV,
    WSA_CONNECT,
    WSA_GETHOSTBY_NAME_ADDR,
    WSA_INVALID_SOCKET,
    WSA_HOSTNAME,
    WSA_IOCTLSOCKET,
    WSA_SELECT,
    BAD_IPV4_ADDR,
    UNDEF_MSG_HEADER = 200,
    UNDEF_MAIL_FROM,
    UNDEF_SUBJECT,
    UNDEF_RECIPIENTS,
    UNDEF_LOGIN,
    UNDEF_PASSWORD,
    BAD_LOGIN_PASSWORD,
    BAD_DIGEST_RESPONSE,
    BAD_SERVER_NAME,
    UNDEF_RECIPIENT_MAIL,
    COMMAND_MAIL_FROM = 300,
    COMMAND_EHLO,
    COMMAND_AUTH_PLAIN,
    COMMAND_AUTH_LOGIN,
    COMMAND_AUTH_CRAMMD5,
    COMMAND_AUTH_DIGESTMD5,
    COMMAND_DIGESTMD5,
    COMMAND_DATA,
    COMMAND_QUIT,
    COMMAND_RCPT_TO,
    MSG_BODY_ERROR,
    CONNECTION_CLOSED = 400, // by server
    SERVER_NOT_READY, // remote server
    SERVER_NOT_RESPONDING,
    SELECT_TIMEOUT,
    FILE_NOT_EXIST,
    MSG_TOO_BIG,
    BAD_LOGIN_PASS,
    UNDEF_XYZ_RESPONSE,
    LACK_OF_MEMORY,
    TIME_ERROR,
    RECVBUF_IS_EMPTY,
    SENDBUF_IS_EMPTY,
    OUT_OF_MSG_RANGE,
    COMMAND_EHLO_STARTTLS,
    SSL_PROBLEM,
    COMMAND_DATABLOCK,
    STARTTLS_NOT_SUPPORTED,
    LOGIN_NOT_SUPPORTED
} SSLError;


static int
initOpenSSL(void)
{
    SSL_library_init();
    SSL_load_error_strings();

    m_ctx = SSL_CTX_new(SSLv23_client_method());
    if (m_ctx == NULL)
	return SSL_PROBLEM;

    return CSMTP_NO_ERROR;
}


static int
openSSLConnect(void)
{
    int res = 0;
    fd_set fdwrite;
    fd_set fdread;
    int write_blocked = 0;
    int read_blocked = 0;
    timeval time;

    if (m_ctx == NULL)
	return SSL_PROBLEM;

    m_ssl = SSL_new(m_ctx);   
    if (m_ssl == NULL)
	return SSL_PROBLEM;

    SSL_set_fd(m_ssl, (int)smtp_socket);
    SSL_set_mode(m_ssl, SSL_MODE_AUTO_RETRY);

    time.tv_sec = TIME_IN_SEC;
    time.tv_usec = 0;

    while (1) {
	FD_ZERO(&fdwrite);
	FD_ZERO(&fdread);

	if (write_blocked)
		FD_SET(smtp_socket, &fdwrite);
	if (read_blocked)
		FD_SET(smtp_socket, &fdread);

	if (write_blocked || read_blocked) {
		write_blocked = 0;
		read_blocked = 0;
		if ((res = select(smtp_socket+1,&fdread,&fdwrite,NULL,&time)) == SOCKET_ERROR) {
			FD_ZERO(&fdwrite);
			FD_ZERO(&fdread);
			return WSA_SELECT;
		}

		if (res == 0) {
			//timeout
			FD_ZERO(&fdwrite);
			FD_ZERO(&fdread);
			return SERVER_NOT_RESPONDING;
		}
	}

	res = SSL_connect(m_ssl);
	switch (SSL_get_error(m_ssl, res)) {
		case SSL_ERROR_NONE:
			FD_ZERO(&fdwrite);
			FD_ZERO(&fdread);
			return CSMTP_NO_ERROR;
			break;

		case SSL_ERROR_WANT_WRITE:
			write_blocked = 1;
			break;

		case SSL_ERROR_WANT_READ:
			read_blocked = 1;
			break;

		default:	      
			FD_ZERO(&fdwrite);
			FD_ZERO(&fdread);
			return SSL_PROBLEM;
	}
    }

    return CSMTP_NO_ERROR;
}


static void
cleanupOpenSSL(void)
{
    if (m_ssl != NULL) {
	SSL_shutdown(m_ssl);  /* send SSL/TLS close_notify */
	SSL_free(m_ssl);
	m_ssl = NULL;
    }

    if (m_ctx != NULL) {
	SSL_CTX_free(m_ctx);	
	m_ctx = NULL;
	ERR_remove_state(0);
	ERR_free_strings();
	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();
    }
}


static int
receiveData_SSL(SSL *ssl, char *buf)
{
    char buff[1024];
    int res = 0;
    int ssl_err;
    int offset = 0;
    fd_set fdread;
    fd_set fdwrite;
    int read_blocked_on_write = 0;
    int bFinish = FALSE;
    timeval time;

    time.tv_sec = SEND_RECIEVE_TO;
    time.tv_usec = 0;

    if (buf == NULL)
	return RECVBUF_IS_EMPTY;

    while (! bFinish) {
	FD_ZERO(&fdread);
	FD_ZERO(&fdwrite);
	FD_SET(smtp_socket, &fdread);
	if (read_blocked_on_write)
		FD_SET(smtp_socket, &fdwrite);

	if ((res = select(smtp_socket+1, &fdread, &fdwrite, NULL, &time)) == SOCKET_ERROR) {
		FD_ZERO(&fdread);
		FD_ZERO(&fdwrite);
		return WSA_SELECT;
	}

	if (res == 0) {
		//timeout
		FD_ZERO(&fdread);
		FD_ZERO(&fdwrite);
		return SERVER_NOT_RESPONDING;
	}

	if (FD_ISSET(smtp_socket,&fdread) || (read_blocked_on_write && FD_ISSET(smtp_socket,&fdwrite)) ) {
		while (1) {
			read_blocked_on_write = 0;

			res = SSL_read(ssl, buff, sizeof(buff));

			if ((ssl_err = SSL_get_error(ssl, res)) == SSL_ERROR_NONE) {
				if (offset + res > MY_BUFF_SIZE - 1) {
					FD_ZERO(&fdread);
					FD_ZERO(&fdwrite);
					return LACK_OF_MEMORY;
				}
				memcpy(buf + offset, buff, res);
				offset += res;
				if (SSL_pending(ssl))
					continue;

				bFinish = TRUE;
				break;
			} else if (ssl_err == SSL_ERROR_ZERO_RETURN) {
				bFinish = TRUE;
				break;
			} else if (ssl_err == SSL_ERROR_WANT_READ) {
				break;
			} else if (ssl_err == SSL_ERROR_WANT_WRITE) {
				/* We get a WANT_WRITE if we're
				 * trying to rehandshake and we block on
				 * a write during that rehandshake.
				 *
				 * We need to wait on the socket to be 
				 * writeable but reinitiate the read
				 * when it is.
				 */
				read_blocked_on_write = 1;
				break;
			} else {
				FD_ZERO(&fdread);
				FD_ZERO(&fdwrite);
				return SSL_PROBLEM;
			}
		}
	}
    }

    FD_ZERO(&fdread);
    FD_ZERO(&fdwrite);
    buf[offset] = 0;
    if (offset == 0)
	return CONNECTION_CLOSED;

    return CSMTP_NO_ERROR;
}


static int
sendData_SSL(SSL *ssl, char *buf)
{
    int offset = 0, res, nLeft = strlen(buf);
    fd_set fdwrite;
    fd_set fdread;
    int write_blocked_on_read = 0;
    timeval time;

    time.tv_sec = SEND_RECIEVE_TO; 
    time.tv_usec = 0;

    if (buf == NULL)
	return SENDBUF_IS_EMPTY;

    while (nLeft > 0) {
	FD_ZERO(&fdwrite);
	FD_ZERO(&fdread);
	FD_SET(smtp_socket,&fdwrite);

	if (write_blocked_on_read)
		FD_SET(smtp_socket, &fdread);

	if ((res = select(smtp_socket+1,&fdread,&fdwrite,NULL,&time)) == SOCKET_ERROR) {
		FD_ZERO(&fdwrite);
		FD_ZERO(&fdread);
		return WSA_SELECT;
	}

	if (res == 0) {
		//timeout
		FD_ZERO(&fdwrite);
		FD_ZERO(&fdread);
		return SERVER_NOT_RESPONDING;
	}

	if (FD_ISSET(smtp_socket,&fdwrite) || (write_blocked_on_read && FD_ISSET(smtp_socket, &fdread)) ) {
		write_blocked_on_read = 0;

		/* Try to write */
		res = SSL_write(ssl, buf+offset, nLeft);
	          
		switch (SSL_get_error(ssl, res)) {
			case SSL_ERROR_NONE: /* We wrote something*/
				nLeft -= res;
				offset += res;
				break;
	              
			case SSL_ERROR_WANT_WRITE: /* We would have blocked */
				break;

			case SSL_ERROR_WANT_READ:
				/* We get a WANT_READ if we're
				 * trying to rehandshake and we block on
				 * write during the current connection.
            			 *
				 * We need to wait on the socket to be readable
				 * but reinitiate our write when it is.
				 */
				write_blocked_on_read = 1;
				break;
	              
			default:	      /* Some other error */
				FD_ZERO(&fdread);
				FD_ZERO(&fdwrite);
				return SSL_PROBLEM;
		}
	}
    }

    OutputDebugStringA(buf);
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdread);

    return CSMTP_NO_ERROR;
}
#endif


static char *
EncodeBase64(char *szIn, char *szOut)
{
    char *pIn = szIn, *pOut = szOut;
    int i, hiteof = FALSE;

    for (i = 0; i < 9; i++) {
	dtable[i]= 'A'+i;
	dtable[i+9]= 'J'+i;
	dtable[26+i]= 'a'+i;
	dtable[26+i+9]= 'j'+i;
    }
    for (i = 0; i < 8; i++) {
	dtable[i+18]= 'S'+i;
	dtable[26+i+18]= 's'+i;
    }
    for (i = 0; i < 10; i++)
	dtable[52+i]= '0'+i;
    dtable[62]= '+';
    dtable[63]= '/';

    while (! hiteof) {
	BYTE igroup[3], ogroup[4];
	int c,n;
	
	igroup[0] = igroup[1] = igroup[2] = 0;
	for (n = 0; n < 3; n++) {
		c = *pIn++;
		if (! c) {
			hiteof = TRUE;
			break;
		}
		igroup[n] = (BYTE)c;
	}
	if (n > 0) {
		ogroup[0] = dtable[igroup[0] >> 2];
		ogroup[1] = dtable[((igroup[0] & 0x03) << 4)|(igroup[1] >> 4)];
		ogroup[2] = dtable[((igroup[1] & 0x0F) << 2)|(igroup[2] >> 6)];
		ogroup[3] = dtable[igroup[2] & 0x3F];

		if (n < 3) {
			ogroup[3] = '=';
			if (n < 2)
				ogroup[2] = '=';
		}

		for (i = 0; i < 4; i++)
			*pOut++ = ogroup[i];
	}
    }
    *pOut = '\0';

    OUTPUTDEBUGMSG((("EncodeBase64(): In >%s< out >%s< \n"),szIn, szOut));

    return szOut;
}


static char *
DecodeBase64(char *szIn, char *szOut)
{
    char *pIn = szIn, *pOut = szOut;
    int i, j;

    for (i = 0; i < 255; i++)
	dtable[i] = 0x80;

    for (i = 'A'; i <= 'I'; i++)
	dtable[i] = 0 + (i-'A');

    for (i = 'J'; i <= 'R'; i++)
	dtable[i] = 9 + (i-'J');

    for (i = 'S'; i <= 'Z'; i++)
	dtable[i] = 18 + (i-'S');

    for (i = 'a'; i <= 'i'; i++)
	dtable[i] = 26 + (i-'a');

    for (i = 'j'; i <= 'r'; i++)
	dtable[i] = 35 + (i-'j');

    for (i = 's'; i <= 'z'; i++)
	dtable[i] = 44 + (i-'s');

    for (i = '0'; i <= '9';i++)
	dtable[i] = 52 + (i-'0');

    dtable['+'] = 62;
    dtable['/'] = 63;
    dtable['='] = 0;

    while (TRUE) {
	BYTE a[4], b[4], o[3];

	for (i = 0; i < 4; i++) {
		int c = *pIn++;		

		if (! c) {
			if (i > 0) {
				OUTPUTDEBUGMSG((("DecodeBase64(): Input line incomplete.\n")));
			}
			*pOut = '\0';
			OUTPUTDEBUGMSG((("DecodeBase64(): In >%s< out >%s< \n"),szIn, szOut));
			return szOut;
		}

		if (dtable[c] & 0x80) {
			OUTPUTDEBUGMSG((("DecodeBase64(): Illegal character '%c' in input line.\n"),c));
			i--;
			continue;
		}
		a[i] = (BYTE)c;
		b[i] = (BYTE)dtable[c];
	}

	o[0] = (b[0] << 2) | (b[1] >> 4);
	o[1] = (b[1] << 4) | (b[2] >> 2);
	o[2] = (b[2] << 6) | b[3];
	i = (a[2] == '=') ? 1 : ((a[3] == '=') ? 2 : 3);

	for (j = 0; j < i; j++)
		*pOut++ = o[j];

	if (i < 3) {
		*pOut = '\0';
		OUTPUTDEBUGMSG((("DecodeBase64(): In >%s< out >%s< \n"),szIn, szOut));
		return szOut;
	}
    }	
}


static void
AddResponse(char *buf) 
{
// #ifdef _DEBUG
    HDC hDC ;
    SIZE Size;

    if (mail.hResponse) {
	hDC = GetDC(mail.hResponse);
	GetTextExtentPoint32(hDC, buf, strlen(buf), &Size);
	if (Size.cx > nMaxLen) {
		nMaxLen = Size.cx;
		SendMessage(mail.hResponse, LB_SETHORIZONTALEXTENT, Size.cx, 0L) ;
	}
	SendMessage(mail.hResponse, LB_ADDSTRING, 0, (LPARAM) (LPSTR)buf);
	OUTPUTDEBUGMSG((("AddResponse() : >>> %s"), buf));

//	FILE *pResponse = NULL;
//	if ((pResponse = fopen("SMTP-response.txt", "a")) != NULL) {
//		fprintf(pResponse, "%s\n", buf);
//		fclose(pResponse);
//		pResponse = NULL;
//	}
    }
// #endif
}


static struct in_addr *
atoAddr(char *address)
{
    static struct in_addr saddr;
    struct hostent *host;
	
    saddr.s_addr=inet_addr(address);
    if (saddr.s_addr != -1)
	return &saddr;
    host = gethostbyname(address);
    if (host != (struct hostent *) NULL)
	return (struct in_addr *)*host->h_addr_list;

    return NULL;
}


static int
initWinSock(void)
{
    WORD version_requested;
    WSADATA wsa_data;
    int err;

    if (! bWsaStartup) {
	version_requested = MAKEWORD(2, 0);
	err = WSAStartup(version_requested, &wsa_data);
	if (err != 0) {
		OUTPUTDEBUGMSG(((" Unable to initialize winsock (%d)\n"), err));
		AddResponse("Unable to initialize winsock\n");
		bWsaStartup = FALSE;
		return -1;
	}

	bWsaStartup = TRUE;
    }

    return 0;
}


// returns SOCKET on success INVALID_SOCKET on failure
static SOCKET
clientSocket(char *address, int port)
{
    struct sockaddr_in sa;
    struct in_addr *addr;
    SOCKET s;
    int rc;

    rc = initWinSock();
    if (rc != 0) {
	OUTPUTDEBUGMSG((("clientSocket() : Error in initWinSock() = 0x%04x\n"), rc));
	AddResponse("clientSocket() : Error in initWinSock()\n");
	return INVALID_SOCKET;
    }	

    addr = atoAddr(address);
    if (addr == NULL) {
	OUTPUTDEBUGMSG((("clientSocket() : Invalid address: %s\n"), address));
	AddResponse("clientSocket() : Invalid address\n");
	return INVALID_SOCKET;
    }

    memset(&sa, 0x00, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons((unsigned short)port);
    sa.sin_addr.s_addr = addr->s_addr;

    // open the socket
    s = socket(AF_INET, SOCK_STREAM, PF_UNSPEC);
    if (s == INVALID_SOCKET) {
	OUTPUTDEBUGMSG((("clientSocket() : Could not create socket\n")));
	AddResponse("clientSocket() : Could not create socket\n");
	return INVALID_SOCKET;
    }

    // connect
    connect(s, (struct sockaddr *)&sa, sizeof(sa));
//FIXME: check return value!

    return s;
}


// this function writes a character string out to a socket.
// it returns -1 if the connection is closed while it is trying to write
static int
sockWrite(SOCKET sock, char *str, size_t count)
{
    size_t bytesSent = 0; 
    int thisWrite;

    while (bytesSent < count)  {
	thisWrite = send(sock, str, count-bytesSent, 0);
	if (thisWrite <= 0)
		return thisWrite;
	bytesSent += thisWrite; 
	str += thisWrite;
    }

    return count;
}


static int
sockPuts(SOCKET sk, char *str)
{
//  OUTPUTDEBUGMSG((("sockPuts() : %s\n"), str));
    AddResponse(str);

#ifdef USE_SSL
    if (m_ssl != NULL)
	return sendData_SSL(m_ssl, str);
#endif

    return sockWrite(sk, str, strlen(str));
}


static int
sockGets(SOCKET sk, char *str, size_t count)
{
    int bytesRead;
    size_t totalCount = 0;
    char buf[1], *currentPosition;
    char lastRead = 0;

    currentPosition = str;
    while (lastRead != 10) {
	bytesRead = recv(sk, buf, 1, 0);
	OUTPUTDEBUGMSG((("%s,%d\n"), __FILE__, bytesRead));
	if (bytesRead <= 0) {
		// the other side may have closed unexpectedly
		OUTPUTDEBUGMSG((("ERRNO:%d\n"), WSAGetLastError()));
		return -1;
	}
	lastRead = buf[0];
	if ((totalCount < count) && (lastRead != 10) && (lastRead != 13)) {
		*currentPosition = lastRead;
		currentPosition++;
		totalCount++;
	}
    }

    if (count > 0)
	*currentPosition = 0;

    return totalCount;
}


// disconnect to SMTP server and returns the socket fd
static void
smtpDisconnect(SOCKET sk)
{
#ifdef USE_SSL
    cleanupOpenSSL();
#endif
    closesocket(sk);
}


// connect to SMTP server and returns the socket fd
static SOCKET
smtpConnect(char *smtp_server, int port)
{
    SOCKET sk;
#ifdef USE_SSL
    int res;
#endif
	
    sk = clientSocket(smtp_server,port);
    if (sk == INVALID_SOCKET) {
	OUTPUTDEBUGMSG((("smtpConnect() : Could not connect to SMTP server \"%s\" at port %d\n"), smtp_server, port));
	AddResponse("smtpConnect() : Could not connect to SMTP server\n");

	nSMTPerrors++;

	return INVALID_SOCKET;
    }

    // save it. we'll need it to clean up
    smtp_socket = sk;

#ifdef USE_SSL
    if (Profile.ssl) {
	if ((res = initOpenSSL()) == CSMTP_NO_ERROR)
		res = openSSLConnect();
	OUTPUTDEBUGMSG(("SSL Connect res = %d\n",res));
    }
#endif

    return sk;
}


// read SMTP response. returns 0 on success, -1 on failure 
static int
smtpResponse(SOCKET sk)
{
    char buf[MY_BUFF_SIZE], tmp[MY_BUFF_SIZE];
    int n, err;

    memset(buf, 0x00, sizeof(buf));

#ifdef USE_SSL
    if (m_ssl != NULL)
	err = receiveData_SSL(m_ssl,buf);
    else
#endif
	n = sockGets(sk, buf, sizeof(buf)-1);
//  OUTPUTDEBUGMSG((("smtpResponse() : %s\n"), buf));
    AddResponse(buf);
    err = atoi(buf);
    OUTPUTDEBUGMSG((("smtpResponse(): Err: %d!\n"), err));
    if (err == 334) {
	DecodeBase64(buf+4, tmp);
	strcpy(buf+4, tmp);
    }

    if (buf[0] == '1' || buf[0] == '2' || buf[0] == '3' && buf[3] == A_SPACE)
	return 0;

    OUTPUTDEBUGMSG((("smtpResponse(): ERROR!\n")));
    nSMTPerrors++;

    // PH: save Last Error
    iSMTPlastError = err;

    return -1;
}


// SMTP: HELO
static int
smtpHelo(SOCKET sk)
{
    // read off the greeting 
#if 0
    //smtpResponse(sk);
#else
    // client should wait for greeting before sending HELO
    smtpResponse(sk);
#endif
    _snprintf(buf, sizeof(buf)-1, "HELO %s\r\n", mail.helo_domain);
//  _snprintf(buf, sizeof(buf)-1, "EHLO %s\r\n", mail.helo_domain);
    sockPuts(sk, buf);

    return smtpResponse(sk);
}


// SMTP: Authentication
static int
smtpLogin(SOCKET sk)
{
    char szTmp[128];

    if (mail.options & MAIL_OPTION_AUTH) {
	_snprintf(buf, sizeof(buf)-1, "AUTH LOGIN\r\n");
	sockPuts(sk, buf);
	if (smtpResponse(sk))
		return TRUE;

	_snprintf(buf, sizeof(buf)-1, "%s\r\n", EncodeBase64(mail.user, szTmp));
	sockPuts(sk, buf);
	if (smtpResponse(sk))
		return TRUE;

	_snprintf(buf, sizeof(buf)-1, "%s\r\n", EncodeBase64(mail.password, szTmp));
	sockPuts(sk, buf);
	return smtpResponse(sk);
    }

    return FALSE;
}


// SMTP: MAIL FROM 
static int
smtpMailFrom(SOCKET sk)
{
    _snprintf(buf, sizeof(buf)-1, "MAIL FROM: <%s>\r\n", mail.from);
//  OUTPUTDEBUGMSG((("smtpMailFrom() : >>> %s"), buf));
    sockPuts(sk, buf);

    return smtpResponse(sk);
}


// SMTP: quit
static int
smtpQuit(SOCKET sk)
{
    sockPuts(sk, "QUIT\r\n");

    return smtpResponse(sk);
}


// SMTP: RSET
// aborts current mail transaction and cause both ends to reset
static int
smtpRset(SOCKET sk)
{
    sockPuts(sk, "RSET\r\n");

    return smtpResponse(sk);
}


static char *
StripSpecial(char *szStr)
{
    int len = strlen(szStr);

    while (len--) {
	switch (szStr[len]) {
		case ',' :
		case ';' :
		case ' ' :
			szStr[len] = '\0';
			break;

		default:
			return szStr;
	}
    }

    return szStr;
}


// SMTP: RCPT TO
static int
smtpRcptTo(SOCKET sk)
{
    static char szTemp[MAX_MAIL * 5];
    char *pTmp1 = szTemp, *pTmp2;

    strncpy(szTemp, mail.to, (MAX_MAIL * 5) - 1);
    StripSpecial(szTemp);

    while (1) {
	pTmp2 = strchr(pTmp1, ';');
	if (pTmp2 == NULL)
		pTmp2 = strchr(pTmp1, ',');
	if (pTmp2 != NULL)
		*pTmp2 = '\0';

	_snprintf(buf, sizeof(buf)-1, "RCPT TO: <%s>\r\n", pTmp1);
	sockPuts(sk, buf);
	if (smtpResponse(sk) != 0) {
		smtpRset(sk);
		return -1;
	}

	if (pTmp2 != NULL) {
		pTmp1 = pTmp2;
		pTmp1++;
	} else
		break;
    } 

    return 0;
}


// SMTP: DATA
static int
smtpData(SOCKET sk)
{
    sockPuts(sk, "DATA\r\n");

    return smtpResponse(sk);
}


// SMTP: EOM
static int
smtpEom(SOCKET sk)
{
    sockPuts(sk, "\r\n.\r\n");

    return smtpResponse(sk);
}


// SMTP: mail
static int
smtpMail(SOCKET sk, char *data)
{	
    char szBuffer[128], *pTmp ;
    char szSubject[1024]="";
    char szBody[1024]="";
    int i;

    for (i = 0; data[i] != 0; i++) {
	if (data[i] == '»') {
		strcat(szSubject, " - ");
		strcat(szBody, "\n");
	} else {
		szSubject[strlen(szSubject)] = data[i];
		szBody[strlen(szBody)] = data[i];
	}
    }

    if (mail.options & MAIL_OPTION_SUBJECT) {
	if (szSubject[0]) {
		memset(buf, 0x00, sizeof(buf));
		_snprintf(buf, sizeof(buf)-1, "Subject: %s\r\n", szSubject);
		sockPuts(sk, buf);

		memset(buf, 0x00, sizeof(buf));
		strcpy(szBuffer, szSmtpCharSets[((Profile.nMailOptions & 0x1F0000) >> 16) -1]);
		pTmp = strchr(szBuffer, ' ');
		if (pTmp != NULL)
			*pTmp = '\0';
		_snprintf(buf, sizeof(buf)-1, "Content-type: text/plain; charset=\"%s\"\r\n", szBuffer);
		sockPuts(sk, buf);
	}
    }

    // headers
    if (mail.from) {
	memset(buf, 0x00, sizeof(buf));
	_snprintf(buf, sizeof(buf)-1, "From: %s\r\n", mail.from);
	sockPuts(sk, buf);
    }
    if (mail.to) {
	memset(buf, 0x00, sizeof(buf));
	_snprintf(buf, sizeof(buf)-1, "To: %s\r\n", mail.to);
	sockPuts(sk, buf);	
    }
    if (mail.cc) {
	memset(buf, 0x00, sizeof(buf));
	_snprintf(buf, sizeof(buf)-1, "Cc: %s\r\n", mail.cc);
	sockPuts(sk, buf);
    }
    if (mail.bcc) {
	memset(buf, 0x00, sizeof(buf));
	_snprintf(buf, sizeof(buf)-1, "Bcc: %s\r\n", mail.bcc);
	sockPuts(sk, buf);
    }
    memset(buf, 0x00, sizeof(buf));
    _snprintf(buf, sizeof(buf)-1, "X-Mailer: %s\r\n", MAILSEND_VERSION);
    sockPuts(sk, buf);

    /* Empty line to terminate headers and start (optional) body. */
    sockPuts(sk, "\r\n");

    if ((mail.options & MAIL_OPTION_MSG) && szBody[0]) {
	sockPuts(sk, szBody);
	sockPuts(sk, "\r\n");
    }

    nSMTPemails++;

    return 0;
}


// returns 0 on success, -1 on failure
int
xSendMail(THEMAIL *pMail)
{
    SOCKET sk;
    int rc = -1;
    char *pTmp;

    if (nBufferdMailStart == nBufferdMailEnd)
	return 0;

    pTmp = szMailBuffer[nBufferdMailEnd];
    nBufferdMailEnd++;
    if (nBufferdMailEnd >= MAX_MAIL)
	nBufferdMailEnd = 0 ;

    if (pMail->from == NULL) {
	OUTPUTDEBUGMSG((("No From address specified")));
	AddResponse("xSendMail(): No From address specified\n");
	return (0);
    }
    if (pMail->smtp_server == NULL) {
	pMail->smtp_server= "127.0.0.1";
	OUTPUTDEBUGMSG((("No smtp_server specified using default : %s"), pMail->smtp_server)) ;
    }
    if (pMail->smtp_port == -1) {
	pMail->smtp_port = MAILSEND_SMTP_PORT;
	OUTPUTDEBUGMSG((("No smtp_port specified using default port %d"), pMail->smtp_port));
    }
    if (pTmp == NULL) {
	pTmp = MAILSEND_DEF_SUB;
	OUTPUTDEBUGMSG((("No subject specified using default subject %s"), pTmp));
    }
    if (pMail->helo_domain == NULL) {
	pMail->helo_domain= "127.0.0.1";
	OUTPUTDEBUGMSG((("No domain specified using default %s"), pMail->helo_domain));
    }

    // open the network connection
    sk = smtpConnect(pMail->smtp_server, pMail->smtp_port);
    if (sk == INVALID_SOCKET) {
	nSMTPerrors++;			// PH: Counts # of Errors
	return 0;
    }
    nSMTPsessions++;			// PH: Counts # of sessions

    if (! (rc = smtpHelo(sk))) {
	if (! (rc = smtpLogin(sk))) {
		if (! (rc = smtpMailFrom(sk))) {
			if (! (rc = smtpRcptTo(sk))) {
				if (! (rc = smtpData(sk))) {
					if (! (rc = smtpMail(sk, pTmp))) {
						if (! (rc = smtpEom(sk))) {
							rc = smtpQuit(sk);
						}
					}
				}
			}
		}
	}
    }

    // close the network connection
    smtpDisconnect(sk);

    return 1;
}


DWORD WINAPI
MailThreadFunc(LPVOID lpData)
{
    OUTPUTDEBUGMSG((("MailThreadFunc()")));

    while (keepbusy) {
	if (! xSendMail((THEMAIL *)lpData))
		Sleep(1000);
    }

    OUTPUTDEBUGMSG((("MailThreadFunc() 	ExitThread(0)\n")));
    ExitThread(0);

    return 0;
}


void
StartMail(int nOptions)
{
    DWORD dummy = 0;

//  OUTPUTDEBUGMSG((("StartMail()")));
    if (nOptions & MAIL_OPTION_ENABLE) {
	if (MailThread != 0) {
		OUTPUTDEBUGMSG((("StartMail() MailThread != 0  Mail is already Started!")));
		return;
	}
	keepbusy = TRUE ;
	MailThread = CreateThread(0, 0, MailThreadFunc, (LPVOID)&mail, 0, &dummy);
		OUTPUTDEBUGMSG((("StartMail() CreateThread\n")));
    } else {
	if (MailThread == NULL) {
		OUTPUTDEBUGMSG((("StartMail() MailThread == 0  Mail is already Stopped!")));
		return;
	}

	keepbusy = FALSE;
	Sleep(500);
	CloseHandle(MailThread);
	MailThread = 0;
	OUTPUTDEBUGMSG((("StartMail() CloseHandle(MailThread)\n")));
    }
}


int
SendMail(HWND hResponse, int bMatch, int bMonitor_only, int iSeparateSMTP, char *sz1, char *sz2, char *sz3, char *sz4, char *sz5, char *sz6, char *sz7, char *szLabel)
{
    int len = 0;
    char szBuffer[1024] = { 0 };
//  char szSubject[1024] = "";

//  OUTPUTDEBUGMSG((("SendMail()")));
    mail.hResponse = hResponse;
    if (hResponse)
	SendMessage(hResponse, LB_RESETCONTENT, 0, 0L);

    if (mail.options & MAIL_OPTION_ENABLE) {
	switch (mail.options & MAIL_OPTION_MODES) {
		case MAIL_OPTION_MODE_ALL:
			OUTPUTDEBUGMSG((("SendMail() Send : Mode All")));
			break;

		case MAIL_OPTION_MODE_FILTER:	
			if (!bMatch || bMonitor_only) {
				OUTPUTDEBUGMSG((("SendMail() Not Send : !bMatch || bMonitor_only")));
				return 0;
			}
			OUTPUTDEBUGMSG((("SendMail() Send : bMatch(%d) || bMonitor_only(%d)"), bMatch, bMonitor_only));
			break;

		case MAIL_OPTION_MODE_MONITOR:
			if (! bMatch) {
				OUTPUTDEBUGMSG((("SendMail() Not Send: !bMatch")));
				return 0;
			}
			OUTPUTDEBUGMSG((("SendMail() Send : bMatch(%d) || bMonitor_only(%d)"), bMatch, bMonitor_only));
			break;

		case MAIL_OPTION_MODE_SELECTABLE:
			if (!bMatch || !iSeparateSMTP) {
				OUTPUTDEBUGMSG((("SendMail() Not Send: !bMatch || !iSeparateSMTP")));
				return 0;
			}
			OUTPUTDEBUGMSG((("SendMail() Send : iSeparateSMTP(%d)"), iSeparateSMTP));
			break;
	}
    } else {
	OUTPUTDEBUGMSG((("SendMail() Mail Disabled")));
	return 0;
    }

    if (mail.options & MAIL_OPTION_ADDRESS)
	len += wsprintf(szBuffer + len, "%s ", sz1);
    if (mail.options & MAIL_OPTION_TIME)
	len += wsprintf(szBuffer + len, "%s ", sz2);
    if (mail.options & MAIL_OPTION_DATE)
	len += wsprintf(szBuffer + len, "%s ", sz3);
    if (mail.options & MAIL_OPTION_MODE)
	len += wsprintf(szBuffer + len, "%s ", sz4);
    if (mail.options & MAIL_OPTION_TYPE)
	len += wsprintf(szBuffer + len, "%s ", sz5);
    if (mail.options & MAIL_OPTION_BITRATE)
	len += wsprintf(szBuffer + len, "%s ", sz6);
    if (mail.options & MAIL_OPTION_MESSAGE)
	len += wsprintf(szBuffer + len, "%s ", sz7);
    if (mail.options & MAIL_OPTION_LABEL)
	len += wsprintf(szBuffer + len, "- %s ", szLabel);

    if (mail.smtp_port == 0) {
	OUTPUTDEBUGMSG((("SendMail() Error: MailInit NOT called!")));
	nSMTPerrors++;		// PH: Counts # of Errors

	return -1;
    }

    nMaxLen = 0;
    if (szBuffer[0]) {
	OUTPUTDEBUGMSG((("SendMail() Send : >%s<\n"), szBuffer));
	strncpy(szMailBuffer[nBufferdMailStart], szBuffer, MAX_MAIL_LEN);
	nBufferdMailStart++;

	if (nBufferdMailStart >= MAX_MAIL)
		nBufferdMailStart = 0;
    }

//  OUTPUTDEBUGMSG((("SendMail() nBufferdMailStart %d nBufferdMailEnd %d\n"), nBufferdMailStart, nBufferdMailEnd));
    return 0;
}	


int
MailInit(char *szMailHost, char *szMailHeloDomain, char *szMailFrom, char *szMailTo, char *szMailUser, char *szMailPassword, int iMailPort, int nOptions)
{
    memset(&mail, 0x00, sizeof(mail));
    mail.from = szMailFrom;
    mail.to = szMailTo;
    mail.cc = NULL;
    mail.bcc = NULL;
    mail.smtp_server = szMailHost;
    mail.helo_domain = szMailHeloDomain;
    mail.user = szMailUser;
    mail.password = szMailPassword;
//  mail.smtp_port = -1;
    mail.smtp_port = iMailPort;
    mail.helo_domain = NULL;
    mail.options = nOptions;

    StartMail(nOptions);

    return 0;
}
