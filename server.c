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

// 定义结构体用户，成员变量为姓名与套接字
typedef struct {
    char name[30];
    int fd;
} User ;
User users[100] = {};

// 类型别名
typedef _Bool bool;
typedef struct sockaddr SA;

// 表情包转换
char* raw_str[] = {"smile","cry","happy","sad","like","dizzy","speechless","dull"};
char* emojis[] = {":-)","qwq","^v^",":-(","*v*","@_@","-_-#","o_o"};

// 服务器套接字
int server_fd;

// 客户端的socketfd, 100个元素, fds[0]~fds[99]
int fds[100];

// 文件传输全局变量，文件名、文件指针
char filename[30];
FILE* fp_recv = NULL;

// 用来控制进入聊天室的人数为100以内 
int size =100;
char* IP = "127.0.0.1";

//表情包转换
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

// 解析文本，检测客户端是否即将发送文件到服务器
bool sendfile_or_not(char* src)
{
	char* token;
	char str[100];
	strcpy(str, src);
    puts(str);
	token = strtok(str, " ");
	bool is_sendfile = false;
	while (token != NULL) {
		if (strcmp(token, "/sendfile") == 0) {
			is_sendfile = true;
		}
		token = strtok(NULL, " ");
		if (is_sendfile == true)
		{
            puts(token);
            bzero(filename, sizeof(filename));
			strcpy(filename, token);
			printf("to receive filename is: %s\n", filename);
			return true;
		}
	}
	return false;
}

// 解析文本，检测客户端是否即将从服务器接收文件
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

// 服务器发送文件到客户端
void sendfile_to_client(char* filename_read, int fd)
{
	FILE* fp_read = fopen(filename_read,"rb"); 
	char buf[4090]; //读写缓冲区
	char filebuf[4096]; //传输文件缓冲区
    if(fp_read == NULL)
    {
        printf("File:%s not found in current path\n",filename);
    }
    else
    {
        bzero(buf, sizeof(buf));  
        int file_block_length = 0;
        while((file_block_length = fread(buf, sizeof(char), sizeof(buf), fp_read))>0)  
        {
			strcpy(filebuf, "!#");  //文件传输信息标记前缀"!#"，以区分
			strcat(filebuf, buf);
            printf("file_block_length:%d\n", file_block_length);  
            if(send(fd, filebuf, sizeof(filebuf),0)<0)  
            {  
                perror("Send");
                exit(1); 
            }
            bzero(buf, sizeof(buf));
			bzero(filebuf, sizeof(filebuf));
        }
		// 发送endfile，以标志文件传输结束
		char endfile[4096];
        strcpy(endfile, "!#endfile");
		send(fd, endfile, sizeof(endfile), 0);
		
        fclose(fp_read);  
        printf("Transfer file finished !\n");
	}
}

// 服务器套接字初始化
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

// 检测姓名是否重复
void check_name(int fd)
{
    while(1){
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
    }
}

// 客户端对应服务器线程，接收来自客户端的信息并处理
void* service_thread(void* p){
	
    User* user = (User*) p;
    printf("pthread = %d\n", user->fd);
	
	// 检查姓名是否存在
    check_name(user->fd);
	
    while(1){
		
		// 接收来自客户端信息
        char buf[4096] = {};
        bzero(buf, sizeof(buf));
        if (recv(user->fd, buf, sizeof(buf), 0) <= 0){
            printf("退出: fd = %d quit\n", user->fd);
            int i;
            for (i = 0;i < size;i++){
                if (user->fd == users[i].fd){
                    users[i].fd = 0;
                    bzero(users[i].name, sizeof(user[i].name));
                    break;
                }
            }
            pthread_exit(&i);
        }
        
		// 接收信息是文件信息，处理
        if(buf[0] == '!' && buf[1] == '#'){
            char filebuf[4096] = {};
            bzero(filebuf, sizeof(filebuf));
			strcpy(filebuf, buf+2);
            if(fp_recv == NULL)
            {
                puts("file opening");
                fp_recv = fopen(filename, "wb+");
                puts("file open succ");
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
        }
		
		//接收信息是聊天信息，处理
        else if(buf[0] != '!' || buf[1] != '#'){
			
            // 文本解析，客户端是否发送文件
            sendfile_or_not(buf);
		
			// 文本解析，客户端是否接受文件
            char recvfilename[50];
            if(recvfile_or_not(buf, recvfilename) == true){
                sendfile_to_client(recvfilename, user->fd);
            }
			
			//表情信息转换
            char temp[100]={};
            char res[200]={};
            trans(buf,temp,res);

			// 发送聊天信息到所有客户端
            SendMsgToAll(res, user->fd);
        }
    }
}

// 检测服务器是否连接
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
