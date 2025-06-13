"""
Simulation must to managed, this module is used to manage it. It will
affected by nc_manager module, which realize the REST API simulation
parts.
"""
import uuid
import time

class SimulationTask:
    def __init__(self, filename: str):
        self.id = str(uuid.uuid4())
        self.filename = filename
        self.status = "pending"  # 可为pending, running, finished, failed
        self.result = None
        self.start_time = None
        self.end_time = None

import threading
from linuxcnc_interface import LinuxCNCManager
from hal_scope_interface import ScopeCtroller
from simulation_result_manager import SimResManager

class SimulationManager:
    def __init__(self):
        self.tasks = {}  # id -> SimulationTask
        self.cnc = LinuxCNCManager()
        self.scope = ScopeCtroller()
        self.simu_res_manager = SimResManager()

    def submit_task(self, filename: str):
        task = SimulationTask(filename)
        self.tasks[task.id] = task
        print("submit task")
        # 启动仿真线程
        threading.Thread(target=self._run_task, args=(task,), daemon=True).start()
        return task.id

    def _run_task(self, task: SimulationTask):
        task.status = "running"
        task.start_time = time.time()
        try:
            # 这里写你的仿真逻辑，例：time.sleep(5)
            print(f"try to satrt {task.filename}")
            self.scope.clear_scope()
            self.scope.start_scope()
            # Start the file simulation
            self.cnc.start_program(task.filename)
            
            while self.cnc.is_parser_idle() == False:
                time.sleep(1)
            self.scope.stop_scope()

            self.simu_res_manager.catch_result()
            print("before save")
            self.simu_res_manager.save_result(task.id)
            print("afeter save")
            
            task.result = f"仿真完成: {task.filename}"

            task.status = "finished"
        except Exception as e:
            task.result = str(e)
            task.status = "failed"
        finally:
            task.end_time = time.time()
            print(f"Task {task.id} eclipse {task.end_time - task.start_time}")

    def get_status(self, task_id: str):
        task = self.tasks.get(task_id)
        if not task:
            return None
        return {
            "id": task.id,
            "filename": task.filename,
            "status": task.status,
            "result": task.result,
            "start_time": task.start_time,
            "end_time": task.end_time,
        }

    def list_tasks(self):
        return [self.get_status(tid) for tid in self.tasks]