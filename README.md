# FingerBoard

## Changes

 * 可用命令行设置密码（与指纹1对1） 数据保存在EEPROM中 上电读取
 * 使用 UTF-8 编码
 * 抓包获得神秘接口[Get_Free_Position](https://github.com/Hcreak/FingerBoard/blob/fd340ab99904085fc42d6c9bfcdca59eccf24364/fingerprintWrapper.cpp#L413) 可得到特征库空缺位置 （暂时未使用）

## Useage

 * 串口默认比特率 `115200`

### 添加指纹

    命令： A,<id>
    参数： id 指纹索引ID

### 设置密码

    命令： P,<id>,<password>;
    参数： id 指纹索引ID
          password 密码明文 因为是以 ; 判断结束 所以不能包含 ;

### 清空指纹和密码

    命令： D

---
# README by peng-zhihui
### 给机械键盘添加指纹识别功能，这是Arduino固件的代码，详细项目介绍请看这篇文章:

[如何制作一个带指纹识别的机械键盘 - 稚晖的文章 - 知乎](https://zhuanlan.zhihu.com/p/64809151)



### 使用方法：

在串口助手中打开Arduino串口号，波特率115200，放上手指会输出信息。

添加手指的方法：串口发送 **A,3 换行** 则表示注册手指为3号，根据串口提示放上手指收到ok即可。

删除手指的方法：直接添加一个覆盖，或者发送 **D 换行** 删除所有手指数据。

可以自行修改CmdCheck函数添加自己的指令。

好用请给个星星~

![](https://pic2.zhimg.com/80/v2-c36e5037d18e0e1221b053a897cdcbc5_hd.jpg)

![](https://pic3.zhimg.com/80/v2-e08f3cd50c4af32c399af8d1e325d2be_hd.jpg)

