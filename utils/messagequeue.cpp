#include "messagequeue.h"


int bMqMonitorStarted = 0;


void
StartMQMonitor(void)
{
    if (! bMqMonitorStarted) {
	bMqMonitorStarted = 1;
    }
}


void
SendMQMessage(char *capCode, char *time, char *date, char *mode, char *type, char *bitRate, char *message)
{
    if (! bMqMonitorStarted)
	StartMQMonitor();
}
