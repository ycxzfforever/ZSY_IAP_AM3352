#include     "common.h"
#define FALSE 1
#define TRUE 0

int speed_arr[] = {B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200};
int name_arr[] = {115200, 57600, 38400,  19200,  9600,  4800,  2400, 1200};

extern char *secname; 


/********************************************************************\
* 函数名: set_speed
* 说明:
* 功能:    设置串口波特率
* 输入:     fd:文件描述符
            speed:波特率
* 返回值:   0:成功，非0:失败
* 创建人:   Yang Chao Xu
* 创建时间: 2014-8-22
\*********************************************************************/
void set_speed(int fd, int speed)
{
    int   i;
    int   status;
    struct termios   Opt;
    tcgetattr(fd, &Opt);
    for(i = 0;  i < sizeof(speed_arr) / sizeof(int);  i++)
    {
        if(speed == name_arr[i])
        {
            tcflush(fd, TCIOFLUSH);
            cfsetispeed(&Opt, speed_arr[i]);
            cfsetospeed(&Opt, speed_arr[i]);
            status = tcsetattr(fd, TCSANOW, &Opt);
            if(status != 0)
                perror("tcsetattr fd1");
            return;
        }
        tcflush(fd, TCIOFLUSH);
    }
}

/********************************************************************\
* 函数名: set_Parity
* 说明:
* 功能:    设置串口数据位、停止位、校验方式
* 输入:     fd:文件描述符
            databits:数据位
            stopbits:停止位
            parity:校验方式
* 返回值:   0:成功，非0:失败
* 创建人:   Yang Chao Xu
* 创建时间: 2014-8-22
\*********************************************************************/
int set_Parity(int fd, int databits, int stopbits, int parity)
{
    struct termios options;
    if(tcgetattr(fd, &options)  !=  0)
    {
        perror("SetupSerial 1");
        return(FALSE);
    }
    options.c_cflag &= ~CSIZE;
    switch(databits)
    {
        case 7:
            options.c_cflag |= CS7;
            break;
        case 8:
            options.c_cflag |= CS8;
            break;
        default:
            fprintf(stderr, "Unsupported data size\n");
            return (FALSE);
    }
    switch(parity)
    {
        case 'n':
        case 'N':
            options.c_cflag &= ~PARENB;
            options.c_iflag &= ~INPCK;
            break;
        case 'o':
        case 'O':
            options.c_cflag |= (PARODD | PARENB);
            options.c_iflag |= INPCK;
            break;
        case 'e':
        case 'E':
            options.c_cflag |= PARENB;
            options.c_cflag &= ~PARODD;
            options.c_iflag |= INPCK;
            break;
        case 'S':
        case 's':
            options.c_cflag &= ~PARENB;
            options.c_cflag &= ~CSTOPB;
            break;
        default:
            fprintf(stderr, "Unsupported parity\n");
            return (FALSE);
    }
    switch(stopbits)
    {
        case 1:
            options.c_cflag &= ~CSTOPB;
            break;
        case 2:
            options.c_cflag |= CSTOPB;
            break;
        default:
            fprintf(stderr, "Unsupported stop bits\n");
            return (FALSE);
    }

    options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    options.c_oflag &= ~OPOST;
    options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

    /* Set input parity option */

    if(parity != 'n')
        options.c_iflag |= INPCK;
    options.c_cc[VTIME] = 150; // 15 seconds
    options.c_cc[VMIN] = 0;

    tcflush(fd, TCIFLUSH); /* Update the options and do it NOW */
    if(tcsetattr(fd, TCSANOW, &options) != 0)
    {
        perror("SetupSerial 3");
        return (FALSE);
    }
    return (TRUE);
}

void DealUartRecvData()
{
    if(Uart_Rx_Buff[0]==0xBB&&Uart_Rx_Buff[1]==0xEE&&Uart_Rx_Buff[4]==0xBB)//打印版本号
        printf("%s\n", &Uart_Rx_Buff[7]);
    else
        printf("%s\n", &Uart_Rx_Buff[0]);
    fflush(stdout);
}

void Uart_Receive_Thread_Select()
{
    int nread, ret;
    fd_set readfds;
    struct timeval timeout;
    timeout.tv_sec = 0;         //无阻塞
    timeout.tv_usec = 0;
    while(1)
    {
        if(UartStopRcvFlag==0)//线程接收用于打印
        {
            FD_ZERO(&readfds);       //每次循环都要清空集合，select检测描述文件中是否有可读的，从而能检测描述符变化
            FD_SET(Uart_fd, &readfds);
            ret = select(Uart_fd + 1, &readfds, NULL, NULL, &timeout); //select检测描述文件中是否有可读的
            usleep(10 * 1000); //等待数据收完
            if(ret < 0)
            {
                printf("select error!\n");
            }
            //检测读文件描述符集合，一直在循环，监视描述符的变化
            ret = FD_ISSET(Uart_fd, &readfds);
            if(ret > 0)
            {
                bzero(Uart_Rx_Buff, LENGTH);
                if((nread = read(Uart_fd, Uart_Rx_Buff, LENGTH)) > 0)   //接收数据
                {
                    Uart_Data_Len = nread;
                    DealUartRecvData();                    
                }
            }
        }
        else//其他地方接收
        {
            usleep(5 * 1000);
        }
    }
    return;
}



