/* J. David's webserver */
/* This is a simple webserver.
 * Created November 1999 by J. David Blackstone.
 * CSE 4344 (Network concepts), Prof. Zeigler
 * University of Texas at Arlington
 */
/* This program compiles for Sparc Solaris 2.6.
 * To compile for Linux:
 *  1) Comment out the #include <pthread.h> line.
 *  2) Comment out the line that defines the variable newthread.
 *  3) Comment out the two lines that run pthread_create().
 *  4) Uncomment the line that runs accept_request().
 *  5) Remove -lsocket from the Makefile.
 */
 
 /*
     代码中除了用到 C 语言标准库的一些函数，也用到了一些与环境有关的函数(例如POSIX标准)
     具体可以参读《The Linux Programming Interface》，以下简称《TLPI》，页码指示均为英文版
     
     注释者：cbsheng 20%, Edward 80%
 */
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
//#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>

#define ISspace(x) isspace((int)(x))  // 判断是不是空字符（空格、换行、缩进...）的宏定义

#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"  // 请求行中服务器信息的宏定义

void accept_request(int);
void bad_request(int);
void cat(int, FILE *);
void cannot_execute(int);
void error_die(const char *);
void execute_cgi(int, const char *, const char *, const char *);
int get_line(int, char *, int);
void headers(int, const char *);
void not_found(int);
void serve_file(int, const char *);
int startup(u_short *);
void unimplemented(int);

/**********************************************************************/
/* A request has caused a call to accept() on the server port to
 * return.  Process the request appropriately.
 * Parameters: the socket connected to the client */
