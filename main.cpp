#include<iostream>
#include<cstdio>
#include<Winsock2.h>
#include<ctime>
#include<string>
#include <ws2tcpip.h>
#include <time.h>
#include <ctype.h>
#define  STR "********* "
#define SIO_RCVALL            _WSAIOW(IOC_VENDOR,1)

using namespace std;
#pragma comment(lib,"ws2_32.lib")//加载dll；
struct Ip {
    unsigned char Version_HLen;    //4位版本号和4位首部长度；
    unsigned char TOS;             //位服务类型TOS
    unsigned short Length;         //总长度
    unsigned short Ident;          //标识
    unsigned short Flags_Offset;   //标志位
    unsigned char TTL;             //生存时间TTL
    unsigned char Protocol;        //协议类型
    unsigned short Checksum;        //IP首部检验和
    unsigned int SourceAddr;     //源IP地址
    unsigned int DestinationAddr;//目的地址
};

struct Tcp {
    unsigned short SrcPort;        //16位源端口
    unsigned short DstPort;        //16位目的端口
    unsigned int SequenceNum;      //32位序号
    unsigned int Acknowledgment;   //32位确认序号
    unsigned char HdrLen;          //首部长度保留字
    unsigned char Flags;           //6位标志位
    unsigned short AdvertisedWindow;//16位窗口长度
    unsigned short Checksum;       //16位校验和
    unsigned short UrgPtr;         //16位紧急指针
};
void PrintIpHead(Ip* ip);//将抓取的Ip打印出来；
void PrintTcpHead(Tcp* tcp);//将抓取的TCP打印出来；

