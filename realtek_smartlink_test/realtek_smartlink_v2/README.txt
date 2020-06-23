附件列表说明：
ko/8188eu.ko：8188eu smart link驱动。
ko/8189es.ko：8189es smart link驱动。
ko/otg-hs.ko：otg驱动，跟以前的不变。
ko/sdio_wifi.ko: 这个是wifi电源控制驱动，内核已有这个驱动的。
SimpleConfigApp.apk：手机端APK。具体的使用请参看Document.zip压缩包里面的Android_Simple_Config_User_Guide_v0.3.pdf。
SimpleConfigRelease-20150106.tar.gz:	APK源码。
rtl8188EUS_rtl8189ES_linux_v4.3.17_13587.20150309.tar.gz: 这是支持smart link的8188eu/8189es驱动源码。
rtw_simple_config_v2.3.0_r13602.tar.gz: 可执行文件rtw_simple_config的源码。其中main.c的宏USB_WIFI配置编译usb还是sdio版本的测试程序。
Document.zip: realtek有关smart link的说明文档。比如apk的使用文档。
IOS.rar：smart link IOS平台的app代码。
IOS_Simple_Config_User_Guide_v1.1.pdf:	IOS端软件用户使用指导手册。


下面是操作步骤
1) 在手机安装realtek官方的SimpleConfigApp.apk
2) mount -t vfat /dev/mmcblk0p1 /mnt
3) cp /mnt/realtek_smartlink /tmp -rf		注意：需事先将rtw_simple_config程序和ko目录拷贝到realtek_smartlink下
4) /tmp/realtek_smartlink/rtw_simple_config -c /tmp/wpa.conf -d
5) 运行手机端已安装的realtek官方测试apk，按说明文档的步骤设置和连接wifi，点击“Start”按钮即可。

成功抓到ssid后串口打印类似：
network={
	ssid="SPC"
	scan_ssid=1
	psk="123456789"
}
并且这些信息也已存在于文件/tmp/wpa.conf
