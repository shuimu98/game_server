# game_server


采用lua5.3.4,底层全部用C来实现，参考了一些skynet的东西，但是还是多进程单线程的架构。


网络库参考redis网络框架，并精简了一些东西，只用了epoll，另外，timer部分改成了时间轮。

代码风格也参考了redis，另外，lua与c交互的部分，参考了skynet。

有空就会继续撸下去，暂时没有太多空闲时间啦~~


**TODO：**

- lua5.4 等正式版出来，直接撸到5.4
- postgre SQL，想玩一把这个数据库
- unix域套接字，用来支持同一台物理机上的内部进程连接
- 想接入一些c版本的stl容器，方便开发
