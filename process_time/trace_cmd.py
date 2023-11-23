class TraceCmd():
    def __init__(self, pid=None) -> None:
        self.plugin = "function_graph"
        self.pid = pid
        self.report_file = "trace.report"
    
    def get_cmd(self, type):
        if(type == 'report'):
            return self.build_report_cmd()
        elif(type == 'record'):
            return self.build_record_cmd()
    
    def build_record_cmd(self) -> str:
        cmd = ['sudo', 'trace-cmd', 'record', "-p {} ".format(self.plugin)]
        if(self.pid != None):
            cmd.append("-P {} ".format(self.pid))
        cmd.append('sleep')
        cmd.append('1')
        return cmd

    def build_report_cmd(self) -> str:
        cmd = "trace-cmd report > {}".format(self.report_file)
        return cmd