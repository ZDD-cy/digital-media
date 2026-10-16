# 蓝染交互插件 · UE 5.8

交付范围：固定视角 3D、鼠标点击与拖放、P0 状态机、蓝图动画/材质接口，以及第一人称工作台、选版界面、折布夹版与四档染色演示。
这是 C++ 源码插件，放入项目 Plugins 后编译使用。源码包不包含预制 .uasset、正式模型、材质或 PAK，也不是完整成品游戏。第一人称接入见 [接入说明](Docs/第一人称工作台接入.md)。

## 验证状态

已读取本机引擎 `Build.version`，版本为 **5.8.3**。
2026-10-14 已通过该引擎的 Development Editor / Win64 编译及 DLL 链接，以及 Development Game / Win64 编译及验证工程 EXE 链接。目录权限和构建缓存路径问题已处理。
2026-10-16 已在本机 UE 5.8.3 完成第一人称工作台选版的编辑器与游戏目标编译、两组自动检查和 PIE 实测。A/B/C 拖入左右槽、同款双选、替换、错拖复位、清空、交换、确认锁定，以及按 E 退出并重新进入，均已实际操作验证。此前的命令行引擎初始化阻塞已不再阻止本轮检查。
选版与工作台切换的实测详见 `Docs/第一人称选版验证记录.md`。1.2.0 新增折布夹版演示，1.3.0 增加浸染/出缸显色、1～4 档深度、收工按钮及色样预览。关闭兜底计时器后由动作完成回调推进；正式布料、夹具与染缸动画和最终作品材质仍待接入。附带四组自动化测试源码和本机编译脚本。

## 1. 安装

1. 将整个 `IndigoInteraction` 文件夹复制到 `<项目>/Plugins/IndigoInteraction`。
2. 安装适用于 UE 的 Visual Studio C++ 工具链及 Windows SDK。关闭编辑器，重新生成项目文件，再编译 Development Editor。
3. 纯蓝图工程建议先通过编辑器添加一个空 C++ 类，再安装插件。
4. 启用插件，重启编辑器。C++ Classes / IndigoInteraction 中应出现插件类。
5. 可先在 PowerShell 运行下列脚本单独编译插件；输出目录必须是新的空目录，避免覆盖已有文件。

```powershell
& .\BuildPlugin.ps1 -EngineRoot 'D:\language\UE_5.7\UE_5.8' -PackageDirectory 'C:\UEBuilds\IndigoInteraction_583'
```

编译脚本只调用引擎官方 RunUAT BuildPlugin。不同引擎小版本需重新编译，不要直接复用 DLL。

## 2. 最小场景接线

1. 新建自己的 GameMode 蓝图，Player Controller Class 设为 `IndigoPlayerController` 或其蓝图子类；Default Pawn Class 设为 None。在关卡 World Settings 中选择该 GameMode。
2. 放置**一个** `IndigoDirector`。该 Actor 自带 `Flow` 组件，是本局数据唯一来源。
3. 放置三个 CameraActor，在 Director 的 Cameras 中分别指定 `Table`（工作台全景）、`Closeup`（工具近景）、`Quilt`（被面展示）。控制器不提供 WASD、旋转或缩放输入。
4. 创建 `IndigoInteractable` 蓝图子类，添加 StaticMesh 作为 HitBox 的子组件。HitBox 保持 Query Only、Visibility Block；可视模型建议关闭碰撞，禁用物理模拟。调整 HitBox 大小，使点击与拖放区域易命中。
5. 创建负责动画/布面显示的 Actor 蓝图，在 Class Settings 中添加 `IndigoVisualInterface`，将实例加入 Flow.VisualTargets。
6. 创建 UMG 界面，绑定 Flow 的 `OnSubtitle`、`OnStepChanged`、`OnResultChanged`。完成绑定后调用一次 `Flow.Restart`，获得初始字幕、机位和数据。按钮调用 `Flow.Perform`；按钮启用状态用 `CanPerform`/`IsInputLocked` 查询。

