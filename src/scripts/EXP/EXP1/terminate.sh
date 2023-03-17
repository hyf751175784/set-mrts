for PID in `ps -ef | grep "./start.sh" | awk '{print $2}'`
do
	kill -9 $PID
done

for PID in `ps -ef | grep "./SubTest" | awk '{print $2}'`
do
	kill -9 $PID
done