/**********************************************************************/
void accept_request(int client)  // 参数1：客户端通信的专用套接字
{
 char buf[1024];  // 创建一个缓冲区用于服务器客户端之间的数据传输
 int numchars;  // 记录get_line函数一次读取到的字符数量
 char method[255];  // 记录请求行中的method
 char url[255];  // 记录请求行中的URL
 char path[512];  // 记录可能要访问的资源地址
 size_t i, j;
 struct stat st;  // 记录文件状态信息的结构体

// cgi是一种标准化的接口规范，核心作用是让 Web 服务器能够调用外部程序，并将程序的输出返回给客户端。
 int cgi = 0;      /* becomes true if server decides this is a CGI
                    * program */
 char *query_string = NULL;

 //读http 请求的第一行数据（request line），把请求方法存进 method 中
 numchars = get_line(client, buf, sizeof(buf));  // 自定义函数，用来读取一行来自客户端的请求
 i = 0; j = 0;
 while (!ISspace(buf[j]) && (i < sizeof(method) - 1))  // ISspace是宏定义函数
 {  // 读取完整的method直到遇到空白符
// 请求行的开头就是这个请求使用的method
  method[i] = buf[j];
  i++; j++;
 }
 method[i] = '\0';

/*
一个完整的HTTP POST请求标准格式
POST /api/user/login HTTP/1.1
Host: www.example.com
Content-Type: application/json
Content-Length: 45
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/120.0.0.0
Accept: application/json

{"username":"test","password":"123456","remember":true}
第一行是请求行
之后是请求体
之后是一个空白行
空白行之后是请求体（GET方法没有请求体，二者具体差异后面处理请求时会体现出）
*/

 // 如果请求的方法不是 GET 或 POST 任意一个的话就直接发送 response 告诉客户端没实现该方法
 // 本程序只实现了GET, POST两种请求方法
 if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
 {
  unimplemented(client);  // 自定义函数，服务器报501错误
  return;
 }

 // 如果是 POST 方法就将 cgi 标志变量置一(true)
 if (strcasecmp(method, "POST") == 0)
  cgi = 1;

 i = 0;
 // 跳过所有的空白字符(空格)
 while (ISspace(buf[j]) && (j < sizeof(buf))) 
  j++;
 
 // 然后把 URL读出来放到 url 数组中
 // 标准格式中api/user/login部分就是URL
 while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))
 {
  url[i] = buf[j];
  i++; j++;
 }
 url[i] = '\0';

 // 如果这个请求是一个 GET 方法的话
 if (strcasecmp(method, "GET") == 0)
 {
  // 用一个指针指向 url
  query_string = url;
  
  // 去遍历这个 url，跳过字符 ？前面的所有字符，如果遍历完毕也没找到字符 ？则退出循环
  // GET方法的URL中可能携带参数，路径和参数中间用?隔开
  while ((*query_string != '?') && (*query_string != '\0'))
   query_string++;
  
  // 退出循环后检查当前的字符是 ？还是字符串(url)的结尾
  if (*query_string == '?')
  {
   // 如果是?的话，证明这个GET请求需要调用 cgi，将 cgi 标志变量置一(true)
   cgi = 1;
   //从字符?处把字符串 url 给分隔会两份
   *query_string = '\0';
   // 使指针指向字符 ？后面的那个字符（即参数部分）
   query_string++;
  }
 }

 // 将前面分隔两份的前面那份字符串（路径部分），拼接在字符串htdocs的后面之后就输出存储到数组 path 中。
 sprintf(path, "htdocs%s", url);
 // 如果 path 数组中的这个字符串的最后一个字符是以字符 / 结尾的话，就拼接上一个"index.html"的字符串。首页的意思
 if (path[strlen(path) - 1] == '/')
  strcat(path, "index.html");
 // 在系统上去查询该文件是否存在
 // stat() 是一个用于获取文件/目录元数据的系统调用，核心作用是通过文件路径，获取文件的状态信息（如文件类型、大小、权限、修改时间等），并将这些信息存储到一个 struct stat 结构体中。
 if (stat(path, &st) == -1) {  // 成功返回0，失败返回-1
  //如果不存在，那把这次 http 的请求后续的内容(head 和 body)全部读完并忽略，这么做是为了清空缓冲区
  while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers  遇到空白行停止*/  
   numchars = get_line(client, buf, sizeof(buf));
  //然后返回一个找不到文件的 response 给客户端
  not_found(client);  // 自定义函数，服务器报404错误
 }
 else
 {
  // 文件存在，那去跟常量S_IFMT按位与，得到的值可以用来判断该文件是什么类型的
  // S_IFMT参读《TLPI》P281，与下面的三个常量一样是包含在<sys/stat.h>
  // 在 Linux 系统中，S_IFMT 的值通常是 0170000（八进制），对应的二进制高 4 位为 1，刚好覆盖 st.st_mode 中文件类型的位区域。
  // st.st_mode用来记录一个文件的状态，其二进制位被划分为文件类型位（高4位）、特殊权限位（中3位）和普通权限位（低9位）三部分
  if ((st.st_mode & S_IFMT) == S_IFDIR)  
   // 在 Linux 中，S_IFDIR 的值通常是 0040000（八进制），表示当 st.st_mode 的文件类型位为这个值时，文件是目录。
   // 如果这个文件是个目录，那就需要再在 path 后面拼接一个"/index.html"的字符串
   strcat(path, "/index.html");
   // printf("服务器路径：%s\n",path);
   // S_IXUSR, S_IXGRP, S_IXOTH三者可以参读《TLPI》P295
   // S_IXUSR一般定义为0100，S_IXGRP为0010，S_IXOTH为0001，三者分别与st.st_mode进行按位与可以用来判断当前文件是否拥有文件所有者、文件所属组、其他用户的执行权限（要有权限才能执行这个文件）
  if ((st.st_mode & S_IXUSR) ||  
      (st.st_mode & S_IXGRP) ||
      (st.st_mode & S_IXOTH))
   // 如果这个文件是一个可执行文件，不论是属于用户/组/其他这三者类型的，就将 cgi 标志变量置一
   cgi = 1;
   
  if (!cgi)
   //如果不需要 cgi 机制的话，直接上传静态内容，逻辑会简单很多
   serve_file(client, path);  // 上传静态内容的自定义函数
  else
   //如果需要则调用
   execute_cgi(client, path, method, query_string);  // 自定义函数，用来运行CGI外部程序
 }

 close(client);  // 关闭客户端连接
}

/**********************************************************************/
/* Inform the client that a request it has made has a problem.
 * Parameters: client socket */
/**********************************************************************/
void bad_request(int client)
{
 char buf[1024];

 sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
 send(client, buf, sizeof(buf), 0);
 sprintf(buf, "Content-type: text/html\r\n");
 send(client, buf, sizeof(buf), 0);
 sprintf(buf, "\r\n");
 send(client, buf, sizeof(buf), 0);
 sprintf(buf, "<P>Your browser sent a bad request, ");
 send(client, buf, sizeof(buf), 0);
 sprintf(buf, "such as a POST without a Content-Length.\r\n");
 send(client, buf, sizeof(buf), 0);
}

/**********************************************************************/
/* Put the entire contents of a file out on a socket.  This function
 * is named after the UNIX "cat" command, because it might have been
 * easier just to do something like pipe, fork, and exec("cat").
 * Parameters: the client socket descriptor
 *             FILE pointer for the file to cat */
