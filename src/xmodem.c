/*
 * Copyright 2001-2010 Georges Menie (www.menie.org)
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University of California, Berkeley nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* this code needs standard functions memcpy() and memset()
   and input/output functions _inbyte() and _outbyte().

   the prototypes of the input/output functions are:
     int _inbyte(unsigned short timeout); // msec timeout
     void _outbyte(int c);

 */

#include "common.h"

#define SOH  0x01
#define STX  0x02
#define EOT  0x04
#define ACK  0x06
#define NAK  0x15
#define CAN  0x18
#define CTRLZ 0x1A

#define DLY_1S 1000
#define MAXRETRANS 25
#define TRANSMIT_XMODEM_1K

static int check(int crc, const unsigned char *buf, int sz)
{
    if(crc)
    {
        unsigned short crc = crc16_ccitt(buf, sz);
        unsigned short tcrc = (buf[sz]<<8)+buf[sz+1];
        if(crc == tcrc)
            return 1;
    }
    else
    {
        int i;
        unsigned char cks = 0;
        for(i = 0; i < sz; ++i)
        {
            cks += buf[i];
        }
        if(cks == buf[sz])
            return 1;
    }

    return 0;
}


int _inbyte(unsigned short timeoutms)
{
    UartStopRcvFlag=1;
    int nread, ret;
    fd_set readfds;
    struct timeval timeout;
    timeout.tv_sec = timeoutms/1000;         
    timeout.tv_usec = timeoutms%1000;//阻塞
    if(timeoutms>0)
    {
        FD_ZERO(&readfds);       //每次循环都要清空集合，select检测描述文件中是否有可读的，从而能检测描述符变化
        FD_SET(Uart_fd, &readfds);
        ret = select(Uart_fd + 1, &readfds, NULL, NULL, &timeout); //select检测描述文件中是否有可读的
        if(ret < 0)
        {
            printf("select error!\n");
        }

        if(ret == 0)
        {
            //printf("receive timeout!\n");
        }
        //检测读文件描述符集合，一直在循环，监视描述符的变化
        ret = FD_ISSET(Uart_fd, &readfds);
        if(ret > 0)
        {
            bzero(Uart_Rx_Buff, LENGTH);
            if((nread = read(Uart_fd, Uart_Rx_Buff, LENGTH)) > 0)   //接收数据
            {
                Uart_Data_Len = nread;
                printf("%s",Uart_Rx_Buff);
                fflush(stdout);
            }
        }
    }
    UartStopRcvFlag=0;
    return Uart_Rx_Buff[0];
}

void _outbyte(int c)
{
    char buf[2]={0};
    buf[0]=c;
    write(Uart_fd, buf, 1);
}

static void flushinput(void)
{
    int count=5;
    while((_inbyte(((DLY_1S)*3)>>1) >= 0)&&(count--));
}

int xmodemReceive(unsigned char *dest, int destsz)
{
    unsigned char xbuff[1030]; /* 1024 for XModem 1k + 3 head chars + 2 crc + nul */
    unsigned char *p;
    int bufsz, crc = 0;
    unsigned char trychar = 'C';
    unsigned char packetno = 1;
    int i, c, len = 0;
    int retry, retrans = MAXRETRANS;

    for(;;)
    {
        for(retry = 0; retry < 16; ++retry)
        {
            if(trychar) _outbyte(trychar);
            if((c = _inbyte((DLY_1S)<<1)) >= 0)
            {
                switch(c)
                {
                    case SOH:
                        bufsz = 128;
                        goto start_recv;
                    case STX:
                        bufsz = 1024;
                        goto start_recv;
                    case EOT:
                        flushinput();
                        _outbyte(ACK);
                        return len; /* normal end */
                    case CAN:
                        if((c = _inbyte(DLY_1S)) == CAN)
                        {
                            flushinput();
                            _outbyte(ACK);
                            return -1; /* canceled by remote */
                        }
                        break;
                    default:
                        break;
                }
            }
        }
        if(trychar == 'C')
        {
            trychar = NAK;
            continue;
        }
        flushinput();
        _outbyte(CAN);
        _outbyte(CAN);
        _outbyte(CAN);
        return -2; /* sync error */

    start_recv:
        if(trychar == 'C') crc = 1;
        trychar = 0;
        p = xbuff;
        *p++ = c;
        for(i = 0;  i < (bufsz+(crc?1:0)+3); ++i)
        {
            if((c = _inbyte(DLY_1S)) < 0) goto reject;
            *p++ = c;
        }

        if(xbuff[1] == (unsigned char)(~xbuff[2]) &&
           (xbuff[1] == packetno || xbuff[1] == (unsigned char)packetno-1) &&
           check(crc, &xbuff[3], bufsz))
        {
            if(xbuff[1] == packetno)
            {
                register int count = destsz - len;
                if(count > bufsz) count = bufsz;
                if(count > 0)
                {
                    memcpy(&dest[len], &xbuff[3], count);
                    len += count;
                }
                ++packetno;
                retrans = MAXRETRANS+1;
            }
            if(--retrans <= 0)
            {
                flushinput();
                _outbyte(CAN);
                _outbyte(CAN);
                _outbyte(CAN);
                return -3; /* too many retry error */
            }
            _outbyte(ACK);
            continue;
        }
    reject:
        flushinput();
        _outbyte(NAK);
    }
}

