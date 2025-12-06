
# TinyHttpServer

一个基于 C 语言实现的简易 HTTP 服务器，用于学习 TCP/IP 网络编程和 HTTP 协议的基础原理。

## 项目简介

TinyHttpServer 是一个轻量级的 HTTP 服务器，支持基本的静态文件服务和 HTTP/1.0 协议。该项目适合网络编程初学者学习，通过阅读源码可以理解：
- TCP 服务器的建立与连接处理
- HTTP 请求的解析与响应构建
- 多线程并发处理客户端请求
- 静态文件的读取与发送

## 原作者与源码

- **原作者**：J. David Blackstone
- **参考源码**：[https://github.com/cbsheng/tinyhttpd](https://github.com/cbsheng/tinyhttpd)


## 功能特性

- ✅ 支持 HTTP GET 方法
- ✅ 静态文件服务（HTML、CSS、图片等）
- ✅ 多线程并发处理请求
- ✅ 基本的错误处理（404、400 等）
- ✅ 支持 HTTP/1.0 协议


## 环境要求

- **编译器**：GCC (GNU Compiler Collection)
- **操作系统**：Linux/macOS（支持 POSIX 线程）
- **依赖库**：pthread（用于多线程）


## 编译与运行

### 1. 克隆仓库
```bash
git clone https://github.com/your-username/tinyHttpServer.git
cd tinyHttpServer
```

### 2. 编译项目
使用 Makefile 编译（推荐）：
```bash
make
```

或直接使用 gcc 编译：
```bash
gcc -W -Wall -lpthread -o httpd httpd.c
```

### 3. 运行服务器
```bash
./httpd
```
默认监听端口：`8080`（可在源码中修改 `PORT` 宏调整）


### 4. 测试访问
在浏览器中访问：
```
http://localhost:8080
```

或使用 curl 命令测试：
```bash
curl http://localhost:8080
```


## 使用说明

### 静态文件目录
将需要访问的静态文件（如 `index.html`、`style.css` 等）放在服务器运行的当前目录下，服务器会自动读取并返回。

### 示例请求
- 访问根目录：`http://localhost:8080/` → 返回 `index.html`（如果存在）
- 访问指定文件：`http://localhost:8080/test.html` → 返回 `test.html`
- 访问不存在的文件：返回 404 错误页面


## 代码结构

```
tinyHttpServer/
├── httpd.c          # 主程序入口，包含服务器核心逻辑
├── Makefile         # 编译脚本
└── README.md        # 项目说明文档
```

### 核心函数说明

| 函数名 | 功能 |
|--------|------|
| `main()` | 服务器初始化（socket、bind、listen），主循环处理连接 |
| `accept_request()` | 处理单个客户端请求（解析 HTTP、读取文件、发送响应） |
| `serve_file()` | 读取并发送静态文件 |
| `send_error()` | 发送 HTTP 错误响应（如 404、400） |
| `execute_cgi()` | （可选）执行 CGI 脚本（原项目支持，需额外配置） |


## 核心原理

1. **TCP 服务器建立**：
   - `socket()`：创建 TCP 套接字
   - `bind()`：绑定 IP 地址和端口
   - `listen()`：转为监听状态，设置连接队列
   - `accept()`：阻塞等待客户端连接

2. **多线程处理**：
   - 每接收到一个客户端连接，创建新线程处理请求
   - 主线程继续监听新连接，实现并发

3. **HTTP 请求处理**：
   - 解析 HTTP 请求行（如 `GET /index.html HTTP/1.0`）
   - 读取请求头部（可选）
   - 根据请求路径查找并读取本地文件
   - 构建 HTTP 响应（状态行、头部、响应体）
   - 发送响应数据给客户端


## 注意事项

- **仅用于学习**：该服务器为简易实现，缺乏生产环境所需的安全防护（如输入验证、权限控制）和性能优化（如线程池、缓存）。
- **HTTP/1.0 支持**：仅支持 HTTP/1.0 协议，不支持持久连接（Keep-Alive）和 HTTP/1.1 特性。
- **单线程 CGI**：CGI 脚本执行采用单线程，性能较低（可选功能）。


## 许可证

遵循原作者 J. David Blackstone 的开源许可证（MIT 或类似），详见原项目仓库。


## 致谢

- 感谢原作者 J. David Blackstone 提供的经典学习案例。
- 感谢所有对 TinyHttpServer 项目做出贡献的开发者。


## 学习资源

- [TCP/IP 详解 卷1：协议](https://book.douban.com/subject/1088054/)
- [HTTP 权威指南](https://book.douban.com/subject/10746113/)
- [Unix 网络编程 卷1：套接字联网 API](https://book.douban.com/subject/1500149/)


## 贡献

欢迎提交 Issue 或 Pull Request 改进项目！

---

**Happy Coding! 🚀**