不要给道具再写独立的鼠标事件。控制器统一处理左键按下/释放、悬停、拖放、右键/Esc 取消；拖动物体使用经过抓取点的相机朝向平面，命中目标使用忽略被拖物件的 Visibility 射线。

UMG 按钮应消费鼠标事件；覆盖 3D 区域的可交互面板在鼠标进入/离开时调用控制器 `SetPointerBlocked(true/false)`，面板关闭时恢复 false。多个重叠面板请由一个 UI 管理器聚合阻挡状态。

## 3. 花版与其他拖放

| 对象 | Kind | ActiveStep | 其他字段 |
|---|---|---|---|
| 花版 A/B/C，各一份 | DragSource | Select | DragGroup=Pattern；Pattern=A/B/C；bReturnToSource=true |
| 左槽 | DropTarget | Select | DragGroup=Pattern；bLeftSlot=true |
| 右槽 | DropTarget | Select | DragGroup=Pattern；bLeftSlot=false |
| 确认按钮 | Click | Select | Action=Confirm |
| 交换按钮 | Click | Select | Action=Swap |
| 左/右清空按钮 | Click | Select | Action=ClearLeft/ClearRight |

花版拖到槽内即设置/替换该侧数据，花版源返回架上，允许 A/A。左右槽的显示由 SetPattern 刷新。不要在花版源上关闭 bReturnToSource。

折布、放布、合版也使用 DragSource + DropTarget。双方 DragGroup、Action、ActionIndex 必须匹配；ActiveStep 设为对应步骤。例如第一折：双方 Action=Fold、ActionIndex=0、DragGroup=Fold0。第二折使用 Index=1。可以放置每折独立道具，也可以在动画完成后更新同一对象的 ActionIndex。组件只允许当前 FoldIndex。

放布设置 Action=PlaceCloth；合版设置 Action=ClosePlate；左右夹具通常使用 Click。bReturnToSource=false 会在有效释放时先吸附到目标变换，然后调用动画接口；动画接管其后的位置。模型根点应与目标吸附点约定一致。

以下蓝图事件提供视觉反馈：

- `SetHover(bool)`：当前对象轻微高亮。
- `SetDropHighlight(bool)`：拖拽时合法目标轮廓。取消、释放或切步骤会清除。
- `InvalidDrop()`：物体已回到本次拖拽起点；在这里短暂闪烁合法区域，并重显当前提示，避免叠加字幕。

模型在每局内可能已移动或变形。`ResetVisuals` 必须把它们恢复到**开局**位置；控制器取消拖拽只恢复到**本次抓取**位置。

## 4. 状态与输入

| 状态 | 合法输入顺序 | 推进条件 |
|---|---|---|
| Receive | InspectCloth、InspectOrder，顺序不限 | 两项都查看 |
| Select | 花版拖放、Swap、ClearLeft/Right、Confirm | 左右均非空且确认 |
| Fold | Fold(Index=0...FoldCount-1) | 每折动画完成后开放下一折 |
| Clamp | PlaceCloth → ClosePlate → ClampLeft/Right | 两侧夹紧且动画完成 |
| Dye | Dye，可重复到第四档；FinishDye | 每段动画完成才加深度；至少一档才能收工 |
| Wash | Wash → Drain → UnclampLeft/Right → Unfold | 两夹具松开，展开动画完成 |
| Reveal | Deliver；回看模式只接受 Back | 展示可无限停留 |
| End | Review；另用 Restart 重开 | 回看 Back 直接返回 End |

`Flow.StepConfig` 可在编辑器配置每个状态的字幕与机位；`FoldCount` 默认 3。详见 `Docs/步骤配置.csv`。
接件点击只记录查看完成，委托单展示 UI 由项目接入；委托后页应只在 Deliver 动画中显示。

本版只实现用户确认的四项/P0 核心。试布、Cold/Ready、燃料与地域短片属于 P1，未实现。P0 染色直接允许合法的下一段，不模拟温度与燃料。Deliver 是交付动画挂接点，默认计时后进入结束页，不含正式剧情。物品几何、材质、字幕 UI、计时闪烁效果由蓝图/美术接入。

