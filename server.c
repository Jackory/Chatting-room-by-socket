/* This program is used for the course "Fundanmental of Computer II"
 * You may copy and revise the source code under GPL V2.0
 *
 * Copyright @ Stanley Peng pcl@nju.edu.cn 2018
 */
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define PORT 8000 
#define EMOJI_NUM 8
#define true 1
#define false 0

typedef struct {
    char name[30];
    int fd;
} User ;

typedef _Bool bool;

char* raw_str[] = {"smile","cry","happy","sad","like","dizzy","speechless","dull"};
char* emojis[] = {":-)","qwq","^v^",":-(","*v*","@_@","-_-#","o_o"};

int server_fd;

/* 客户端的socketfd, 100个元素, fds[0]~fds[99] */
int fds[100];
User users[100] = {};

/* 用来控制进入聊天室的人数为100以内 */
int size =100;
char* IP = "127.0.0.1";

typedef struct sockaddr SA;

void trans(char* str,char* temp,char* res){

    int len = strlen(str);

    int i = 0;
    int resIdx = 0;
    char c;
    while(i<len && str[i]!='\0'){
        c = str[i];
        if(c!='/'){
            res[resIdx] = c;
        }else{
            //待转义字符
            int j = i + 1;
            int tempi = 0;
            while(j < len && str[j]!=' ' && str[j]!='\0'){
                temp[tempi++] = str[j++];
            }
            temp[tempi] = '\0';

            int notchange = 1;
            for (int k = 0; k < EMOJI_NUM; ++k) {
                if(strcmp(temp,raw_str[k])==0){
                    notchange = 0;
                    char* emoji = emojis[k];
                    int ei = 0;
                    while(emoji[ei]!='\0'){
                        res[resIdx++]=emoji[ei++];
                    }
                    i = j-1;
                    --resIdx;
                    break;
                }
            }
            if(notchange){
                res[resIdx] = c;
            }


        }
        ++resIdx;
        ++i;
    }
    res[resIdx] = '\0';
}

// 解析文本，是否发送文件
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

void sendfile_to_client(char* filename, int fd)
{
	FILE* fp = fopen(filename,"rb"); 
	char buf[4096]; //读写缓冲区
    if(fp == NULL)
    {
        printf("File:%s not found in current path\n",filename);
    }
    else
    {
        bzero(buf, sizeof(buf));  //把缓冲区清0
        int file_block_length = 0;
        //每次读4096个字节，下一次读取时内部指针自动偏移到上一次读取到位置
        while((file_block_length = fread(buf, sizeof(char), sizeof(buf), fp))>0)  
        {  
            printf("file_block_length:%d\n", file_block_length);  
            //把每次从文件中读出来到128字节到数据发出去
            if(send(fd, buf, file_block_length,0)<0)  
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

void recvfile_from_client(char* filename, int client_fd)
{
    char buf[4096];
	FILE* fp=fopen(filename,"wb+");
	
    if(fp == NULL)
    {
        perror("open");
        exit(1);
    }
    bzero(buf,sizeof(buf)); //缓冲区清0

    int length=0;

    bool end = false; 
    while(length = recv(client_fd,buf,sizeof(buf),0)) //这里是分包接收，每次接收4096个字节
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

void init(){
    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (server_fd == -1){
        perror("创建socket失败");
        exit(-1);
    }
    struct sockaddr_in addr;
    addr.sin_family = PF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(IP);
    if (bind(server_fd, (SA*)&addr, sizeof(addr)) == -1){
        perror("绑定失败");
        exit(-1);
    }
    if (listen(server_fd, 100) == -1){
        perror("设置监听失败");
        exit(-1);
    }
}

void SendMsgToAll(char* msg, int fd){
    int i;
    for (i = 0;i < size;i++){
        if (fds[i] != 0 && fds[i]!=fd){
            printf("sendto%d\n", fds[i]);
            send(fds[i], msg, strlen(msg), 0);
        }
    }
}

void check_name(int fd)
{
    while(1){//姓名重复检测
        char buf[100] = {};
        int i = 0;
        if (recv(fd,buf,sizeof(buf),0) <= 0){
                printf("退出：fd = %dquit\n",fd);
                pthread_exit(&i);
        }

        for(i=0;i<size;i++){
            if(strcmp(buf, users[i].name)==0){
                memset(buf, 0, sizeof(buf));
                sprintf(buf,"姓名已存在");
                send(fd, buf, strlen(buf), 0);
                break;
            }
        }

        if(i == size){
    
            for (i = 0;i < size;i++){
                if (fds[i] == 0){
                    //记录客户端的socket
                    fds[i]=fd;
                    sprintf(users[i].name,"%s",buf);
                    break;
                }
            }
            memset(buf,0,sizeof(buf));
            sprintf(buf, "name is ok");
            send(fd,buf,strlen(buf),0);
            break;
        }
    }//姓名重复检测
}

void* service_thread(void* p){
    User* user = (User*) p;
    printf("pthread = %d\n", user->fd);

    check_name(user->fd);
    while(1){
        char buf[100] = {};
        if (recv(user->fd, buf, sizeof(buf), 0) <= 0){
            int i;
            for (i = 0;i < size;i++){
                if (user->fd == users[i].fd){
                    users[i].fd = 0;
                    bzero(users[i].name, sizeof(user[i].name));
                    break;
                }
            }
                printf("退出: fd = %dquit\n", user->fd);
                pthread_exit(&i);
        }
	    /* 转换服务器接受到的信息，如表情转换 */
	    char temp[100]={};
	    char res[200]={};
	    trans(buf,temp,res);
		
		// 解析文件名
		char filename[50];
		bzero(filename, sizeof(filename));
		if(sendfile_or_not(res, filename) == true){
			recvfile_from_client(filename, user->fd);
		}
        if(recvfile_or_not(res, filename) == true){
            sendfile_to_client(filename, user->fd);
        }
		// 写文件
        /* 把服务器接受到的信息发给所有的客户端 */
        SendMsgToAll(res, user->fd);
    }
}

void service(){
    printf("服务器启动\n");
    while(1){
        struct sockaddr_in clientaddr;
        socklen_t len = sizeof(clientaddr);
        int fd = accept(server_fd, (SA*)&clientaddr, &len);
        if (fd == -1){
            printf("客户端连接出错...\n");
            continue;
        }
        int i = 0;
        for (i = 0;i < size;i++){
            if (users[i].fd == 0){
                /* 记录客户端的socket */
                users[i].fd = fd;
                printf("fd = %d\n",users[i].fd);
                /* 有客户端连接之后，启动线程给此客户服务 */
                pthread_t tid;
                pthread_create(&tid,0, service_thread, &users[i]);
                break;
            }
            if (size == i){
                /* 发送给客户端说聊天室满了 */
                char* str = "对不起，聊天室已经满了!";
                send(fd, str, strlen(str), 0); 
                close(fd);
            }
        }
    }
}

int main(){
    init();
    service();
    return 0;
}
