import os
import json
import threading
from typing import Optional, Dict, Any
import redis

class SimResManager:
    """
    仿真结果管理器。
    支持将仿真结果保存为JSON文件，按任务ID/文件名检索，线程安全。
    """

    def __init__(self, base_dir: str = "./sim_results"):
        self.base_dir = base_dir
        os.makedirs(self.base_dir, exist_ok=True)
        self.r = redis.Redis(host='0.0.0.0', port=6379, db=0)
        self.catch_lock = threading.Lock()
        self.save_lock = threading.Lock()
        self.raw_lines = []

    def _get_result_path(self, task_id: str) -> str:
        return os.path.join(self.base_dir, f"{task_id}.txt")
    
    def catch_result(self):
        print("Catch Resutl")
        with self.catch_lock:
            self.raw_lines.clear()
            while True:
                raw = self.r.rpop('rtcp')
                if not raw:
                    break
                line = raw.decode('utf-8')
                self.raw_lines.append(line)  # 新增：保存原始字符串

    def save_result(self, task_id: str) -> None:
        path = self._get_result_path(task_id)
        print(path)
        with self.save_lock:
            with open(path, "w", encoding="utf-8") as f:
                for line in self.raw_lines:
                    f.write(line + '\n')

    def load_result(self, task_id: str) -> Optional[Dict[str, Any]]:
        """
        读取指定task_id的仿真结果，若不存在返回None。
        """
        path = self._get_result_path(task_id)
        if not os.path.isfile(path):
            return None
        with self.lock:
            with open(path, "r", encoding="utf-8") as f:
                return json.load(f)

    def list_results(self) -> Dict[str, Dict[str, Any]]:
        """
        返回所有仿真结果的字典，key为task_id。
        """
        results = {}
        with self.lock:
            for fname in os.listdir(self.base_dir):
                if fname.endswith(".json"):
                    task_id = fname[:-5]
                    path = os.path.join(self.base_dir, fname)
                    try:
                        with open(path, "r", encoding="utf-8") as f:
                            results[task_id] = json.load(f)
                    except Exception:
                        continue
        return results

    def delete_result(self, task_id: str) -> bool:
        """
        删除指定task_id的仿真结果，成功返回True，否则False。
        """
        path = self._get_result_path(task_id)
        with self.lock:
            if os.path.isfile(path):
                os.remove(path)
                return True
            return False