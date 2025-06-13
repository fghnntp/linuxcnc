This project can use linuxcnc for simulator model.
Here are some tips for linuxcnc in simulation mode.
1. rtapi in user space must eclipse the POSIX::wait time for quick
simulate, return would let one CPU in 100% usage, which is not sense
in multicores CPU.
2. Don't change the cycle for emcmot but aff some function 
3. Batch transport must active for the large data trans.
