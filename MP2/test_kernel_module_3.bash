rmmod mp2.ko
make
insmod mp2.ko
./userapp 1000 200 10 &
./userapp 2000 350 10 &
./userapp 3000 500 10 &
wait