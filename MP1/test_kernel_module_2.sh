rmmod mp1
make
insmod mp1.ko
./userapp &
./userapp &
wait
cat /proc/mp1/status
