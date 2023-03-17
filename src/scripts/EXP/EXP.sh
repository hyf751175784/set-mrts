#PROC_NUM=`cat /proc/cpuinfo | grep "processor" | wc -l`
PROC_NUM=4
ROOT=`pwd`
FOLDER="EXP"
PARAM_NUM=288
GAP=`expr $PARAM_NUM / $PROC_NUM `

rm -rf $FOLDER

if(($PARAM_NUM > `expr $GAP \* $PROC_NUM`))
then
	GAP=`expr $GAP + 1`
fi
mkdir $FOLDER
for ((index=0; index < $PROC_NUM; index++))
do
	cd $ROOT
	mkdir "$FOLDER/sp$index"
#	cp SubTest "$FOLDER/sp$index"
	cp config.xml "$FOLDER/sp$index"
	cp start.sh "$FOLDER/sp$index"
	START=`expr $index \* $GAP`
	END=`expr $START + $GAP - 1`
	for ((index2=$START; index2 <= $END; index2++))
	do
		echo "$index2" >> "$FOLDER/sp$index/param"
	done
	cd $FOLDER/sp$index
#	echo "--------------Processor $index--------------"
#	nohup ./start.sh &
done

