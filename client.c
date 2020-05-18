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
#define true 1
#define false 0

char* IP = "127.0.0.1";
int sockfd;

typedef _Bool bool;
typedef struct sockaddr SA;

char name[50];
char filename[50];
char endfile[4096];
FILE* fp_recv = NULL;

// 客户端套接字初始化
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

// 文本解析，客户端向服务器发送文件
bool sendfile_or_not(char* src, char* filename)
{
	char* token;
	char str[100];
	strcpy(str, src);
	token = strtok(str, " ");
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

// 文本解析，客户端是否从服务器接收文件
bool recvfile_or_not(char* src, char* filename)
{
    char* token;
	char str[100];
	strcpy(str, src);
	token = strtok(str, " ");
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

// 客户端向服务器发送文件
void sendfile(char* filename)
{
	FILE* fp_read = fopen(filename,"rb"); 
	char buf[4096];
    if(fp_read == NULL)
    {
        printf("File:%s not found in current path\n",filename);
    }
    else
    {
        bzero(buf, sizeof(buf)); 
        int file_block_length=0;
        while((file_block_length = fread(buf, sizeof(char), sizeof(buf), fp_read))>0)  
        {  
            printf("file_block_length:%d\n", file_block_length);  
            if(send(sockfd, buf, file_block_length, 0) < 0)  
            {
                perror("Send");
                exit(1);
            }  
            bzero(buf, sizeof(buf));
        }  
        fclose(fp_read);  
        printf("Transfer file finished !\n");  
	}
}

// 姓名重复检测
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
		
		// 从键盘读入信息，并把换行符删除
        fgets(buf, sizeof(buf), stdin); 
		size_t ln = strlen(buf) - 1;
		if(*buf && buf[ln] == '\n')
			buf[ln] = '\0';
		
        char msg[131] = {};
        sprintf(msg, "%s: %s", name, buf);
        send(sockfd, msg, strlen(msg), 0);
		
		// 文本解析、是否发送文件
		bzero(filename, sizeof(filename));
		if(sendfile_or_not(buf, filename) == true){
			sendfile(filename);
		}
		
	    // 文本解析，是否接收文件
        bzero(filename, sizeof(filename));
		recvfile_or_not(buf, filename); 
	
		
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
    while(1)
	{
		// 接受来自服务端的信息
        char buf[4096] = {};
		int length = 0;
        length = recv(sockfd, buf, sizeof(buf), 0);
        if (length <= 0){
            return NULL;
        }
        // printf("length = %d\n", length);
		
		// 信息是文件信息，处理
		if(buf[0] == '!' && buf[1] == '#'){
			char filebuf[4096] = {};
			strcpy(filebuf, buf+2);
            if(fp_recv == NULL)
            {
                fp_recv = fopen(filename, "wb+");
                if(fp_recv == NULL){
                    perror("open");
                    exit(1);
                }
            }
			if(strcmp(filebuf, "endfile") == 0){
				printf("Receieved file:%s finished!\n", filename);
				fclose(fp_recv);
                fp_recv = NULL;
			}
            if(fp_recv != NULL){
                int write_len = fwrite(filebuf, sizeof(char), strlen(filebuf), fp_recv);
                if(write_len < strlen(filebuf)){
                    perror("write");
                    exit(1);
			    }   
            }
            bzero(filebuf, sizeof(filebuf));
		}
		// 信息是聊天信息，处理
		else if(buf[0] != '!' || buf[1] != '#'){
			printf("%s\n", buf);
		}
    }
}

int main(){
    strcpy(endfile, "!#endfile");
    init();
    start();
    return 0;
}