int xmodemTransmit(unsigned char *src, int srcsz)
{
    unsigned char xbuff[1030]; /* 1024 for XModem 1k + 3 head chars + 2 crc + nul */
    int bufsz, crc = -1;
    unsigned char packetno = 1;
    int i, c, len = 0;
    int retry;
    for(;;)
    {
        for(retry = 0; retry < 16; ++retry)
        {
            if((c = _inbyte((DLY_1S)<<1)) >= 0)
            {
                switch(c)
                {
                    case 'C':
                        crc = 1;                        
                        goto start_trans;
                    case NAK:
                        crc = 0;
                        goto start_trans;
                    case CAN:
                        if((c = _inbyte(DLY_1S)) == CAN)
                        {
                            _outbyte(ACK);
                            flushinput();
                            return -1; /* canceled by remote */
                        }
                        break;
                    default:
                        break;
                }
            }
        }
        _outbyte(CAN);
        _outbyte(CAN);
        _outbyte(CAN);
        flushinput();
        return -2; /* no sync */

        for(;;)
        {        
        start_trans:
#ifdef TRANSMIT_XMODEM_1K
            xbuff[0] = STX;
            bufsz = 1024;
#else
            xbuff[0] = SOH;
            bufsz = 128;
#endif
            xbuff[1] = packetno;
            xbuff[2] = ~packetno;
            c = srcsz - len;
            if(c > bufsz) c = bufsz;
            if(c > 0)
            {
                memset(&xbuff[3], 0, bufsz);
                memcpy(&xbuff[3], &src[len], c);
                if(c < bufsz) xbuff[3+c] = CTRLZ;
                if(crc)
                {
                    unsigned short ccrc = crc16_ccitt(&xbuff[3], bufsz);
                    xbuff[bufsz+3] = (ccrc>>8) & 0xFF;
                    xbuff[bufsz+4] = ccrc & 0xFF;
                }
                else
                {
                    unsigned char ccks = 0;
                    for(i = 3; i < bufsz+3; ++i)
                    {
                        ccks += xbuff[i];
                    }
                    xbuff[bufsz+3] = ccks;
                }
                for(retry = 0; retry < MAXRETRANS; ++retry)
                {          
                    for(i = 0; i < bufsz+4+(crc?1:0); ++i)
                    {
                        _outbyte(xbuff[i]);
                    }
                    
                    if((c = _inbyte(DLY_1S)) >= 0)
                    {
                        switch(c)
                        {
                            case ACK:
                                ++packetno;
                                len += bufsz;
                                printf("%.2f%%\n",((float)packetno/((srcsz/bufsz)+2))*100);
                                goto start_trans;
                            case CAN:
                                if((c = _inbyte(DLY_1S)) == CAN)
                                {
                                    _outbyte(ACK);
                                    flushinput();
                                    return -1; /* canceled by remote */
                                }
                                break;
                            case NAK:
                            default:
                                break;
                        }
                    }
                }
                _outbyte(CAN);
                _outbyte(CAN);
                _outbyte(CAN);
                flushinput();
                return -4; /* xmit error */
            }
            else
            {
                for(retry = 0; retry < 10; ++retry)
                {
                    _outbyte(EOT);
                    if((c = _inbyte((DLY_1S)<<1)) == ACK)  break;                    
                }
                flushinput();
                return (c == ACK)?len:-5;
            }
        }
    }
}

