Import("env")

def silent_xiaozhi(env, node):
    file_path = node.get_path()
    
    # 如果路径中包含 xiaozhi-mcp，则针对该文件关闭弃用警告
    if "xiaozhi-mcp" in file_path:
        return env.Object(node, CCFLAGS=env['CCFLAGS'] + ["-Wno-deprecated-declarations"])
    
    return node

# 将拦截器挂载到构建中间件
env.AddBuildMiddleware(silent_xiaozhi)
