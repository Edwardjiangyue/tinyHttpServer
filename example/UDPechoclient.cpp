/*linux socket AF_INET  UDP 编程示例，客户端，单进程单线程。*/
#include <cstdio>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#define BUFFER_SIZE 1024  // 定义缓冲区大小

int main()
{
	int cli_sock = socket(AF_INET, SOCK_DGRAM, 0);
	char buf[1024];
	//conn_addr 是要连接的服务器地址结构
	struct sockaddr_in conn_addr;

	conn_addr.sin_family = AF_INET;
	conn_addr.sin_port = htons(8345);
	//serv_addr 是用来存储 recvfrom 中的地址结构
	struct sockaddr_in serv_addr;
	socklen_t serv_addr_len = sizeof(serv_addr);
	char serv_ip[INET_ADDRSTRLEN];
	strcpy(serv_ip, "127.0.0.1");

	//conn_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	//将 inet_addr() 改用 inet_pton() 这个现代的方法，支持 IPV4 和 IPV6

	if (inet_pton(AF_INET, serv_ip, &conn_addr.sin_addr) !=1) {
		printf("inet_pton error\n");
		close(cli_sock);
		return 0;
	}
	
	printf("已连接到服务器%s，端口号：%d，输入消息发送（输入 exit 退出）：\n", inet_ntop(AF_INET, &conn_addr.sin_addr, serv_ip, sizeof(serv_ip)), ntohs(conn_addr.sin_port));
	while (1) {
		printf("@this_client> ");
		if(!fgets(buf, BUFFER_SIZE, stdin)){
			perror("fgets error");
			break;
		}
		buf[strcspn(buf, "\n")] = '\0';

		if (strcmp(buf, "exit") == 0) {
            		printf("退出连接...\n");
			buf[0] = '\0';  // 结束时按照约定给服务器发一个空数据报
			if(sendto(cli_sock, buf, strlen(buf) + 1, 0,
				(struct sockaddr*)&conn_addr,
				sizeof(conn_addr) ) < 1){
				perror("sendto error");
				break;
			}
            		break;
        	}

		if(sendto(cli_sock, buf, strlen(buf) + 1, 0,
			(struct sockaddr*)&conn_addr,
			sizeof(conn_addr) ) < 1){
			perror("sendto error");
			break;
		}
		int n = recvfrom(cli_sock, buf, sizeof(buf), 0,
						 (struct sockaddr*)&serv_addr,
						 &serv_addr_len );
						 
		if (n > 0) {		
			char serv_ip[INET_ADDRSTRLEN];
			if (inet_ntop(AF_INET, &serv_addr.sin_addr, serv_ip, sizeof(serv_ip)) == NULL) {
				printf("inet_ntop error\n");
				close(cli_sock);
				return 0;
			}
			printf("@server>%s\n", buf);
		}
	}

	close(cli_sock);
	return 0;
}