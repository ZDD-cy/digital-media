# Unreal MCP 接入说明

## 当前方案

`LanRan` 使用 [Monolith](https://github.com/tumourlove/monolith) 作为 Unreal
Editor 的 MCP 桥接：

1. Monolith 插件运行在 Unreal Editor 进程内，在 `127.0.0.1:9316` 提供 MCP
   HTTP 服务。
2. `monolith_proxy.exe` 把 Codex 的 stdio MCP 连接转发给编辑器。
3. Codex 通过 `blueprint_query`、`material_query`、`editor_query` 等命名空间
   工具读取和修改 UE 项目。

当前锁定版本为 `v0.24.0`，面向 Unreal Engine `5.8`。发布包 SHA-256：

```text
a12674c3bd35e48c92138531e1a3c5d85d0e630bc017e10466809bf93d5483b1
```

## 首次安装

在仓库根目录执行：

```powershell
powershell -ExecutionPolicy Bypass -File Tools/Install-MonolithMCP.ps1
```

脚本会：

- 下载针对 UE 5.8 的 Monolith 发布包；
- 校验 SHA-256；
- 解压到 `LanRan/Plugins/Monolith`；
- 检查代理程序；
- 在 Codex CLI 可用时注册全局 `monolith` MCP 服务。

安装完成后重启 Unreal Editor。第一次启动会建立项目和引擎索引，耗时取决于
机器和资产数量。输出日志出现以下内容即表示编辑器端已启动：

```text
Monolith MCP server listening on port 9316
```

然后新开 Codex 会话，调用 `monolith_status` 验证。当前工作会话不会自动刷新
新注册的 MCP 工具。

## 手动注册 Codex

如果安装脚本跳过了注册，可执行：

```powershell
codex mcp add monolith -- "D:\UserDate\Documents\GitHub\digital-media\LanRan\Plugins\Monolith\Binaries\monolith_proxy.exe"
```

代理会在编辑器重启时保持 MCP 会话，并在编辑器下线时返回可重试的错误。

## 使用约定

- 先调用 `monolith_status`。
- 用 `monolith_guide` 了解工作流。
- 用 `monolith_discover` 查找命名空间和动作。
- 用 `describe_query("action_schema", ...)` 获取动作参数，避免猜测字段。
- 修改资产前先读取当前状态，并明确目标资产路径。

## 安全与关闭

Monolith 当前实际监听 `127.0.0.1:9316`，只接受本机连接。MCP 具有编辑器读写
权限，运行期间不要执行来源不明的脚本或工具调用。

需要临时关闭时，在 Unreal Editor 中打开
`Editor Preferences > Plugins > Monolith`，取消 `MCP Server Enabled` 并重启
编辑器。也可以使用编辑器命令行参数 `-NoMonolithMcp` 为单次编辑器进程禁用
MCP。

## 更新

更新版本时需要同步修改
`Tools/Install-MonolithMCP.ps1` 中的版本号、下载地址和 SHA-256。升级后重启
编辑器，并重新确认 `monolith_status`。

## 验证记录

2026-10-21 已在 `LanRan` 中完成验证：

- Monolith `0.24.0`
- Unreal Engine `5.8.3`
- 编辑器端健康检查：`200 OK`
- 已注册动作：`1362`
- 命名空间：`26`
