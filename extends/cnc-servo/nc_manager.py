"""
This module support the FastAPI realization. It can managerde by
cnc_sever directly.
"""
import os
import shutil
import time
from fastapi import APIRouter, UploadFile, File, Form, HTTPException
from typing import List
from simulation_manager import SimulationManager

simulation_manager = SimulationManager()

class NCManager:
    def __init__(self, nc_dir: str = "/data/nc/", max_size_bytes: int = 100 * 1024 * 1024 * 1024):
        self.nc_dir = nc_dir
        self.max_size = max_size_bytes
        os.makedirs(self.nc_dir, exist_ok=True)
        self.router = APIRouter()
        self.define_routes()

    # 内部文件操作
    def list_nc_files(self) -> List[dict]:
        files = []
        for f in os.listdir(self.nc_dir):
            if f.lower().endswith('.nc'):
                fp = os.path.join(self.nc_dir, f)
                stat = os.stat(fp)
                files.append({
                    "filename": f,
                    "size": stat.st_size,
                    "atime": stat.st_atime,
                    "mtime": stat.st_mtime
                })
        return sorted(files, key=lambda x: x["mtime"], reverse=True)

    def save_file(self, up_file: UploadFile):
        file_path = os.path.join(self.nc_dir, up_file.filename)
        # 获取即将上传文件大小
        up_file.file.seek(0, 2)  # 移动到文件末尾
        file_size = up_file.file.tell()
        up_file.file.seek(0)     # 回到开头

        # 检查空间（已用 + 新文件）是否超限
        used = self.get_total_size()
        if used + file_size > self.max_size:
            raise HTTPException(
                status_code=400,
                detail=f"空间不足：当前已用{used}字节，上传文件{file_size}字节，超过最大{self.max_size}字节"
            )

        print(file_path)
        with open(file_path, "wb") as f:
            shutil.copyfileobj(up_file.file, f)
        return file_path

    def delete_file(self, filename: str):
        file_path = os.path.join(self.nc_dir, filename)
        if os.path.exists(file_path):
            os.remove(file_path)
            return True
        return False

    def get_total_size(self) -> int:
        return sum(f["size"] for f in self.list_nc_files())

    def auto_cleanup(self):
        # 超容量时按atime删除最久未访问的文件
        files = self.list_nc_files()
        total_size = sum(f["size"] for f in files)
        if total_size <= self.max_size:
            return
        # 按最后访问时间升序排序
        files = sorted(files, key=lambda x: x["atime"])
        for f in files:
            if total_size <= self.max_size:
                break
            fp = os.path.join(self.nc_dir, f["filename"])
            try:
                os.remove(fp)
                total_size -= f["size"]
            except Exception:
                pass

    # FastAPI 路由定义
    def define_routes(self):
        ### NC file manages
        @self.router.get("/api/v1/programs/list")
        async def api_list_nc():
            return self.list_nc_files()

        @self.router.post("/api/v1/programs/upload")
        async def api_upload_nc(file: UploadFile = File(...)):
            if not file.filename.lower().endswith('.nc'):
                raise HTTPException(status_code=400, detail="仅允许上传.nc文件")
            path = self.save_file(file)
            return {"status": "ok", "path": path}

        @self.router.delete("/api/v1/programs/{filename}")
        async def api_delete_nc(filename: str):
            success = self.delete_file(filename)
            if not success:
                raise HTTPException(status_code=404, detail="文件不存在")
            return {"status": "deleted", "filename": filename}

        @self.router.get("/api/v1/programs/usage")
        async def api_usage():
            files = self.list_nc_files()
            total_size = sum(f["size"] for f in files)
            return {
                "total_size_bytes": total_size,
                "file_count": len(files),
                "max_size_bytes": self.max_size
            }

        # Simulation task management
        @self.router.post("/api/v1/programs/simulate")
        async def api_simulate_nc(filename: str = Form(...)):
            # ... 检查文件逻辑 ...
            filename = self.nc_dir + filename
            task_id = simulation_manager.submit_task(filename)
            return {"status": "submitted", "task_id": task_id}

        @self.router.get("/api/v1/programs/simulate/{task_id}")
        async def api_get_sim_status(task_id: str):
            status = simulation_manager.get_status(task_id)
            if not status:
                raise HTTPException(404, "任务不存在")
            return status

        @self.router.get("/api/v1/programs/simulate")
        async def api_list_sim_tasks():
            return simulation_manager.list_tasks()