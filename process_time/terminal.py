from trace_cmd import *
from trace_cmd_parser import *
import subprocess
import time

class Terminal:
    def __init__(self, trace_cmd: TraceCmd, parser: Parser) -> None:
        self.trace_cmd = trace_cmd
        self.parser = parser

    def run(self):
        # self.record()
        # self.report()
        # self.parser.parse(self.trace_cmd.get_report_file())
        self.parser.execute()
        
        #self.parse_file()
        #self.parse_trace_text()

    def record(self) -> None:
        cmd = self.trace_cmd.get_cmd(RECORD_CMD)
        subprocess.run(cmd)
        time.sleep(2)
        
    def report(self) -> None:
        cmd = self.trace_cmd.get_cmd(REPORT_CMD)
        subprocess.run(cmd, shell=True)
        time.sleep(2)
        