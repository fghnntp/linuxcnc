#!/usr/bin/env python3
import hal
import time

def main():
    # HAL pin 完整名称
    ctrl_cmd_pin = 'ctrl_cmd'  # 请根据实际 HAL pin 名称替换

    # 创建 HAL 组件（仅用于访问 pin，不会新建 pin）
    h = hal.component("scope_ctrl_cmd")
    h.newpin("in", hal.HAL_S32, hal.HAL_IN)
    h.newpin("out", hal.HAL_S32, hal.HAL_OUT)
    h.ready()

    if not hal.component_exists("blk_scope"):
        print("blk_scope not loaded")
        return
    
    if not hal.component_is_ready("blk_scope"):
        print("blk_scope is not ready")
        return

    hal.new_sig("sig_ctrl_cmd",hal.HAL_S32)
    hal.connect("scope_ctrl_cmd.in", "sig_ctrl_cmd")
    hal.connect("ctrl_cmd", "sig_ctrl_cmd")
    hal.connect("scope_ctrl_cmd.out", "sig_ctrl_cmd")


    print("请输入指令编号：0=Stop, 1=Start, 其它=Exit")
    try:
        while True:
            try:
                print("请输入数字:")
                print("0:STOP")
                print("1:START")
                print("2:CLEAR MSG")
                print("其它:Exit")
                key = input("输入 ctrl_cmd 值: ")
                cmd = int(key)
            except ValueError:
                print("请输入数字:")
                print("0:STOP")
                print("1:START")
                print("2:CLEAR MSG")
                continue
            if cmd not in [0, 1, 2]:
                print("退出")
                break
            h['in'] = cmd
            print("设置 {} = {}".format(ctrl_cmd_pin, cmd))
            time.sleep(0.1)
    except KeyboardInterrupt:
        print("\n手动中断，退出程序。")

if __name__ == '__main__':
    main()