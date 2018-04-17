#ifndef _common_H
#define _common_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/in.h>
#include <error.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <pthread.h>
#include <time.h>
#include <sys/timeb.h>
#include <stdarg.h>
#include <signal.h>
#include <time.h>
#include <sys/ioctl.h>
#include <math.h>
#include <net/if.h>
#include <netinet/tcp.h>
#include <linux/socket.h>
#include <sys/stat.h>
#include <termios.h>

#include "inirw.h"
#include "uart.h"
#include "crc16.h"



#define Uchar	unsigned char
#define Uint 	unsigned int
#define Ushort	unsigned short
#define Ulong 	unsigned long

#define inifile_Name  	"/JKJN/sys.ini"	    //配置文件路径和名字




#define BIT0        0X00000001
#define BIT1        0X00000002
#define BIT2        0X00000004
#define BIT3        0X00000008
#define BIT4        0X00000010
#define BIT5        0X00000020
#define BIT6        0X00000040
#define BIT7        0X00000080
#define BIT8        0X00000100
#define BIT9        0X00000200
#define BIT10       0X00000400
#define BIT11       0X00000800
#define BIT12       0X00001000
#define BIT13       0X00002000
#define BIT14       0X00004000
#define BIT15       0X00008000
#define BIT16       0X00010000
#define BIT17       0X00020000
#define BIT18       0X00040000
#define BIT19       0X00080000
#define BIT20       0X00100000
#define BIT21       0X00200000
#define BIT22       0X00400000
#define BIT23       0X00800000
#define BIT24       0X01000000
#define BIT25       0X02000000
#define BIT26       0X04000000
#define BIT27       0X08000000
#define BIT28       0X10000000
#define BIT29       0X20000000
#define BIT30       0X40000000
#define BIT31       0X80000000


#define Uchar   unsigned char
#define Uint    unsigned int
#define Ushort  unsigned short
#define Ulong   unsigned long


typedef union       //浮点数联合体
{
  float data;
  Uchar rdcv[4];
}UnionFloat;

typedef union       // 16位整型数联合体
{
  Ushort data;
  Uchar  rdcv[2];
}UnionU16;

typedef union       // 32位整型数联合体
{
	Uint    data;
	Uchar   rdcv[4];
}UnionU32;

typedef union       // 64位整型数联合体
{
	unsigned long long data;
	Uchar   rdcv[8];
}UnionU64;


struct Conf
{
    Uchar Gun_num;              //枪号
    char Uart_NO[12];           //选择串口号
    Uint Uart_Speed;            //配置串口波特率
    Uchar Uart_Data_Bits;       //配置数据位
    Uchar Uart_Stop_Bits;       //配置停止位
    Uchar Uart_Parity[2];       //配置校验方式N:无校验、O：奇校验、E：偶校验
};

struct Conf conf;

#define LENGTH 1030

int     Uart_fd;
int     Uart_Data_Len;

Uchar   Uart_Tx_Flag;
Uchar   UartStopRcvFlag;
Uchar   Uart_Rx_Buff[LENGTH];              //串口接收数据
Uchar   Uart_Tx_Buff[LENGTH];              //串口发送数据

int xmodemTransmit(unsigned char *src, int srcsz);


#endif

