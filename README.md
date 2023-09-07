# zedis

Redis的Modern C++实现，内容包括但不限于，简单的Socket编程，大量的侵入式数据结构实现，其他零散的Redis功能比如TTL。

>目前看来项目还是有点问题  
>一方面是项目文件结构，使用的[C++项目模板](https://github.com/Codesire-Deng/TemplateRepoCxx)不能处理自定义的库是声明和定义多文件的形式（而且我现阶段也没能力写出更好的）  
>另一方面是代码整体风格不统一，发心是Modern C++，但是侵入式的数据结构不太容易用智能指针管理，后续考虑用CRTP进行改良。
>>还是应该多看优秀的开源代码呀。

+ 项目需求，在学完《C++ Primer》之后，发现对很多C++11之后的特性和实践理解不够深，考虑在实践中进行更深的理解，而Web Server写的人太多了，接触到[《Build Your Own Redis with C/C++》](https://build-your-own.org/redis/)，调研后发现它的核心在于数据结构，有部分网络编程，同时也涉及Unix对多线程的API，虽然叫C/C++，其实更应该叫做C with vector and string，再考虑到Redis是很有名的开源软件，于是想根据这个资料将代码改写成Modern C++的mini Redis项目。

+ 网络部分：
  + 字节流封装: `./include/bytes.h`，TCP是面向字节的，而S/C两端需要对字节进行解析。这里使用一个单独的类维护这里的字节流，要保证放入和取出的前后顺序，并提供相应的接口   
    使用`std::vector<std::byte>`存储内容，取额外的一个变量表示对字节流读的位置，在读和写时进行位置检测，另外就是支持多种类型的读和写  
    >但是现在看接口的设计依然有问题，不同的类型使用不同的方法，感觉不是很优雅  
    >另外整体是动态数组，只是使用额外的变量指示末尾位置，也就是说该字节流不能长时间运行，不然会有很多冗余的空间。

  + 文件描述符封装: `./include/file.h`，C/C++的网络编程主要是关于网络和IO的系统调用，这样太C风格了，我希望对这部分进行封装，IO部分已经通过`byte.h`实现了，那网络部分呢？这里主要实现了对Linux下文件描述符的封装，即通过RAII自动的管理文件描述符的关闭。而网络编程中的Socket，在Linux也是文件描述符，这里直接在构造的时候将套接字放进文件描述符的封装中。
  
  关于上面两个部分，更具体的，所有的网络的通信，都直接往字节流里塞，然后塞完后将字节流交给文件描述符（里面放着套接字），进行发送。相应的接收也是把一个空的字节流交给文件描述符，它会从套接字中读出字节并放入到字节流中。

  还有一个问题，上面是套接字的阻塞模式，那么非阻塞模式呢？在下面讨论

  + 通信协议：
    + 服务端向客户端的请求：最开始四个字节作为消息头表示后面消息体的字节个数，消息体中，最开始四个字节表示一共有多少个命令（其实这个比较冗余），后面每个命令中，开始四个字节表示后面命令主体的字节个数。
    + 客户端向服务端的响应：最开始四个字节作为消息头表示后面消息体的字节个数，消息体中，使用TLV(type-length-value)格式  
      这里的`type`只有一个字节
      + `nil`
      + `err`：后面首先四个四节表示错误类型，然后四个字节表示错误消息长度
      + `str`：四个字节表示字符串长度
      + `int`：八个字节表示整数
      + `arr`：开始四个字节表示数组元素个数，数组元素中每个元素的类型同上。

  + 客户端：`./include/client.h`：因为有字节流和文件描述符了，这里就是简单的按照通信协议进行手法即可。
  + TCP连接的封装：`./include/connect.h`：虽然我们没讲服务端，但是这里已经进入到服务端这边了，为了提高并发量，这里使用的是IO多路复用`poll`机制，这里将每个连接也封装到一个类中。由类内方法完成一整个`读取 -> 处理 -> 写会`过程。  
  这个关于系统调用的封装在`file.h`中，结合系统调用的返回值和`errno`信号判断情况，然后将不同的情况使用不同的整数返回。交由connect处理。
  
  + 服务端：`./include/servef`：核心在一个死循环中，每次循环，都会尝试建立连接，然后将连接缓存；每次循环也会利用`poll`的系统调用检测那些连接可读或者结束。结束则清除对应缓存，以此往复。



## Build

```bash
mkdir build -p && cd build && cmake .. && make
```

## Use

+ 服务端: `./build/src/server`
+ 客户端: `./build/src/client`前缀, 后接命令, 支持命令如下:
  ```
  set 键 值
  get 键
  del 键
  # 有序集
  zadd   有序集名 键 值
  zrem   有序集名 键 值  # 删除
  zscore 有序集名 键  # 查询
  zquery 有序集名 起始键 起始值 偏移量
  # TTL
  pexpire 键 TTL  # 设置存活时间, 键应该提前set, 时间单位为毫秒
  pttl    键      # 查看存活时间
  ```

## Develop

### 通信协议

客户端向服务端
+ 开始四个字节表示后面消息的字节数
+ 开始四个字节表示命令个数
+ 每个命令四个字节表示命令字节数

服务端向客户端
+ 开始四个字节表示后面消息的字节数
+ 消息使用TLV(type-length-value)格式

### 内存泄露

+ 检测使用
  ```bash
  valgrind -s --track-origins=yes --leak-check=full --tool=memcheck 可执行文件
  ```
