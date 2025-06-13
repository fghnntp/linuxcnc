#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import linuxcnc
import os
import time

class LinuxCNCManager:
    def __init__(self):
        try:
            self.stat = linuxcnc.stat()
            self.command = linuxcnc.command()
            self.error_channel = linuxcnc.error_channel()
        except linuxcnc.error as detail:
            raise RuntimeError(f"无法连接到LinuxCNC: {detail}")

    def get_status(self):
        """获取当前状态字典"""
        self.stat.poll()
        status = {}
        for x in dir(self.stat):
            if not x.startswith("_"):
                try:
                    status[x] = getattr(self.stat, x)
                except Exception:
                    pass
        return status

    def get_errors(self):
        """获取LinuxCNC错误通道的消息"""
        # 一次性取出所有错误消息
        errors = []
        while True:
            try:
                err = self.error_channel.poll()
                if err:
                    errors.append(err)
                else:
                    break
            except Exception:
                break
        return errors

    def is_parser_idle(self):
        status = self.get_status()
        print(status.get("interp_state"))
        if status.get("interp_state") == linuxcnc.INTERP_IDLE:
            return True
        else:
            self.command.reset_interpreter()
            return False

    
    def start_program(self, filename):
        """装载并运行指定的NC程序"""
        # 这里假定你已设置好配置文件和环境
        # Whatever abort the cnc and reset interpreter
        self.command.abort()
        self.command.reset_interpreter()

        self.command.mode(linuxcnc.MODE_MDI)
        self.command.mdi("G00G53X0Y0Z0A0B0C0")

        self.command.abort()
        self.command.reset_interpreter()

        #Open the file and the run it
        self.command.mode(linuxcnc.MODE_AUTO)
        self.command.program_open(os.path.abspath(filename))

        self.command.auto(linuxcnc.AUTO_RUN, 0)



# ========== 测试代码 ==========
if __name__ == "__main__":
    cnc = LinuxCNCManager()
    
    cnc.is_parser_reading()
    # 以下操作请确保环境允许，否则可能会影响实际机床！
    cnc.start_program("./data/nc/007010_converted.nc")
