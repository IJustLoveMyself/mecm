# mecm
MECM（Mini Ethercat Master）一个简易的ethercat主站，目前实现了基本的同行功能，上层应用实现了COE与FOE两个常用功能

文件结构

~~~
-- inc mecm头文件
-- port mecm接口文件
-- src mecm源文件
-- test mecm使用例子
~~~

移植说明：移植时只需要inc、port、src三个文件，需要根据自己的平台对port文件中的接口进行实现
