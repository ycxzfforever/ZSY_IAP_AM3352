#ifndef _UART_H_
#define _UART_H_

void set_speed(int fd, int speed);
int set_Parity(int fd, int databits, int stopbits, int parity);
int Open_Uart(char *uart_no);
int Uart_Pthread_Creat();
void SendRebootDispenser();
void SendIAP();
void QuitBootloader();
void GetVersion();




#endif

