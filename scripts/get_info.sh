
for HOST in $(cat node)
do
	CORE_NUM=0
	CORE_NUM=`ssh -t hadoop@$HOST 'cat /proc/cpuinfo | grep "processor" | wc -l'`
	if [ $CORE_NUM > 0 ]
	then
		echo "$HOST $CORE_NUM"
		echo $CORE_NUM > node_info/$HOST
	fi
done
