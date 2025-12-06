#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024  // 定义缓冲区大小

int main(int argc, char *argv[])
{
 int sockfd;
 int len;
 struct sockaddr_in address;
 int result;
 char buffer[BUFFER_SIZE];  // 缓冲区，用于存储终端输入和服务器返回数据
 char ch = 'A';

 //申请一个流 socket
 sockfd = socket(AF_INET, SOCK_STREAM, 0);
 //填充地址结构，指定服务器的 IP 和 端口
 address.sin_family = AF_INET;
 //inet_addr 可以参考 man inet_addr
 //可以用现代的inet_pton()替代inet_addr(), example 中有参考例子
 address.sin_addr.s_addr = inet_addr("127.0.0.1");
 address.sin_port = htons(8123);
 len = sizeof(address);
 
 //下面的语句可以输出连接的 IP 地址
 //但是inet_ntoa()是过时的方法，应该改用 inet_ntop(可参考 example)。但很多代码仍然遗留着inet_ntoa.
 printf("访问的服务器地址：%s\n", inet_ntoa( address.sin_addr));
 
 result = connect(sockfd, (struct sockaddr *)&address, len);

 if (result == -1)
 {
  perror("oops: client1");
  exit(1);
 }
 
 //往服务端写一个字节
 //write(sockfd, &ch, 1);
 //从服务端读一个字符
 //read(sockfd, &ch, 1);
 //printf("char from server = %c\n", ch);

 printf("已连接到服务器，输入消息发送（输入 exit 退出）：\n");

    // 循环读取终端输入并发送到服务器
    while (1) {
        // 读取终端输入（最多 BUFFER_SIZE-1 字节，留一个位置存结束符）
        printf("@this_client> ");
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            perror("fgets error");
            break;
        }

        // 移除 fgets 读取的换行符（可选，根据服务器处理逻辑调整）
        buffer[strcspn(buffer, "\n")] = '\0';

        // 如果输入 "exit"，退出循环
        if (strcmp(buffer, "exit") == 0) {
            printf("退出连接...\n");
            break;
        }

        // 发送数据到服务器
        if (send(sockfd, buffer, strlen(buffer) + 1, 0) == -1) {  // +1 包含结束符
            perror("send error");
            break;
        }

        // 接收服务器返回的数据
        ssize_t n = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if (n <= 0) {
            if (n == 0) {
                printf("服务器已关闭连接\n");
            } else {
                perror("recv error");
            }
            break;
        }

        // 打印服务器返回的数据
        printf("@server> %s\n", buffer);
    }

 close(sockfd);
 exit(0);
}