## 5. 动画、材质接口

| 接口/事件 | 用途 |
|---|---|
| ResetVisuals | 停止旧 Sequence/Timeline、恢复模型、材质和物品位置 |
| SetPattern(Left, Right) | 同步左右纹样，中央旧样由美术固定 |
| SetDyeProgress(CompletedDepth, Progress) | 已完成档位 + 当前阶段进度 0..1 |
| ShowFinalQuilt(Result) | 展开、回看和结束页显示同一份结果 |
| PlayIndigoAnimation(Cue, Token) | 请求动作动画，完成后回传本次 Token |
| Flow.OnAnimationRequested(Cue, Token) | 可选集中动画路由，与上个接口二选一作为播放入口 |

动画 Cue 与动作枚举名称相同：Fold、PlaceCloth、ClosePlate、ClampLeft、ClampRight、Dye、Wash、Drain、UnclampLeft、UnclampRight、Unfold、Deliver。详见 `Docs/音画事件.csv`。

默认 `bUsePlaceholderTimer=true`，0.6 秒后模拟动画结束，便于没有素材时联调状态；不自动生成视觉动画。接正式动画时设为 **false**：

1. 接到 Cue、Token，按 Cue 播放 Montage/Sequence/Timeline，保存**本次播放**的 Token。
2. Dye 动画分浸入、提出滴水、氧化显色；Timeline 更新调用 `ReportDyeProgress(Token, Progress)`。
3. 动画真正结束时调用 `CompleteAnimation(Token)`。多段/多对象演出由一个协调器等全部结束后回调，其他视觉对象不要提前回调。
4. 取消、重开时停止旧动画，不能读取一个被新动画覆盖的“最新 Token”来完成旧播放。

序号会拒绝旧回调和重复回调，动画期间拒绝其他状态输入。关闭占位计时后，丢失完成回调会保持锁定，这是有意设计；修复接线或 Restart，不会用任意超时跳过正式演出。

外部蓝图的 Delay/Timer 不受组件直接管理，必须在 ResetVisuals 中取消；其回调仍须携带 Token。外部自定义镜头演出调用 SetCameraBusy(true)，结束后设 false；内置三机位切换自动管理锁定并取消当前拖拽。

结果只有 Left、Right、Depth，均向蓝图只读公开。九张遮罩索引可用 `(Left枚举整数-1)*3+(Right枚举整数-1)`；只有两侧非 None 时读取遮罩。四档颜色索引为 Depth-1；Depth=0 只画纹样位置/轮廓，禁止提前显示完整成品。白区和中央旧样的保留在材质中实现。

## 6. 验收

在启用插件的 Editor 工程里打开 Session Frontend → Automation，运行 `Indigo.Flow.GuardsAndReplay`。源码覆盖两局 A/A 第一档、A/B 第四档；交换、顺序限制、动画重复输入、过期/重复回调、零档收工、四档上限、镜头锁定、回看和重开。

2026-10-17 更新：四组自动化测试全部通过，Editor 与 Game 目标构建成功；实际 PIE 已验证 A/A 一档和 A/B 四档、动画期间输入限制及深度材质复位。详见 `Docs/染色与出生点验证记录.md`。以下为完整项目的人工验收清单，未接入的正式布面、展开和展示仍待后续验收：

- 拖空、拖错区域、Esc/右键取消后复位；架上源保留；A/B 与 B/A 的布面实际交换。
- 拖拽中切镜头或重开，松开鼠标后物品不继续跟随；切出窗口后不误提交。
- UI 覆盖时点击/释放不穿透到场景。
- 染色动画中连点不会跳过或重复加档；第四档等待收工。
- 展开、交付、结束页和回看读取同一个 Result；重开后清除旧视觉和所有过程数据。

参考 API：[Epic APlayerController 文档](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/APlayerController)。交付源码的真实编译结果以本机构建日志为准。

