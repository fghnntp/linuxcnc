#!/usr/bin/env python3
import hal
import time
import os


class ScopeCtroller:
    def __init__(self):
        pass
    
    def stop_scope(self):
        os.system("halcmd sets sig_ctrl_cmd 0")
        os.system("halcmd setp ctrl_cmd 0")
        
    def start_scope(self):
        os.system("halcmd sets sig_ctrl_cmd 1")
        os.system("halcmd setp ctrl_cmd 1")

    def clear_scope(self):
        os.system("halcmd sets sig_ctrl_cmd 2")
        os.system("halcmd setp ctrl_cmd 2")
        