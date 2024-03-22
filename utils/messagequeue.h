#ifndef MESSAGEQUEUE_H
# define MESSAGEQUEUE_H


void StartMQMonitor(void);

void SendMQMessage(char *capCode, char *time, char *date, char *mode, char *type, char *bitRate, char *message); 


#endif	/*MESSAGEQUEUE_H*/
