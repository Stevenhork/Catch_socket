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
#pragma comment(lib,"ws2_32.lib")//����dll��
struct Ip {
    unsigned char Version_HLen;    //4λ�汾�ź�4λ�ײ����ȣ�
    unsigned char TOS;             //λ��������TOS
    unsigned short Length;         //�ܳ���
    unsigned short Ident;          //��ʶ
    unsigned short Flags_Offset;   //��־λ
    unsigned char TTL;             //����ʱ��TTL
    unsigned char Protocol;        //Э������
    unsigned short Checksum;        //IP�ײ������
    unsigned int SourceAddr;     //ԴIP��ַ
    unsigned int DestinationAddr;//Ŀ�ĵ�ַ
};

struct Tcp {
    unsigned short SrcPort;        //16λԴ�˿�
    unsigned short DstPort;        //16λĿ�Ķ˿�
    unsigned int SequenceNum;      //32λ���
    unsigned int Acknowledgment;   //32λȷ�����
    unsigned char HdrLen;          //�ײ����ȱ�����
    unsigned char Flags;           //6λ��־λ
    unsigned short AdvertisedWindow;//16λ���ڳ���
    unsigned short Checksum;       //16λУ���
    unsigned short UrgPtr;         //16λ����ָ��
};
void PrintIpHead(Ip* ip);//��ץȡ��Ip��ӡ������
void PrintTcpHead(Tcp* tcp);//��ץȡ��TCP��ӡ������

