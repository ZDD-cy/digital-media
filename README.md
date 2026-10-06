# digital-media
这是数媒的项目库
请提交文件后在这里说明所提交的文件是什么作用
请安装Git LFS配合版本控制

## 文档

- [Content 文件管理结构说明](LanRan/Content/README.md)

## 版本控制（Git LFS）

项目已启用 Git LFS，仓库根目录的 `.gitattributes` 声明了需要用 LFS 存储的文件类型：

- Unreal 资产：`.uasset`、`.umap`、`.ubulk`、`.uexp`
- 模型与动画源文件：`.fbx`、`.obj`、`.gltf`、`.blend`、`.max` 等
- 贴图与图像：`.png`、`.jpg`、`.tga`、`.psd`、`.exr`、`.hdr` 等
- 音频与视频：`.wav`、`.mp3`、`.mp4`、`.mov` 等
- 字体与素材压缩包：`.ttf`、`.otf`、`.zip`、`.7z`
- 说明：`.uproject`、`.ini`、`.md` 等文本文件仍由 Git 直接管理

使用步骤：

1. 安装 Git LFS（`git lfs install`），克隆后执行一次即可。
2. 正常 `git add` / `git commit`，命中规则的文件会自动转成 LFS 指针。
3. 首次拉取大文件时执行 `git lfs pull`，确认资源已下载到本地。
4. 用 `git lfs ls-files` 查看当前被 LFS 管理的文件清单。

`.gitignore` 已排除 `Binaries/`、`Intermediate/`、`DerivedDataCache/`、`Saved/` 等引擎生成目录，这些内容不需要也不应该提交。
