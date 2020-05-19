# 基于C语言Socket的聊天室

### 1	表情解析



`"\smile","\cry","\happy","\sad","\like","\dizzy","\speechless","\dull"`
对应:

`":-)","qwq","^v^",":-(","*v*","@_@","-_-#","o_o"`

上述表情符号可出现在消息中的任意位置



### 2	文件传输

`\sendfile` + `文件名` , 客户端发送文件到聊天室

`\recvfile` + `文件名`， 客户端从聊天室接收文件

上述文件传输符可出现在消息中的任意位置



### 3	私聊

`\prvtmsg NAME1,NAME2,... MSG`

发送消息到NAME1,NAME2,...

当然你也可以把MSG放在前面，只是`\prvtmsg`前面要空格



另外此次commitment也修正了check_name中的小错误；改正了布局和排版（费了很多事，不过现在看起来工整多了）；废除了fds数组，完全采用User结构体数组