/**********************************************************************/
void cat(int client, FILE *resource)  // 参数1：客户端套接字 参数2：资源文件指针
{
 char buf[1024];

 //将文件中全部内容读取并逐行发送到客户端
 fgets(buf, sizeof(buf), resource);
 while (!feof(resource))
 {
  send(client, buf, strlen(buf), 0);
  fgets(buf, sizeof(buf), resource);
 }
}

/**********************************************************************/
/* Inform the client that a CGI script could not be executed.
 * Parameter: the client socket descriptor. */
/**********************************************************************/
void cannot_execute(int client)
{
 char buf[1024];

 sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "Content-type: text/html\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
 send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Print out an error message with perror() (for system errors; based
 * on value of errno, which indicates system call errors) and exit the
 * program indicating an error. */
/**********************************************************************/
void error_die(const char *sc)
{
 // 包含于<stdio.h>,基于当前的 errno 值，在标准错误上产生一条错误消息。参考《TLPI》P49
 perror(sc); 
 exit(1);
}

/**********************************************************************/
/* Execute a CGI script.  Will need to set environment variables as
 * appropriate.
 * Parameters: client socket descriptor
 *             path to the CGI script */
/**********************************************************************/
void execute_cgi(int client, const char *path,
                 const char *method, const char *query_string)  // 参数1：客户端套接字 参数2：CGI脚本地址 参数3：请求方法 参数4：如果是GET方法，用来传递URL中的参数部分
{
 char buf[1024];
 int cgi_output[2];  // 存放管道的两个文件描述符（每个管道有读端和写端，对于下标0和1）
 int cgi_input[2];
 pid_t pid;
 int status;  // 记录子进程的退出状态
 int i;
 char c;
 int numchars = 1;
 int content_length = -1;
 
 // 往 buf 中填东西以保证能进入下面的 while
 buf[0] = 'A'; buf[1] = '\0';
 // 如果请求是 GET 方法的话读取并忽略请求剩下的内容（参数已通过query_string传递）
 if (strcasecmp(method, "GET") == 0)
  while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
   numchars = get_line(client, buf, sizeof(buf));
 else    /* POST */
 {
  // 只有 POST 方法才继续读内容（POST方法的参数通过请求头传递）
  numchars = get_line(client, buf, sizeof(buf));
  // 这个循环的目的是读出指示 body 长度大小的参数，并记录 body 的长度大小。其余的 header 里面的参数一律忽略
  // 注意这里只读完 header 的内容，body 的内容没有读
  while ((numchars > 0) && strcmp("\n", buf))
  {
   buf[15] = '\0';  // 这里把每行的前15个字符单独拎出来，判断是不是Content-Length:
   if (strcasecmp(buf, "Content-Length:") == 0)
    content_length = atoi(&(buf[16]));  // 如果是的话，记录 body 的长度大小，atoi函数用来将ASCII字符串转为整数
   numchars = get_line(client, buf, sizeof(buf));
  }
  
  // 如果 http 请求的 header 没有指示 body 长度大小的参数，则报错返回
  if (content_length == -1) {
   bad_request(client);  // 自定义函数，服务器报400错误（来自客户端的HTTP请求格式错误）
   return;
  }
 }

 sprintf(buf, "HTTP/1.0 200 OK\r\n");  // 向客户端发送状态行
 send(client, buf, strlen(buf), 0);

 // 下面这里创建两个管道，用于两个进程间通信
 if (pipe(cgi_output) < 0) {  // 成功返回0，失败返回-1
  cannot_execute(client);  // 自定义函数，服务器报500错误（服务器内部运行出错）
  return;
 }
 if (pipe(cgi_input) < 0) {
  cannot_execute(client);
  return;
 }

 // 创建一个子进程
 if ( (pid = fork()) < 0 ) {  // fork创建失败会返回-1，成功会在父进程中返回子进程的PID，子进程中返回0
  cannot_execute(client);
  return;
 }
 
 // 注意从现在开始父子进程开始同时运行，区别他们的方法是PID的值

 // 子进程用来执行 cgi 脚本
 if (pid == 0)  /* child: CGI script */
 {
  char meth_env[255];
  char query_env[255];
  char length_env[255];

  // dup2()包含<unistd.h>中，参读《TLPI》P97
  // 将子进程的输出由标准输出重定向到 cgi_ouput 的管道写端上
  dup2(cgi_output[1], 1);  // dup2函数的作用是重定向标准输入、标准输出和标准错误
  // 将子进程的输入由标准输入重定向到 cgi_ouput 的管道读端上
  dup2(cgi_input[0], 0);  // 参数2的0，1，2代表标准输入、标准输出和标准错误
  // 关闭 cgi_ouput 管道的读端与cgi_input 管道的写端
  close(cgi_output[0]);
  close(cgi_input[1]);
  
  // 构造一个环境变量
  sprintf(meth_env, "REQUEST_METHOD=%s", method);
  // putenv()包含于<stdlib.h>中，参读《TLPI》P128
  // 将这个环境变量加进子进程的运行环境中
  putenv(meth_env);  // 成功返回0，失败返回-1
  
  // 根据http 请求的不同方法，构造并存储不同的环境变量
  if (strcasecmp(method, "GET") == 0) {
   sprintf(query_env, "QUERY_STRING=%s", query_string); // query_string就是url中?后面的部分
   putenv(query_env);
  }
  else {   /* POST */
   sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
   putenv(length_env);
  }
  
  //execl()包含于<unistd.h>中，参读《TLPI》P567
  //最后将子进程替换成另一个进程并执行 cgi 脚本
  execl(path, path, NULL);  // 参数1和2一般是要执行的脚本的地址，等参数传完之后一定是NULL结尾
  exit(0);
  
 } else {    /* parent */
  //父进程则关闭了 cgi_output管道的写端和 cgi_input 管道的读端
  close(cgi_output[1]);
  close(cgi_input[0]);
  
  // 如果是 POST 方法的话就继续读 body 的内容，并写到 cgi_input 管道里让子进程去读
  if (strcasecmp(method, "POST") == 0)
   for (i = 0; i < content_length; i++) {
    recv(client, &c, 1, 0);  // recv函数用于从已连接的套接字中读取数据，参数3是缓冲区大小，参数4是标志位，为0表示阻塞接收
    write(cgi_input[1], &c, 1);  // 通过管道逐字符地将请求体传递给子进程
   }
   
  // 然后从 cgi_output 管道中读子进程的输出，并发送到客户端去
  while (read(cgi_output[0], &c, 1) > 0)  // 当子进程运行结束，所有写端都被关闭时，read函数会返回0
   send(client, &c, 1, 0);

  // 关闭管道
  close(cgi_output[0]);
  close(cgi_input[1]);
  // 为什么子进程没有显式关闭其读写文件描述符，即cgi_output[1]和cgi_input[0]？因为这两个文件描述符被重定向到了标准输入输出，excel之后的CGI脚本还要用，并且用完之后会被操作系统自动关闭
  // 等待子进程的退出，并且回收子进程的资源
  waitpid(pid, &status, 0);  // 参数三为0表示阻塞等待子进程运行结束，父进程才会继续往下执行
 }
}

/**********************************************************************/
/* Get a line from a socket, whether the line ends in a newline,
 * carriage return, or a CRLF combination.  Terminates the string read
 * with a null character.  If no newline indicator is found before the
 * end of the buffer, the string is terminated with a null.  If any of
 * the above three line terminators is read, the last character of the
 * string will be a linefeed and the string will be terminated with a
 * null character.
 * Parameters: the socket descriptor
 *             the buffer to save the data in
 *             the size of the buffer
 * Returns: the number of bytes stored (excluding null) */
/**********************************************************************/
int get_line(int sock, char *buf, int size)
{
 int i = 0;
 char c = '\0';
 int n;

 while ((i < size - 1) && (c != '\n'))
 {
  //recv()包含于<sys/socket.h>,参读《TLPI》P1259, 
  //读一个字节的数据存放在 c 中
  n = recv(sock, &c, 1, 0);
  /* DEBUG printf("%02X\n", c); */
  if (n > 0)
  {
   if (c == '\r')
   {
    //
    n = recv(sock, &c, 1, MSG_PEEK);
    /* DEBUG printf("%02X\n", c); */
    if ((n > 0) && (c == '\n'))
     recv(sock, &c, 1, 0); // 统一行结束符为\n
    else
     c = '\n';
   }
   buf[i] = c;
   i++;
  }
  else
   c = '\n';
 }
 buf[i] = '\0';

 return(i);
}

/**********************************************************************/
/* Return the informational HTTP headers about a file. */
/* Parameters: the socket to print the headers on
 *             the name of the file */
/**********************************************************************/
void headers(int client, const char *filename)
{
 char buf[1024];
 (void)filename;  /* could use filename to determine file type */
/*
服务器response的标准格式
HTTP/1.1 200 OK\r\n          // 状态行
Server: Apache/2.4.41 (Ubuntu)\r\n  // 响应头1
Content-Type: text/html\r\n  // 响应头2
Content-Length: 1234\r\n     // 响应头3（可选）
\r\n                         // 空行（分隔头和体）
<html><body>Hello World</body></html>  // 响应体
*/

 strcpy(buf, "HTTP/1.0 200 OK\r\n");
 send(client, buf, strlen(buf), 0);
 strcpy(buf, SERVER_STRING);
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "Content-Type: text/html\r\n");
 send(client, buf, strlen(buf), 0);
 strcpy(buf, "\r\n");
 send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Give a client a 404 not found status message. */
/**********************************************************************/
void not_found(int client)
{
 char buf[1024];

 sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, SERVER_STRING);
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "Content-Type: text/html\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "your request because the resource specified\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "is unavailable or nonexistent.\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "</BODY></HTML>\r\n");
 send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Send a regular file to the client.  Use headers, and report
 * errors to client if they occur.
 * Parameters: a pointer to a file structure produced from the socket
 *              file descriptor
 *             the name of the file to serve */
/**********************************************************************/
void serve_file(int client, const char *filename)  // 参数1：客户端套接字 参数2：要访问的资源地址
{
 FILE *resource = NULL;
 int numchars = 1;
 char buf[1024];

 // 确保 buf 里面有东西，能进入下面的 while 循环
 buf[0] = 'A'; buf[1] = '\0';
 //循环作用是读取并忽略掉这个 http 请求后面的所有内容，以清空套接字接收缓冲区
 while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
  numchars = get_line(client, buf, sizeof(buf));

 // 打开这个传进来的这个路径所指的文件
 resource = fopen(filename, "r");
 if (resource == NULL)
  not_found(client);  // 服务器报404错误
 else
 {
  // 打开成功后，将这个文件的基本信息封装成 response 的头部(header)并发送给客户端
  headers(client, filename);  // 自定义函数
  // 接着把这个文件的内容读出来作为 response 的 body 发送到客户端
  cat(client, resource);  // 自定义函数
 }
 
 fclose(resource);  // 关闭文件
}

/**********************************************************************/
/* This function starts the process of listening for web connections
 * on a specified port.  If the port is 0, then dynamically allocate a
 * port and modify the original port variable to reflect the actual
 * port.
 * Parameters: pointer to variable containing the port to connect on
 * Returns: the socket */
/**********************************************************************/
int startup(u_short *port)  // 传入参数：1.指定的端口号（如果为0，则随机分配端口）
{
 int httpd = 0;  // 声明一个变量（用于存放总线套接字文件描述符，用int存储）
 // sockaddr_in 是 IPV4的套接字地址结构。定义在<netinet/in.h>,参读《TLPI》P1202
 struct sockaddr_in name;
 /*
 IPV4套接字地址结构体具体定义（Linux系统）
struct sockaddr_in {
    sa_family_t    sin_family;   // 地址族，IPV4或IPV6
    in_port_t      sin_port;     // 端口号（网络字节序）
    struct in_addr sin_addr;     // IPv4 地址（网络字节序）
    char           sin_zero[8];  // 填充字段，使结构体大小与 sockaddr 一致
};
*/
 // socket()用于创建一个用于 socket 的描述符，函数包含于<sys/socket.h>。参读《TLPI》P1153
 // 这里的PF_INET其实是与 AF_INET同义，具体可以参读《TLPI》P946
 httpd = socket(PF_INET, SOCK_STREAM, 0);  // 向操作系统申请一个套接字资源并存放在httpd变量中
/*
socket()函数用于申请套接字资源
参数1：指定IPV4或IPV6 （PF_INET或AF_INET都代表IPV4）
参数2：指定通信协议（SOCK_STREAM代表TCP/IP协议）
参数3：指定协议种类（TCP/IP协议只有一种，所以填0，有些协议有多种）
*/
 if (httpd == -1)  // 如果申请失败，函数会返回-1（成功会返回一个套接字文件描述符）
  error_die("socket");  // 自定义的函数，根据errno打印错误信息并结束程序
  
 memset(&name, 0, sizeof(name));  // 清空垃圾值
 name.sin_family = AF_INET;  // 指定协议族为IPV4
 // htons()，ntohs() 和 htonl()包含于<arpa/inet.h>, 参读《TLPI》P1199
 // 将*port 转换成以网络字节序表示的16位整数
 // 一个端口号（比如52000），平时以主机字节序表示，转为网络字节序就是其16个比特位的二进制
 name.sin_port = htons(*port);  // htons = host to network short，用来转换端口号
 // INADDR_ANY是一个 IPV4通配地址的常量，包含于<netinet/in.h>
 // 大多实现都将其定义成了0.0.0.0 参读《TLPI》P1187
 // 使用INADDR_ANY后，该套接字就可以监听本地所有IP地址上发来的连接请求
 name.sin_addr.s_addr = htonl(INADDR_ANY);  // htonl = host to network long，用来转换地址
 
 // bind()用于绑定地址与 socket。参读《TLPI》P1153
/*
bind函数参数解析：
参数1：一个套接字文件描述符
参数2：希望绑定的IP地址
参数3：地址结构体的长度
*/
 // 如果传进去的sockaddr结构中的 sin_port 指定为0，这时系统会选择一个临时的端口号
 if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0)  // bind绑定失败会返回-1，成功返回0
  error_die("bind");
  
 // 如果是动态分配端口，需要通过getsockname()函数获取到这个临时端口号并修改port
 if (*port == 0)  /* if dynamically allocating a port */
 {
  int namelen = sizeof(name);
  // getsockname()包含于<sys/socker.h>中，参读《TLPI》P1263
  // 调用getsockname()获取系统给 httpd 这个 socket 随机分配的端口号
  if (getsockname(httpd, (struct sockaddr *)&name, &namelen) == -1)
   error_die("getsockname");
  *port = ntohs(name.sin_port);
 }
 
 // 最初的 BSD socket 实现中，backlog 的上限是5.参读《TLPI》P1156
// listen函数的作用是将已绑定（bind）的套接字转换为监听套接字，等待客户端的连接请求。
// 监听队列的最大长度（backlog）：表示内核为该套接字维护的未完成连接队列
 if (listen(httpd, 5) < 0)   // listen转换失败会返回-1，成功返回0
  error_die("listen");
 return(httpd);
}

