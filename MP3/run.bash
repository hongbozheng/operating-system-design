rmmod mp3
make
insmod mp3.ko
mknod node c 247 0
nice ./work 1024 R 50000 & nice ./work 1024 R 10000
wait
./monitor > profile1.data
nice ./work 1024 R 50000 & nice ./work 1024 L 10000
wait
./monitor > profile2.data
cp profile* ../../linux