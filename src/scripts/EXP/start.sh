#for p_index in `cat param`
while((`wc -l param` > 0))
do
	echo $p_index
	sed -i "/$p_index/d" param
#	./SubTest $p_index
	echo $p_index >> Finished
done
