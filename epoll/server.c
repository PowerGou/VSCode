#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
typedef struct socketInfo{
    int fd;
    int epfd;
}Sock_fo;

void* accept_con(void* arg) {
    printf("accept id:%ld\n",pthread_self());
    Sock_fo* info = (Sock_fo*)arg;

    int cli_fd = accept(info->fd, NULL, NULL);
    //设置非阻塞属性
    int flag = fcntl(cli_fd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cli_fd, F_SETFL, flag);
    struct epoll_event ev;
    //ev.events = EPOLLIN;
    ev.events = EPOLLIN | EPOLLET;//边沿触发
    ev.data.fd = cli_fd;
    epoll_ctl(info->epfd , EPOLL_CTL_ADD, cli_fd, &ev);

    free(info);
    return NULL;
}

void* call(void* arg) {
    printf("call id:%ld\n",pthread_self());
    Sock_fo* info = (Sock_fo*)arg;
    int fd = info->fd;
    int epfd = info->epfd;
    
    char recv_buf[5] = {0};
    while (1)
    {
        int len = recv(fd, recv_buf, sizeof(recv_buf),0);
        if (-1 == len) {
            if(errno == EAGAIN) {
                printf("数据已经接收完毕...\n");
                break;
            } else {
                perror("recv error");
                break;
            }
            
        } else if (0 == len) {
            printf("客户端已经断开连接\n");
            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
            close(fd);
            break;
        }
        printf("revc_buf:%s\n",recv_buf);
        for (int i = 0; i < len; i++) {
            recv_buf[i] = toupper(recv_buf[i]);
        }
        printf("up recv_buf:%s\n",recv_buf);

        int ret = send(fd, recv_buf, sizeof(recv_buf),0);
        if (-1 == ret) {
            perror("send");
            exit(1);
        }
    }
    free(info);
    return NULL;
}

int main()
{
    //创建监听套接字
    int ser_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == ser_fd) {
        perror("socket");
        exit(1);
    }

    //绑定
    struct sockaddr_in Ser_addr;
    memset(&Ser_addr, 0, sizeof(Ser_addr));
    Ser_addr.sin_family = AF_INET;
    Ser_addr.sin_port = htons(8000);
    Ser_addr.sin_addr.s_addr = INADDR_ANY;

    int ret = bind(ser_fd, (struct sockaddr*)&Ser_addr, sizeof(Ser_addr));
    if (-1 == ret) {
        perror("bind");
        exit(1);
    }

    //监听
    ret = listen(ser_fd, 64);
    if (-1 == ret) {
        perror("listen");
        exit(1);
    }

    //创建epoll示例
    int epfd = epoll_create(1);
    if (-1 == epfd) {
        perror("epooll_create");
        exit(1);
    }

    struct epoll_event ev;
    //ev.events = EPOLLIN; //水平触发
    ev.events = EPOLLIN | EPOLLET;//设置边沿模式
    ev.data.fd = ser_fd;
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, ser_fd, &ev);
    if (-1 == ret) {
        perror("epoll_ctl");
        exit(1);
    }

    struct epoll_event evs[1024];
    int size = sizeof(evs) / sizeof(evs[0]);
    while (1)
    {
        int num = epoll_wait(epfd, evs, size, -1);
        printf("num:%d\n",num);
        pthread_t con_id;
        pthread_t call_id;
        
        for (int i = 0; i < num; i++) {
            int fd = evs[i].data.fd;
            Sock_fo * info = (Sock_fo *)calloc(1,sizeof(Sock_fo));
            info->fd = fd;
            info->epfd = epfd;
            if (fd == ser_fd) //用于监听得描述符
            {
               
               pthread_create(&con_id, NULL, accept_con,info);
               pthread_detach(con_id);
            } else { //用于通信得描述符
                //char recv_buf[1024] = {0};
               pthread_create(&call_id, NULL, call,info);
                pthread_detach(call_id);
                 
            }
        }
    }
    close(ser_fd);
    return 0;
}