/**********************************************************************/
/* Inform the client that the requested web method has not been
 * implemented.
 * Parameter: the client socket */
/**********************************************************************/
void unimplemented(int client)
{
 char buf[1024];

 sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, SERVER_STRING);
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "Content-Type: text/html\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "</TITLE></HEAD>\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "</BODY></HTML>\r\n");
 send(client, buf, strlen(buf), 0);
}

/**********************************************************************/

int main(void)
{
 int server_sock = -1;  // 服务器总线套接字声明（该套接字负责监听所有发往服务器的连接申请）
 u_short port = 52000;  // 固定一个端口方便测试
 int client_sock = -1;  // 客户端专用套接字声明（该套接字负责和某一具体的客户端建立通信）
 // sockaddr_in 是 IPV4的套接字地址结构。定义在<netinet/in.h>,参读《TLPI》P1202
 // sockaddr_in专门用于存储IPv4 协议的套接字地址信息，是对通用套接字地址结构体 struct sockaddr 的具体化
 struct sockaddr_in client_name;  // 客户端套接字地址信息结构体声明
 int client_name_len = sizeof(client_name);
 //pthread_t newthread;  // 该语句声明了一个线程变量，但本项目使用进程代替线程

 server_sock = startup(&port);  // 自定义函数，用于创建服务器的监听通道并返回其套接字
 printf("httpd running on port %d\n", port);

 while (1)
 {
  // 阻塞等待客户端的连接，参读《TLPI》P1157
  client_sock = accept(server_sock,
                       (struct sockaddr *)&client_name,
                       &client_name_len);

/*
参数解读：
参数1：服务器总线套接字
参数2：客户端的套接字地址信息
参数3：客户端的套接字地址信息长度
参数2和3需要填入对应变量的指针，这样accept函数可以将客户端的套接字相关信息放入里面方便后面使用
*/

  if (client_sock == -1)  // 接收连接失败返回-1，成功返回一个新的套接字，专门用于和这个客户端进行通信
   error_die("accept");
  accept_request(client_sock);  // 自定义函数，成功连接到客户端之后，开始读取并处理客户端的请求

 /*if (pthread_create(&newthread , NULL, accept_request, client_sock) != 0)
   perror("pthread_create");*/
 }

 close(server_sock);  // 关闭服务器

 return(0);
}
