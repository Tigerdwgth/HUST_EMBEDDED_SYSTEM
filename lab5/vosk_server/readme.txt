
语音识别本地服务器server和libvosk.so的运行需要依赖高版本的glibc环境, 
所以把高版本glibc-2.31环境的动态库拷贝到该目录，

把该目录(vosk_server)拷贝到PYNA-Z1开发板上的任意目录,
通过 run_server.sh 来执行 server

word.txt可自定义识别词语, 请保持json格式并在一行中添加

#=================================================
语音识别库：https://github.com/alphacep/vosk-api
官网地址：https://alphacephei.com/vosk/
中文识别模型vosk-model-small-cn-0.3.zip
模型下载地址: https://alphacephei.com/vosk/models
自行训练参考: https://github.com/alphacep/vosk-api/tree/master/training
解析语音识别结果, json解析库yyjson.c和yyjson.h https://github.com/ibireme/yyjson

