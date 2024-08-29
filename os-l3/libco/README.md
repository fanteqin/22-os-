server文件夹下面为echo服务器代码，在该文件夹下执行make server可以启动64bit的基于协程实现的echo server。改进版本以及对比实验详见下面的压力测试部分。

通过执行test_connect.py可以测试服务器的最大并发连接数量和正确性。

通过ulimit -n可以查看当前系统的最大文件描述符数量即为4096。

## 压力测试
server文件夹下normalServer.c为普通的单线程服务器，通过执行make normalServer可以启动64bit的普通服务器。

Fortio是一个开源的HTTP负载测试工具，通过执行在server文件夹下执行 ./usr/bin/fortio load -qps 0 -n 100000 tcp://localhost:12345 可以对服务器进行压力测试。

- 单线程版本的epoll的echo服务器在100000个请求下的测试结果如下：
Sockets used: 4 (for perfect no error run, would be 4)
Total Bytes sent: 2400000, received: 2400000
tcp OK : 100000 (100.0 %)
All done 100000 calls (plus 0 warmup) 0.170 ms avg, 23268.0 qps

- 普通未使用多路复用的协程版本的echo服务器在100000个请求下的测试结果如下：
Sockets used: 4 (for perfect no error run, would be 4)
Total Bytes sent: 2400000, received: 2400000
tcp OK : 100000 (100.0 %)
All done 100000 calls (plus 0 warmup) 0.260 ms avg, 15287.8 qps
**但是这里存在一个问题：占用cpu几乎为100%**

- 使用epoll多路复用的协程版本的echo服务器在100000个请求下的测试结果如下：
Error cases : no data
Sockets used: 4 (for perfect no error run, would be 4)
Total Bytes sent: 2400000, received: 2400000
tcp OK : 100000 (100.0 %)
All done 100000 calls (plus 0 warmup) 0.177 ms avg, 22172.5 qps

fortio测试中，由于echo-server的逻辑相对简单，并且几乎没有阻塞的I/O操作，因此单线程epoll实现和使用协程的epoll实现之间的性能差异可能不太明显。但在其他场景下，协程可能会表现得更有优势：

长连接、多请求: 如果服务器维护与客户端的长连接，并需要处理从每个客户端发来的大量请求，那么协程模型可以有效地在一个线程中处理这些请求，而无需为每个连接或请求创建新的线程。

阻塞的I/O或计算: 如果服务器逻辑中有阻塞的I/O操作，如数据库查询、文件I/O或其他远程服务调用，协程可以在等待这些操作完成时让出CPU，允许其他协程继续执行。这可以使服务器保持高并发，而不会因为线程阻塞而降低吞吐量。

资源限制: 在资源受限的环境中，如容器或FaaS环境，协程可以提供高并发，而无需消耗大量的内存或CPU资源。

微服务调用: 如果服务器需要与多个后端服务交互，特别是在微服务架构中，协程可以并发地发起多个远程调用，并在等待响应时让出CPU。

复杂的业务逻辑: 对于需要维护复杂状态机的业务逻辑，使用协程可以使代码更易读、更直观，因为可以避免使用回调或事件驱动的方式编写代码。

