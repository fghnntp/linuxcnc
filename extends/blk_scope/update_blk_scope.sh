# update the blk_scope
cp  build/libblk_scope.so ~/srcCode/linuxcnc-master/rtlib/blk_scope.so
sudo cp build/libblk_scope.so /usr/lib/linuxcnc/modules/blk_scope.so
# load blk_scopt to rt area
halcmd loadrt blk_scope 
# halcmd net rtcp1 joint.0.pos-cmd  input1
# halcmd net rtcp2 joint.1.pos-cmd  input2
# halcmd net rtcp3 joint.2.pos-cmd  input3
# halcmd net rtcp4 joint.3.pos-cmd  input4
# halcmd net rtcp5 joint.4.pos-cmd  input5
halcmd addf blk_scope_catch_cycle servo-thread