int main() {
    WSADATA wsadata;
    int r = WSAStartup(MAKEWORD(2,2),&wsadata);//选取库版本，
    if(r == SOCKET_ERROR) {
        printf("加载GG了\n");
        return -1;
    }

    SOCKET Socket = socket(AF_INET,SOCK_RAW,IPPROTO_IP);//創建原始套接字
    if(r == SOCKET_ERROR) {
        printf("套接字创建GG了\n");
        return -1;
    }
    char Name[255];//存放主机名的缓冲区；
    r = gethostname(Name,255);//获取主机名，第一个参数为放主机名的缓冲区的指针，第二个参数为缓冲区长度；
    if(r == SOCKET_ERROR) {
        printf("获取主机名GG了\n");
        return -1;
    }
    /*结构记录主机的信息，包括主机名、别名、地址类型、地址长度和地址列表;
    struct hostent {
        char * h_name;
        char ** h_aliases;
        short h_addrtype;
        short h_length;
        char ** h_addr_list;
        #define h_addr h_addr_list[0];
    };*/
    struct hostent* pHostent;
    pHostent = (struct hostent*)malloc(sizeof(struct hostent));
    pHostent = gethostbyname(Name);//返回给定的主机名的信息存进结构体pHostent中；
    /*
    struct sockaddr_in
    {

        short sin_family;//Address family一般来说AF_INET（地址族）PF_INET（协议族）

        unsigned short sin_port;//Port number(必须要采用网络数据格式,普通数字可以用htons()函数转换成网络数据格式的数字)

        struct in_addr sin_addr;//IP address in network byte order（Internet address）

        unsigned char sin_zero[8];//Same size as struct sockaddr没有实际意义,只是为了　跟SOCKADDR结构在内存中对齐

    };
    */

    sockaddr_in sock;
    sock.sin_family = AF_INET;//定义AF_INET地址族，代表TCP、IP协议；
    sock.sin_port = htons(8000);//无实际意义，瞎定义就行；但必须将普通数字转成网络数据格式的数字
    memcpy(&sock.sin_addr.S_un.S_addr,pHostent->h_addr_list[0],pHostent->h_length);//给ip赋值；

    /*bind( SOCKET sockaddr, const struct sockaddr FAR* my_addr,int addrlen);
    sockfd 表示已经建立的socket编号（描述符）；
    my_addr 是一个指向sockaddr结构体类型的指针；
    参数addrlen表示my_addr结构的长度，可以用sizeof操作符获得。
    */

    r = bind(Socket,(PSOCKADDR)&sock,sizeof(sock));
    if(r == SOCKET_ERROR) {
        printf("绑定套接字GG了\n");
        return -1;
    }

    /*网卡设置为混杂模式*/

    DWORD dwBufferLen[10];//注册表；
    DWORD dwBufferInLen = 1;
    DWORD dwBytesReturned = 0;
    r = WSAIoctl(Socket,SIO_RCVALL,&dwBufferInLen,sizeof(dwBufferInLen),&dwBufferLen,sizeof(dwBufferLen),&dwBytesReturned,NULL,NULL);
    if(r == SOCKET_ERROR) {
        printf("注册表设置失败\n");
        return -1;
    }
    /*句柄*/
    HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO bInfo;
    GetConsoleScreenBufferInfo(hCon,&bInfo);

    char Buffer[65535] = {0};//建立缓冲区存放socket的数据；
    cout<<"**********************CATCHING THE PACKAGE ************************"<<endl;
    while(true) {

        memset(Buffer,0,sizeof(Buffer));//清除Buffer缓冲区内容

        /*int recv( _In_ SOCKET s, _Out_ char *buf, _In_ int len, _In_ int flags);
        （1）recv先等待s的发送缓冲中的数据被协议传送完毕，如果协议在传送s的发送缓冲中
        的数据时出现网络错误，那么recv函数返回SOCKET_ERROR；
        （2）如果s的发送缓冲中没有数据或者数据被协议成功发送完毕后，recv先检查套接字s
        的接收缓冲区，如果s接收缓冲区中没有数据或者协议正在接收数据，那么recv就一直等
        待，直到协议把数据接收完毕。当协议把数据接收完毕，recv函数就把s的接收缓冲中的
        数据copy到buf中（注意协议接收到的数据可能大于buf的长度，所以在这种情况下要调
        用几次recv函数才能把s的接收缓冲中的数据copy完。recv函数仅仅是copy数据，真正的
        接收数据是协议来完成的）；
        (3)recv函数返回其实际copy的字节数。如果recv在copy时出错，那么它返回SOCKET_ERROR；
        如果recv函数在等待协议接收数据时网络中断了，那么它返回0。
        */
        r = recv(Socket,Buffer,sizeof(Buffer),0);//接受的socket向Buffer中放入
        if(r == SOCKET_ERROR) {
            printf("COPY到Buffer缓冲区的数据GG了\n");
            return -1;
        }

        struct Ip *ip = (struct Ip*)Buffer;//将其强制转换成ip的类型
        int Protocol = ip->Protocol;


        if( Protocol == 6) { //过滤只显示上层协议为TCP的报文
            cout<<("*********************Package Caught***********************\n")<<endl;
            cout<<("IP：\n");
            PrintIpHead(ip); //打印IP首部
            int IpHeaderLength = (ip->Version_HLen&0x0f)*4;
            /*用一条语句来指向TCP头，因为接收的数据中，IP头的大小是固定的4字节，
            所以用IP长度乘以4就能指向TCP头部分*/
            if(Protocol == 6) {
                cout<<("--------------------------------------------------------------------------------");
                printf("\nTCP:\n");
                struct Tcp *tcp = (struct Tcp*)(Buffer+IpHeaderLength);
                PrintTcpHead(tcp); //打印TCP首部
            }

            struct Tcp *tcp = (struct Tcp*)(Buffer+IpHeaderLength);
            int IpTotalLength = ntohs(ip->Length);
            /* ntohs()是将一个无符号长整形数从网络字节顺序转换为主机字节顺序，
            ntohl()返回一个以主机字节顺序表达的数。*/
            int IpDataLength = IpTotalLength - IpHeaderLength;//总长度-头部长度=数据长度；
            int TcpDataLength =  IpDataLength - (tcp->HdrLen>>4)*4;
            if(IpDataLength > 0) {
                cout<<("Data as followings：\n");
                int TcpHrL = (tcp->HdrLen>>4)*4;
                unsigned char* IpData = (unsigned char*)Buffer+IpHeaderLength+TcpHrL; //将指针放置到TCP数据部分
                cout<<IpData<<endl;
            }

        }
    }
    if(closesocket(Socket) == SOCKET_ERROR) {
        printf("关闭套接字GG了\n");
        return 0;
    }
    if(WSACleanup() == SOCKET_ERROR) {
        printf("库关闭GG了\n");
        return 0;
    }



    return 0;
}
void PrintIpHead(Ip* ip) {
    printf("Version：    %d\n",ip->Version_HLen>>4);
    printf("HLen：  %d\n",(ip->Version_HLen&0x0f)*4);
    printf("TOS:%d\n",ip->TOS);
    printf("Length：    %d\n",ntohs(ip->Length));
    printf("Ident：%d\n",ntohs(ip->Ident));
    printf("TTL:   %d\n",ip->TTL);
    int p = ip->Protocol;
    switch(p) {
    case 1:
        printf("Protocol：  %s\n","ICMP");
        break;
    case 6:
        printf("Protocol：  %s\n","TCP");
        break;
    case 17:
        printf("Protocol： %s\n","UDP");
        break;
    default:
        printf("Protocol：  %d\n",ip->Protocol);
    }
    //printf("校验和：    %02X\n",ntohs(ip->Checksum));
    SOCKADDR_IN addr;
    addr.sin_addr.s_addr = ip->SourceAddr;
    printf("SourceAddr：  %s\n",inet_ntoa(addr.sin_addr));
    addr.sin_addr.s_addr = ip->DestinationAddr;
    printf("DestinationAddr: %s\n",inet_ntoa(addr.sin_addr));
}

void PrintTcpHead(Tcp* tcp) {
    printf("SrcPort：  %d\n",ntohs(tcp->SrcPort));
    printf("DstPort: %d\n",ntohs(tcp->DstPort));
    printf("SequenceNum：    %u\n",ntohl(tcp->SequenceNum));
    printf("Acknowledgment：    %u\n",ntohl(tcp->Acknowledgment));
    printf("HdrLen:   %d\n",(tcp->HdrLen>>4)*4);
    printf("Flags：\n");
    printf("URG:        ..%d.....\n",(tcp->Flags&0x20)>0?1:0);
    printf("ACK:        ...%d....\n",(tcp->Flags&0x10)>0?1:0);
    printf("PSH:        ....%d...\n",(tcp->Flags&0x08)>0?1:0);
    printf("RST:        .....%d..\n",(tcp->Flags&0x04)>0?1:0);
    printf("SYN:        ......%d.\n",(tcp->Flags&0x02)>0?1:0);
    printf("FIN:        .......%d\n",(tcp->Flags&0x01)>0?1:0);
    printf("AdvertisedWindow：  %d\n",ntohs(tcp->AdvertisedWindow));
    printf("Checksum：    %02X\n",ntohs(tcp->Checksum));
    printf("UrgPtr：  %d\n",ntohs(tcp->UrgPtr));
}

