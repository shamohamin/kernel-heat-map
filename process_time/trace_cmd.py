RECORD_CMD = 'record'
REPORT_CMD = 'report'

class TraceCmd():
    def __init__(self, pid=None) -> None:
        self.plugin = "function_graph"
        self.pid = pid
        self.report_file = "trace.report"
    
    def get_cmd(self, type: str) -> list:
        if(type == REPORT_CMD):
            return self.build_report_cmd()
        elif(type == RECORD_CMD):
            return self.build_record_cmd()
    
    def build_record_cmd(self) -> list:
        cmd = ['sudo', 'trace-cmd', 'record', "-p {} ".format(self.plugin)]
        if(self.pid != None):
            cmd.append("-P {} ".format(self.pid))
        cmd.append('sleep')
        cmd.append('10')
        return cmd

    def build_report_cmd(self) -> list:
        cmd = "trace-cmd report > {}".format(self.report_file)
        return cmd
    
    def get_report_file(self) -> str:
        return self.report_file