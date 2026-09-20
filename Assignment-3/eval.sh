pgms="1 2"
sizes="16 32 64 128 256 512 1024 2048"
trails=10

rm -rf data
mkdir data

for s in $sizes; do
    for p in $pgms; do
	if [[ ! -a filter$p.$s.times ]]; then
	    echo "running <./build/filter images/${s}x${s}.bmp $p $s $s> $trails times"
	    for ((i=0; i<$trails; i++)); do
		./build/filter images/${s}x${s}.bmp $p $s $s >> data/filter$p.$s.out
	    done
	fi
    done
done
	 
