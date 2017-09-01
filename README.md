[ZMQ-client4cpp](http://gitlab.ztesoft.com/chen.si1/mq-client4cpp) C++ Client
===================

### 主要贡献者
* @[kangliqiang](https://github.com/kangliqiang)

### 目前现状
* 支持发送消息，支持pull/push模式消费消息

### 编译说明 
+ **安装C++编译器**
    `sudo yum install gcc gcc-c++`

+ **首先编译依赖的libs库**
   对于centos7或redhat7系统，直接编译即可；
   对于redhat6.X系统，分别使用libs目录下的lib-boost-share6.X，lib-boost-static6.X下的动态库
   替换掉lib-boost-share，lib-boost-static下的动态库，再编译
     `make deps`
 
+ **编译客户端**
     `make`
 
### 编译成功, `librocketmq64.so`和`libaliyunmq64.so`生成于bin目录中
### 应用运行时，需要确保LD_LIBRARY_PATH环境变量中，配置了bin目录，这样才能加载该动态库
    export LD_LIBRARY_PATH=XXX/bin

+ **测试**
    `example/simple/build.sh`