/********************************************************************\
* 函数名：: Uart_send_Thread
* 说明:
* 功能:     串口数据发送处理
* 输入:     无
* 返回值:   无
* 创建人:   Yang Chao Xu
* 创建时间:  2014-8-22
\*********************************************************************/
void Uart_send_Thread(void)
{
    while(1)
    {
        if(Uart_Tx_Flag == 1)
        {
            Uart_Tx_Flag = 0;
            write(Uart_fd, Uart_Tx_Buff, Uart_Data_Len);
        }
        usleep(10 * 1000);
    }
    return;
}


/********************************************************************\
* 函数名: Uart_Pthread_Creat
* 说明:
* 功能:  串口的接受和发送线程创建
* 输入:
* 返回值:   0:成功非0:失败
* 创建人:   Yang Chao Xu
* 创建时间: 2014-8-22
\*********************************************************************/
int Uart_Pthread_Creat()
{
    int err;
    pthread_t receiveid, send;
    err = pthread_create(&receiveid, NULL, (void*)Uart_Receive_Thread_Select, NULL); //创建接收线程
    if(err != 0)
    {
        printf("can't create Uart_Receive thread thread: %s\n", strerror(err));
        return 1;
    }
    else
        printf("create Uart_Receive thread thread OK\n");
    err = pthread_create(&send, NULL, (void*)Uart_send_Thread, NULL); //创建发送线程
    if(err != 0)
    {
        printf("can't create Uart_send thread: %s\n", strerror(err));
        return 1;
    }
    else
        printf("create Uart_send thread OK\n");

    return 0;
}


/********************************************************************\
* 函数名：: Open_Uart
* 说明:
* 功能:     打开串口
* 输入:     串口号
* 返回值:   0:成功-1:失败
* 创建人:   Yang Chao Xu
* 创建时间:  2014-8-22
\*********************************************************************/
int Open_Uart(char *uart_no)
{
    Uart_fd= open(uart_no, O_RDWR);
    if(Uart_fd < 0)
    {
        printf("open device %s faild\n", uart_no);
        return(-1);
    }
    return 0;
}

void FCS_To_OPT_HeadandTail(Uchar cmdID,Ushort len)//帧头+帧尾
{
    Ushort crc;
    //帧头
    Uart_Tx_Buff[0] = 0xAA;       //起始符 AAH
    Uart_Tx_Buff[1] = 0xFF;       //起始符 FFH
    Uart_Tx_Buff[2] = conf.Gun_num;//枪号号 ，目标设备
    Uart_Tx_Buff[3] = 0x00;       //源设备
    Uart_Tx_Buff[4] = cmdID;      //命令字 01H~90H
    Uart_Tx_Buff[5] = len>>8;
    Uart_Tx_Buff[6] = (Uchar)len;
    //帧尾
    crc=ModbusCrc16(&Uart_Tx_Buff[2], len+5);
    Uart_Tx_Buff[len+7] = crc>>8;
    Uart_Tx_Buff[len+8] = (Uchar)crc;
    Uart_Tx_Buff[len+9] = 0xCC; //结束符CC
    Uart_Tx_Buff[len+10] = 0xFF; //结束符FF
    Uart_Tx_Flag=1;
    Uart_Data_Len=len+11;
}


//发送reboot dispenser命令
void SendRebootDispenser()
{
    Uchar tt=7;
    Uart_Tx_Buff[tt++]=0xAA;
    Uart_Tx_Buff[tt++]=0x00;
    Uart_Tx_Buff[tt++]=0xAA;
    Uart_Tx_Buff[tt++]=0x00;
    FCS_To_OPT_HeadandTail(0xAA,tt-7);    
}

//发送IAP命令
void SendIAP()
{
    Uchar tt=0;
    Uart_Tx_Buff[tt++]='I';
    Uart_Tx_Buff[tt++]='A';
    Uart_Tx_Buff[tt++]='P';
    Uart_Tx_Flag=1;
    Uart_Data_Len=tt;
}

//quit bootloader
void QuitBootloader()
{
    Uchar tt=0;
    Uart_Tx_Buff[tt++]='q';
    Uart_Tx_Flag=1;
    Uart_Data_Len=tt;
}
//获取版本号
void GetVersion()
{
    Uchar tt=7;
    Uart_Tx_Buff[tt++]=0xBB;
    Uart_Tx_Buff[tt++]=0x00;
    Uart_Tx_Buff[tt++]=0xBB;
    Uart_Tx_Buff[tt++]=0x00;
    FCS_To_OPT_HeadandTail(0xBB,tt-7); 
}
