MAJOR_NUM=423
MINOR_NUM=0

rmmod mp3
make
insmod mp3.ko
mknod node c $MAJOR_NUM $MINOR_NUM
#nice ./work 1024 R 50000 & nice ./work 1024 R 10000
#wait
#./monitor > profile1.data
#nice ./work 1024 R 50000 & nice ./work 1024 L 10000
#wait
#./monitor > profile2.data

for i in {1..20}; do
  for _ in $(seq 1 $i); do nice ./work 200 R 10000 & done
  wait
  ./monitor > profile_${i}.data
done
cp profile* ../../linux/hello