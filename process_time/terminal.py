from trace_cmd import *
import subprocess
import signal
import os

RECORD_CMD = 'record'

class Terminal:
    def __init__(self, trace_cmd) -> None:
        self.trace_cmd: TraceCmd = trace_cmd

    def run(self):
        self.record()

    def record(self):
        cmd = self.trace_cmd.get_cmd(RECORD_CMD)
        subprocess.run(cmd)
        
    def report():
        pass