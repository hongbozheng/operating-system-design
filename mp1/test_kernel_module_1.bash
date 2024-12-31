rmmod mp1
make
insmod mp1.ko
./userapp &
wait
cat /proc/mp1/status