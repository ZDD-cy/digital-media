# UI、动画与媒体接入说明

## 已导入资产

- 花版按钮：`/Game/UI/Textures/T_Mask_A_Blue`、`T_Mask_B_Blue`、`T_Mask_C_Blue`（用户遮罩图的蓝底版本）
- 全局背景音乐：`/Game/Audio/SW_BackgroundMusic`（SoundWave，循环播放）
- 开场视频：`/Game/Movies/MS_Opening`（File Media Source）
- 视频副本：`LanRan/Content/Movies/Opening.mp4`
- 音乐源文件副本：`LanRan/Content/Movies/BGM.mp3`

## 运行流程

1. 进入关卡后显示米黄色开始页。
2. 点击“开始游戏”后启动全局背景音乐和开场视频。
3. 开场结束或点击“跳过片头”后进入游戏场景。
4. 制作阶段的 UMG 只保留侧栏、标题、进度、按钮、温度卡片和文字；其余区域透明，直接显示 3D 游戏画面。
5. 阶段的标题、右侧操作区、温度卡片和完成卡片带淡入/位移动画。

## 媒体兼容性

原始 MP4 为 HEVC 编码。为了避免 Windows 缺少 HEVC 解码扩展，项目内副本已转为 H.264/AAC，并保留原下载文件不改动。Windows 使用 `WmfMedia` 播放开场视频。

`DefaultGame.ini` 已配置 `Movies` 目录按 Non-UFS 文件随包复制，确保运行时视频文件可用。

## 关卡说明

`L_TestLight.umap` 当前是 Git LFS 合并冲突占位（289 字节），不是可用关卡。项目启动地图暂时指向已验证完整的 `L_TestLight_Backup`；待原关卡从 LFS 恢复后再切回。
