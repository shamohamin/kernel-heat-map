from trace_cmd import *
import subprocess
import signal
import os


class Terminal:
    def __init__(self, trace_cmd: TraceCmd) -> None:
        self.trace_cmd = trace_cmd

    def run(self):
        self.record()
        self.report()

    def record(self) -> None:
        cmd = self.trace_cmd.get_cmd(RECORD_CMD)
        subprocess.run(cmd)
        
    def report(self) -> None:
        cmd = self.trace_cmd.get_cmd(REPORT_CMD)
        subprocess.run(cmd, shell=True)
        