# import ctypes as ct

class DataCollector:
    def __init__(self) -> None:
        self.pid_to_process_name = {}

    def collect(self, perf_data, *args, **kwargs):
        pid = kwargs['pid']
        pname = kwargs['pname']
        self.pid_to_process_name[pid] = pname