/* This program is used for the course "Fundanmental of Computer II"
 * You may copy and revise the source code under GPL V2.0
 *
 * Copyright @ Stanley Peng pcl@nju.edu.cn 2018
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>

#define PORT 8000 
typedef _Bool bool;
#define true 1
#define false 0

int sockfd;
char* IP = "127.0.0.1";
typedef struct sockaddr SA;
char name[30];

void init(){
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = PF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(IP);
    if (connect(sockfd, (SA*)&addr, sizeof(addr)) == -1){
        perror("无法连接到服务器");
        exit(-1);
    }
    printf("客户端启动成功\n");
}

bool sendfile_or_not(char* src, char* filename)
{
	char* token;
	/* 获取第一个子字符串 */
	char str[100];
	strcpy(str, src);
	token = strtok(str, " ");

	/* 继续获取其他的子字符串 */
	bool is_sendfile = false;
	while (token != NULL) {
		
		if (strcmp(token, "/sendfile") == 0) {
			is_sendfile = true;
		}
		token = strtok(NULL, " ");
		if (is_sendfile == true)
		{
			strcpy(filename, token);
			printf("filename is:%s\n", filename);
			return true;
		}
	}
	return false;
}

bool recvfile_or_not(char* src, char* filename)
{
    char* token;
	/* 获取第一个子字符串 */
	char str[100];
	strcpy(str, src);
	token = strtok(str, " ");

	/* 继续获取其他的子字符串 */
	bool is_recvfile = false;
	while (token != NULL) {	
		if (strcmp(token, "/recvfile") == 0) {
			is_recvfile = true;
		}
		token = strtok(NULL, " ");
		if (is_recvfile == true)
		{
			strcpy(filename, token);
			printf("filename is:%s\n", filename);
			return true;
		}
	}
	return false;
}

void sendfile(char* filename)
{
    // Get the size of file
    // struct stat statbuf;
    // stat(filename, &statbuf);
    // char filesize[50] = {};
    // sprintf(filesize, "%ld", statbuf.st_size);
    // send(sockfd, filesize, sizeof(filesize), 0);

	FILE* fp = fopen(filename,"rb"); 
	char buf[4096]; //读写缓冲区
    if(fp == NULL)
    {
        printf("File:%s not found in current path\n",filename);
    }
    else
    {
        bzero(buf, sizeof(buf));  //把缓冲区清0
        int file_block_length=0;
        //每次读4096个字节，下一次读取时内部指针自动偏移到上一次读取到位置
        while((file_block_length=fread(buf, sizeof(char),sizeof(buf),fp))>0)  
        {  
            printf("file_block_length:%d\n", file_block_length);  
            //把每次从文件中读出来到128字节到数据发出去
            if(send(sockfd, buf, file_block_length, 0) < 0)  
            {  
                perror("Send");  
                exit(1);  
            }  
            bzero(buf, sizeof(buf));//发送一次数据之后把缓冲区清零
        }  
        fclose(fp);  
        printf("Transfer file finished !\n");  
	}
}

void recvfile(char* filename)
{
	char buf[4096];
	FILE* fp = fopen(filename,"wb+");
	
    if(fp == NULL)
    {
        perror("open");
        exit(1);
    }
    bzero(buf,sizeof(buf)); //缓冲区清0

    int length = 0;

    bool end = false; 
    while(length = recv(sockfd, buf, sizeof(buf), 0)) //这里是分包接收，每次接收4096个字节
    {
        if(length<0)
        {
            perror("recv");
            exit(1);
        }

        if(length < sizeof(buf)){
            end = true;
        }

        //把从buf接收到的字符写入（二进制）文件中
        int writelen=fwrite(buf,sizeof(char),length,fp);
        if(writelen<length)
        {
            perror("write");
            exit(1);
        }
        if(end == true){
            end = false;
            break;
        }
        bzero(buf,sizeof(buf)); //每次写完缓冲清0，准备下一次的数据的接收
    }
    printf("Receieved file:%s finished!\n", filename);
    fclose(fp);
}

void check_name()
{
	char buf2[100] = {};
    printf("请输入您的名字：");
    scanf("%s", name);
    send(sockfd, name, strlen(name), 0);
    recv(sockfd, buf2, sizeof(buf2), 0);
	printf("%s\n", buf2);
    while(strcmp(buf2,"name is ok")!=0){
	   printf("\n请输入您的名字：");
           scanf("%s",name);
           send(sockfd,name,strlen(name),0);
	       memset(buf2,0,sizeof(buf2));
           recv(sockfd,buf2,sizeof(buf2),0);
	       printf("%s\n",buf2);

    }
}


void start(){

    check_name();
    pthread_t id;
    void* recv_thread(void*);
    pthread_create(&id, 0, recv_thread, 0);
    char buf2[100] = {};
    sprintf(buf2, "%s进入了聊天室", name);
    send(sockfd, buf2, strlen(buf2), 0);
    while(1){
        char buf[100] = {};
        fgets(buf, sizeof(buf), stdin);  //Compared to scanf(), fgets() can read space
		size_t ln = strlen(buf) - 1;
		if(*buf && buf[ln] == '\n') // delete the newline of buf (fgets's defect)
			buf[ln] = '\0';

        char msg[131] = {};
        sprintf(msg, "%s: %s", name, buf);
        send(sockfd, msg, strlen(msg), 0);
		
		// filename 解析
		char filename[50];
		bzero(filename, sizeof(filename));
		if(sendfile_or_not(buf, filename) == true){
			sendfile(filename);
		}
        bzero(filename, sizeof(filename));
		if(recvfile_or_not(buf, filename) == true){
			recvfile(filename);
		}
		
        if (strcmp(buf, "bye") == 0){
            memset(buf2, 0, sizeof(buf2));
            sprintf(buf2, "%s退出了聊天室", name);
            send(sockfd, buf2, strlen(buf2), 0);
            break;
        }
    }
    close(sockfd);
}

void* recv_thread(void* p){
    while(1){
        char buf[100] = {};
        if (recv(sockfd, buf, sizeof(buf), 0) <= 0){
            return NULL;
        }
        printf("%s\n", buf);
    }
}

int main(){
    init();
    start();
    return 0;
}