int main() {
    WSADATA wsadata;
    int r = WSAStartup(MAKEWORD(2,2),&wsadata);//ѡȡ��汾��
    if(r == SOCKET_ERROR) {
        printf("����GG��\n");
        return -1;
    }

    SOCKET Socket = socket(AF_INET,SOCK_RAW,IPPROTO_IP);//����ԭʼ�׽���
    if(r == SOCKET_ERROR) {
        printf("�׽��ִ���GG��\n");
        return -1;
    }
    char Name[255];//����������Ļ�������
    r = gethostname(Name,255);//��ȡ����������һ������Ϊ���������Ļ�������ָ�룬�ڶ�������Ϊ���������ȣ�
    if(r == SOCKET_ERROR) {
        printf("��ȡ������GG��\n");
        return -1;
    }
    /*�ṹ��¼��������Ϣ����������������������ַ���͡���ַ���Ⱥ͵�ַ�б�;
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
    pHostent = gethostbyname(Name);//���ظ���������������Ϣ����ṹ��pHostent�У�
    /*
    struct sockaddr_in
    {

        short sin_family;//Address familyһ����˵AF_INET����ַ�壩PF_INET��Э���壩

        unsigned short sin_port;//Port number(����Ҫ�����������ݸ�ʽ,��ͨ���ֿ�����htons()����ת�����������ݸ�ʽ������)

        struct in_addr sin_addr;//IP address in network byte order��Internet address��

        unsigned char sin_zero[8];//Same size as struct sockaddrû��ʵ������,ֻ��Ϊ�ˡ���SOCKADDR�ṹ���ڴ��ж���

    };
    */

    sockaddr_in sock;
    sock.sin_family = AF_INET;//����AF_INET��ַ�壬����TCP��IPЭ�飻
    sock.sin_port = htons(8000);//��ʵ�����壬Ϲ������У������뽫��ͨ����ת���������ݸ�ʽ������
    memcpy(&sock.sin_addr.S_un.S_addr,pHostent->h_addr_list[0],pHostent->h_length);//��ip��ֵ��

    /*bind( SOCKET sockaddr, const struct sockaddr FAR* my_addr,int addrlen);
    sockfd ��ʾ�Ѿ�������socket��ţ�����������
    my_addr ��һ��ָ��sockaddr�ṹ�����͵�ָ�룻
    ����addrlen��ʾmy_addr�ṹ�ĳ��ȣ�������sizeof��������á�
    */

    r = bind(Socket,(PSOCKADDR)&sock,sizeof(sock));
    if(r == SOCKET_ERROR) {
        printf("���׽���GG��\n");
        return -1;
    }

    /*��������Ϊ����ģʽ*/

    DWORD dwBufferLen[10];//ע���
    DWORD dwBufferInLen = 1;
    DWORD dwBytesReturned = 0;
    r = WSAIoctl(Socket,SIO_RCVALL,&dwBufferInLen,sizeof(dwBufferInLen),&dwBufferLen,sizeof(dwBufferLen),&dwBytesReturned,NULL,NULL);
    if(r == SOCKET_ERROR) {
        printf("ע�������ʧ��\n");
        return -1;
    }
    /*���*/
    HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO bInfo;
    GetConsoleScreenBufferInfo(hCon,&bInfo);

    char Buffer[65535] = {0};//�������������socket�����ݣ�
    cout<<"**********************CATCHING THE PACKAGE ************************"<<endl;
    while(true) {

        memset(Buffer,0,sizeof(Buffer));//���Buffer����������

        /*int recv( _In_ SOCKET s, _Out_ char *buf, _In_ int len, _In_ int flags);
        ��1��recv�ȵȴ�s�ķ��ͻ����е����ݱ�Э�鴫����ϣ����Э���ڴ���s�ķ��ͻ�����
        ������ʱ�������������ôrecv��������SOCKET_ERROR��
        ��2�����s�ķ��ͻ�����û�����ݻ������ݱ�Э��ɹ�������Ϻ�recv�ȼ���׽���s
        �Ľ��ջ����������s���ջ�������û�����ݻ���Э�����ڽ������ݣ���ôrecv��һֱ��
        ����ֱ��Э������ݽ�����ϡ���Э������ݽ�����ϣ�recv�����Ͱ�s�Ľ��ջ����е�
        ����copy��buf�У�ע��Э����յ������ݿ��ܴ���buf�ĳ��ȣ����������������Ҫ��
        �ü���recv�������ܰ�s�Ľ��ջ����е�����copy�ꡣrecv����������copy���ݣ�������
        ����������Э������ɵģ���
        (3)recv����������ʵ��copy���ֽ��������recv��copyʱ������ô������SOCKET_ERROR��
        ���recv�����ڵȴ�Э���������ʱ�����ж��ˣ���ô������0��
        */
        r = recv(Socket,Buffer,sizeof(Buffer),0);//���ܵ�socket��Buffer�з���
        if(r == SOCKET_ERROR) {
            printf("COPY��Buffer������������GG��\n");
            return -1;
        }

        struct Ip *ip = (struct Ip*)Buffer;//����ǿ��ת����ip������
        int Protocol = ip->Protocol;


        if( Protocol == 6) { //����ֻ��ʾ�ϲ�Э��ΪTCP�ı���
            cout<<("*********************Package Caught***********************\n")<<endl;
            cout<<("IP��\n");
            PrintIpHead(ip); //��ӡIP�ײ�
            int IpHeaderLength = (ip->Version_HLen&0x0f)*4;
            /*��һ�������ָ��TCPͷ����Ϊ���յ������У�IPͷ�Ĵ�С�ǹ̶���4�ֽڣ�
            ������IP���ȳ���4����ָ��TCPͷ����*/
            if(Protocol == 6) {
                cout<<("--------------------------------------------------------------------------------");
                printf("\nTCP:\n");
                struct Tcp *tcp = (struct Tcp*)(Buffer+IpHeaderLength);
                PrintTcpHead(tcp); //��ӡTCP�ײ�
            }

            struct Tcp *tcp = (struct Tcp*)(Buffer+IpHeaderLength);
            int IpTotalLength = ntohs(ip->Length);
            /* ntohs()�ǽ�һ���޷��ų��������������ֽ�˳��ת��Ϊ�����ֽ�˳��
            ntohl()����һ���������ֽ�˳���������*/
            int IpDataLength = IpTotalLength - IpHeaderLength;//�ܳ���-ͷ������=���ݳ��ȣ�
            int TcpDataLength =  IpDataLength - (tcp->HdrLen>>4)*4;
            if(IpDataLength > 0) {
                cout<<("Data as followings��\n");
                int TcpHrL = (tcp->HdrLen>>4)*4;
                unsigned char* IpData = (unsigned char*)Buffer+IpHeaderLength+TcpHrL; //��ָ����õ�TCP���ݲ���
                cout<<IpData<<endl;
            }

        }
    }
    if(closesocket(Socket) == SOCKET_ERROR) {
        printf("�ر��׽���GG��\n");
        return 0;
    }
    if(WSACleanup() == SOCKET_ERROR) {
        printf("��ر�GG��\n");
        return 0;
    }



    return 0;
}
void PrintIpHead(Ip* ip) {
    printf("Version��    %d\n",ip->Version_HLen>>4);
    printf("HLen��  %d\n",(ip->Version_HLen&0x0f)*4);
    printf("TOS:%d\n",ip->TOS);
    printf("Length��    %d\n",ntohs(ip->Length));
    printf("Ident��%d\n",ntohs(ip->Ident));
    printf("TTL:   %d\n",ip->TTL);
    int p = ip->Protocol;
    switch(p) {
    case 1:
        printf("Protocol��  %s\n","ICMP");
        break;
    case 6:
        printf("Protocol��  %s\n","TCP");
        break;
    case 17:
        printf("Protocol�� %s\n","UDP");
        break;
    default:
        printf("Protocol��  %d\n",ip->Protocol);
    }
    //printf("У��ͣ�    %02X\n",ntohs(ip->Checksum));
    SOCKADDR_IN addr;
    addr.sin_addr.s_addr = ip->SourceAddr;
    printf("SourceAddr��  %s\n",inet_ntoa(addr.sin_addr));
    addr.sin_addr.s_addr = ip->DestinationAddr;
    printf("DestinationAddr: %s\n",inet_ntoa(addr.sin_addr));
}

void PrintTcpHead(Tcp* tcp) {
    printf("SrcPort��  %d\n",ntohs(tcp->SrcPort));
    printf("DstPort: %d\n",ntohs(tcp->DstPort));
    printf("SequenceNum��    %u\n",ntohl(tcp->SequenceNum));
    printf("Acknowledgment��    %u\n",ntohl(tcp->Acknowledgment));
    printf("HdrLen:   %d\n",(tcp->HdrLen>>4)*4);
    printf("Flags��\n");
    printf("URG:        ..%d.....\n",(tcp->Flags&0x20)>0?1:0);
    printf("ACK:        ...%d....\n",(tcp->Flags&0x10)>0?1:0);
    printf("PSH:        ....%d...\n",(tcp->Flags&0x08)>0?1:0);
    printf("RST:        .....%d..\n",(tcp->Flags&0x04)>0?1:0);
    printf("SYN:        ......%d.\n",(tcp->Flags&0x02)>0?1:0);
    printf("FIN:        .......%d\n",(tcp->Flags&0x01)>0?1:0);
    printf("AdvertisedWindow��  %d\n",ntohs(tcp->AdvertisedWindow));
    printf("Checksum��    %02X\n",ntohs(tcp->Checksum));
    printf("UrgPtr��  %d\n",ntohs(tcp->UrgPtr));
}

