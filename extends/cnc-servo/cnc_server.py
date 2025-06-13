"""
    This file is the man function
"""
from fastapi import FastAPI
from nc_manager import NCManager

app = FastAPI(title="NC管理REST API示例")

# 初始化NC文件管理器，指定存储目录和最大容量
nc_manager = NCManager(nc_dir="./data/nc/", max_size_bytes=100*1024*1024*1024)

# 注册API路由
app.include_router(nc_manager.router)

if __name__ == "__main__":
    import uvicorn
    uvicorn.run("cnc_server:app", host="0.0.0.0", port=8000, reload=True)