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

## 版本号与标签

项目使用 `v主版本.次版本.修订号` 格式的标签标记每一次提交，例如 `v0.1.0`。

### 首次克隆后必须执行一次

仓库里的自动版本钩子放在 `.githooks/` 目录下，Git 不会自动启用，**每个新克隆的仓库都要执行一次**：

```
git config core.hooksPath .githooks
```

没有执行这一句，之后提交不会自动生成版本标签，也不会报错，很容易漏掉，请务必确认。

### 日常使用

提交时钩子会自动读取本次提交的说明文字，决定版本号怎么涨：

| 提交说明包含 | 版本变化 | 例子 |
| --- | --- | --- |
| `BREAKING` 或 `[major]` | 主版本 +1 | `v0.3.2` → `v1.0.0` |
| `feat` 开头，或含 `[minor]` | 次版本 +1 | `v0.3.2` → `v0.4.0` |
| 其他 | 修订号 +1 | `v0.3.2` → `v0.3.3` |

因此提交说明请尽量写成 `feat: 新增敌人巡逻`、`fix: 修复跳跃穿墙` 这种形式，版本号才会正确反映改动大小。

两个可选的临时开关（只在当次命令生效）：

```
BUMP=major git commit -m "重构技能系统"      # 强制按主版本递增
SKIP_VERSION_TAG=1 git commit -m "临时提交"  # 本次不打标签
```

查看当前版本用 `git describe --tags`。

### 推送

标签默认不会随提交一起上传，推送时请带上 `--tags` 或 `--follow-tags`：

```
git push origin main --follow-tags
```

注意 `git commit --amend` 和 `git rebase` 会把提交替换成新的提交，旧标签会留在被丢弃的提交上，这时候需要手动 `git tag -d 旧标签` 清理，或者用 `SKIP_VERSION_TAG=1` 避免多打一个标签。

### 版本回退

- 想查看某个版本的文件：`git switch v0.1.0`（看完用 `git switch main` 回来）。
- 想撤销已经推到主干的某次改动：`git revert <提交号>`，不要用 `git reset`。
- 只想把某个资源退回旧版本：`git restore --source=v0.1.0 -- 文件路径`，如果本地没有该版本的原始资源，再执行一次 `git lfs pull`。
