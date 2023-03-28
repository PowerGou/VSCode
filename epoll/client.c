#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

int main()
{
    //创建通信套接字
    int cli_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == cli_fd) {
        perror("socket failed!");
        return -1;
    }

    struct sockaddr_in Ser_addr;
    Ser_addr.sin_family = AF_INET;
    Ser_addr.sin_port = htons(8000);
    inet_pton(AF_INET, "192.168.80.148",&Ser_addr.sin_addr.s_addr);
    //2.连接服务器
    int ret = connect(cli_fd,(struct sockaddr*)&Ser_addr,sizeof(Ser_addr));
    if (-1 == ret) {
        perror("connect");
        return -1;
    }

    //5.通信
    char send_buf[1024] = {0};
    char recv_buf[1024] = {0};
    int number = 0;
    while (1)
    {
        memset(send_buf,0,sizeof(send_buf));
        memset(recv_buf,0,sizeof(recv_buf));
        //sprintf(send_buf,"hello world,%d...\n",number++);
        printf("请输入你要发送的数据：");
        fgets(send_buf, 1024 ,stdin);
        strtok(send_buf, "\n");
        //发送数据
        send(cli_fd, send_buf, strlen(send_buf) + 1, 0);

        //接收数据
        int len = recv(cli_fd, recv_buf, sizeof(recv_buf), 0);
        if (len > 0) {
            printf("server say:%s\n",recv_buf);
        } else if (0 == len) {
            printf("服务端已经断开连接！\n");
            break;
        } else {
            perror("recv");
            break;
        }
        sleep(1);

    }
    //关闭文件描述符
    close(cli_fd);
    return 0;
}