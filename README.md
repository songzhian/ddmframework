# mframework

它是一款SOA的轻量级分布式服务框架,其核心部分包含
* **远程通讯**: restful或者其它api一般用来提供对外的服务，对内的服务更适合采用效率更高的TCP套接字协议。
* **自动发现**: 内部服务，基于注册中心目录服务，使服务消费方能动态的查找服务提供方，使地址透明，使服务提供方负载均衡可以平滑增加或减少机器。
* **语言依赖**  : 语言依赖不强,多语言,支持包括C,Java,PHP,GO,Nodejs等广泛的多语言环境。

## DEMO
目前只做了php(因为作者对php比较熟悉)的 DEMO  
  provider使用php swoole实现tcp长连接
  consumer实现了php的protobuf扩展
* 定义一个远程服务接口
* provider发布远程服务到注册中心
* consumer自动发现远程服务并完成服务调用

想要了解更多, 请使用wike(http://a.io).

## 文档

* [User's Guide](http://a.io)
* [Developer's Guide](http://a.io)
* [Admin's Guide](http://a.io)

## 快速启动
#### 下载代码
下载本地副本代码, 通过这种工作快速入门。下载我们的演示代码[Github repository](https://github.com/songzhian/ddmframework)


```sh
$ cd ~
$ # Clone the repository to get the source code.
$ git clone https://github.com/songzhian/ddmframework.git mframework
$ git checkout master
$ # or: git checkout -b mframework-x
```
#### 编译 & 运行
1. 编译文件

```sh
$ cd ~/mframework
$ # The demo code for this quickstart all stay in the `dubbo-demo` folder
$ cd ./soa_agent
$ make
$ ls
```
2. 运行 consumer-agent. 开启一个代理服务用于发现服务建立连接
```sh
$ cd ~/mframework/soa_agent
$ #  run
$ agent.sh
```

3. 运行 demo-provider. Start the consumer and consume service provided by _the provider_ above

```sh
$ # Navigate to the consumer part
$ cd ~/mframework/demo-demo/
$ php SoaServer.php
```


## 参考文献

http://en.wikipedia.org/wiki/Service-oriented_architecture
http://www.infoq.com/cn/articles/micro-soa-1
http://www.infoq.com/cn/articles/micro-soa-2
http://zookeeper.apache.org/
http://msgpack.org/




