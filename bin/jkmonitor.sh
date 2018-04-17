#!/bin/sh
# 函数: CheckProcess
# 功能: 检查一个进程是否存在
# 参数: $1 --- 要检查的进程名称
# 返回: 如果存在返回0, 否则返回1.
#------------------------------------------------------------------------------
Pnum=10
CheckProcess()
{
	# 检查输入的参数是否有效
	if [ "$1" = "" ];
	then
		return 1
	fi

	#$PROCESS_NUM获取指定进程名的数目，为1返回0，表示正常，不为1返回1，表示有错误，需要重新启动
	PROCESS_NUM=`ps | grep "$1" | grep -v "grep" | wc -l` 	
	if [[ $PROCESS_NUM -eq $Pnum ]];
	then
		return 0
	else
		return 1
	fi
}

StartProcess()
{
	ret=`ps | grep "/JKJN/UartToNet $1" | grep -v "grep" | wc -l`
	if [[ $ret -eq 0 ]];
	then
		/JKJN/UartToNet $1
	fi
} 
 
# 检查/JKJN/UartToNet实例是否已经存在
while [ 1 ] ; do
	CheckProcess "/JKJN/UartToNet"
	CheckQQ_RET=$?
	if [ $CheckQQ_RET -eq 1 ];
	then
	# 杀死所有/JKJN/UartToNet进程
	#ps | grep 'UartToNet'|grep -v 'grep'|awk '{print$1}'|xargs kill
	Pnum=`grep "\[Gun_Info" /JKJN/sys.ini |wc -l`
		case $Pnum in
		1)
			ps | grep 'UartToNet'|grep -v 'grep'|awk '{print$1}'|xargs kill		
			/JKJN/UartToNet 1
		;;
		2)
			StartProcess 1
			StartProcess 2
		;;
		3)
			StartProcess 1
			StartProcess 2
			StartProcess 3
		;;
		4)
			StartProcess 1
			StartProcess 2
			StartProcess 3
			StartProcess 4			
		;;
		esac 
	fi
	sleep 1
done
