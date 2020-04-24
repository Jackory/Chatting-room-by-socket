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

#define PORT  YOUR_CODE 

int sockfd;
char* IP = "YOUR_CODE";
typedef struct sockaddr SA;
char name[30];

void init(){
    sockfd = YOUR_CODE;
    struct YOUR_CODE addr;
    addr.sin_family = YOUR_CODE;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(IP);
    if (connect(sockfd, (SA*)&addr, sizeof(addr)) == -1){
        perror("无法连接到服务器");
        exit(-1);
    }
    printf("客户端启动成功\n");
}

void start(){
    pthread_t id;
    void* recv_thread(void*);
    pthread_create(&id, 0, recv_thread, 0);
    char buf2[100] = {};
    sprintf(buf2, "%s进入了聊天室", name);
    YOUR_CODE(sockfd, buf2, strlen(buf2), 0);
    while(1){
        char buf[100] = {};
        scanf("%s",buf);
        char msg[131] = {};
        sprintf(msg, "%s:%s", name, buf);
        YOUR_CODE(sockfd, msg, strlen(msg), 0);
        if (strcmp(buf, "bye") == 0){
            memset(buf2, 0, sizeof(buf2));
            sprintf(buf2, "%s退出了聊天室", name);
            YOUR_CODE(sockfd, buf2, strlen(buf2), 0);
            break;
        }
    }
    YOUR_CODE;
}

void* recv_thread(void* p){
    while(1){
        char buf[100] = {};
        if (YOUR_CODE(sockfd, buf, sizeof(buf), 0) <= 0){
            return NULL;
        }
        printf("%s\n", buf);
    }
}

int main(){
    init();
    printf("请输入您的名字：");
    scanf("%s", name);
    start();
    return 0;
}
