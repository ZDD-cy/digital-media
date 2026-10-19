# 蓝染交互插件 2.1.0 · UE 5.8

已在本机 UE 5.8.3 编译。源码插件提供第一人称工作台入口、三组固定制作机位、统一点击/拖放、P0/P1 流程、心声字幕、动画与材质接口。当前 LanRan 关卡已接入。源码包不包含整个游戏工程、美术资产或预编译 DLL。

## 本轮修复

修复地形缺失、板面投放框错位与普通模型遮挡；接入已有的 45 张组合/深度贴图，增加游戏开头视频组件。完整资源同步和范围见 [2.1.0 修复与同步](Docs/2.1.0修复与同步.md)，视频接入见 [片头视频接入](Docs/片头视频接入.md)。只安装源码包不会带走关卡或贴图。

## 开始运行

打开 LanRan.uproject，打开 `/Game/Japanese_Village/Levels/L_TestLight`，点击运行。WASD 行走、鼠标转向，靠近工作台按 E；工作台中用鼠标操作，右键取消拖放，E 返回行走。编辑器视口小时按 F11 放大。

顺序：查看旧布与订单 → 选左右花版并确认 → 舀液/浸试布/提出/等待显色 → 添一份谷壳 → 完整复试 → 三折 → 放布/合版/左右夹紧 → 染色与补温 → 收工 → 清洗/沥水/松夹/展开 → 交付 → 阅读委托背面 → 叠布寄件 → 回看或重开。

## 安装和工程接线

1. 将 IndigoInteraction 放在目标工程 Plugins 中，启用插件。关闭编辑器，用 UE 5.8 的 Visual Studio C++ 工具链构建 Development Editor。其他引擎小版本需重新编译。
2. 放一个 IndigoDirector，其 Flow 是唯一结果来源。Cameras 配置 Table、Closeup、Quilt 三个 CameraActor。
3. 需要保留第一人称时，保持自己的角色和 GameMode，放一个 IndigoWorkbenchBridge，设置 Director、WorkbenchCamera 和交互半径。无需把第一人称控制器换成 IndigoPlayerController。
4. 完全固定视角项目也可用 IndigoPlayerController，DefaultPawn=None；其接线见早期接入文档。不要同时启用两套工作台输入。
5. 当前工作台桥自带中文 UI。启用 Flow.EnableTemperatureRules 和 EnableDeliveryStory 使用 P1；默认 false 为旧 P0 工程兼容。当前 LanRan 工作关卡已启用。
6. 将视觉 Actor 加入 Flow.VisualTargets。当前 IndigoCraftPreview 提供动作演示，UsePlaceholderTimer=false，由真实演示动作结束回传原始 Token。

也可使用 BuildPlugin.ps1 调用官方 RunUAT BuildPlugin 单独构建，输出必须指定新目录。不要将其他引擎编译的 DLL 直接复制使用。

## 输入和状态

花版源：IndigoInteractable，Kind=DragSource、ActiveStep=Select、DragGroup=Pattern、Pattern=A/B/C；左右槽为 DropTarget，LeftSlot 对应左右。三个源保留在架上，允许 A/A，确认前可替换、清空和交换。左右非空才允许确认。HitBox 使用 QueryOnly、Visibility Block，可视模型关闭碰撞。

其他拖放的源与目标必须同时匹配 ActiveStep、DragGroup、Action、ActionIndex。Fold 只接受当前 FoldIndex。CraftPreview 自动提供蓝色拖动点、绿色吸附目标，支持三折、放布、合版、左右夹具；按钮可操作同一合法动作。错拖返回本次抓取原位，短暂提示正确区域。切镜头、切步骤、重开和离开工作台会取消拖放。

| 状态 | 输入/完成条件 |
|---|---|
| Receive | InspectCloth、InspectOrder 都查看 |
| Select | 左右都有花版，Confirm |
| Trial | ScoopTest → DipTest → LiftTest → OxidizeTest；冷缸观察后 AddFuel，完整暖缸复试 |
| Fold | Fold，Index 依次 0/1/2，每折动画完成 |
| Clamp | PlaceCloth → ClosePlate → ClampLeft/Right，双侧完成 |
| Dye | Ready 时 Dye；每段结束加一档并回到 Cold；Depth≥1 可 FinishDye |
| Wash | Wash → Drain → UnclampLeft/Right → Unfold |
| Reveal | 可停留，Deliver；回看模式只接受 Back |
| Delivery | 阅读保护默认 8 秒，然后 Ship |
| End | Review 回看，Back 返回；Restart 新一局 |

初始 Cold、Fuel=5。补温开始扣一份燃料，动画结束才 Ready；Ready 不能重复添料。试布和折布不降温，第一份燃料同时支持暖缸复试和第一档染色。最多四档共用四份，至少剩一份。Cold 时禁染但保留已有深度；至少一档可直接收工。动画和镜头过渡中不能重复推进。

## 三机位、字幕和剧情

StepConfig 配置每步字幕和机位；当前 Receive/Select/Dye 为全景，Trial/Fold/Clamp/Wash 为近景，Reveal/Delivery/End 为作品机位。手动切镜期间锁住状态输入，提前不开放作品机位。底部心声最短显示 2.5 秒，只保留最新待显示提示，不叠加多条。

旧布角和订单正面在接件查看；订单背面只在作品展开后点击交付才显示。寄件和结束页读取本局 Result。回看直接往返 Reveal/End，不重播交付剧情。地域短片按用户确认留空，当前寄件后直接进入完成页。

## 动画与材质接口

| 接口 | 用途 |
|---|---|
| ResetVisuals | 停止旧动作并恢复所有开局姿态/材质 |
| SetPattern(Left,Right) | 同步左右纹样，中央旧样由素材固定 |
| SetDyeProgress(CompletedDepth,Progress) | 已完成档位和本段 0..1 进度 |
| ShowFinalQuilt(Result) | 展开、交付、结束和回看使用同一结果 |
| PlayIndigoAnimation(Cue,Token) | 开始动作，真正结束后 CompleteAnimation(Token) |
| OnAnimationRequested(Cue,Token) | 可选集中音画路由；避免与视觉接口双重完成 |

关掉 UsePlaceholderTimer 后不能靠任意超时跳过演出。正式 Montage/Sequence/Timeline 保存本次 Token；染色持续调用 ReportDyeProgress(Token,Progress)，完成才回调。过期、重复 Token 均被拒绝。ResetVisuals 必须取消外部 Delay/Timer/Sequence；外部镜头用 SetCameraBusy 管理输入。

结果只有 Left、Right、Depth。九张布局遮罩可按 `(Left-1)*3+(Right-1)` 取，四档颜色按 `Depth-1` 取，Depth=0 只显示布局轮廓。中央旧样和留白在材质中保留。已有花纹资源不由本轮生成或改绘；当前 LanRan 展开布面已按组合/深度绑定仓库已有贴图，折布/夹版仍使用程序预览几何；其他正式布面资源替换继续使用以上接口。音效资源尚未提供，Cue 挂接表已列出，未伪造正式音效或短片。

## 验证与交付

本轮 Editor / Game 目标构建通过，六项 Indigo 自动化测试通过。实际 LanRan PIE 连续通过 A/A 一档和 A/B 四档，包括冷暖试布、燃料、三折/夹版目标命中、清洗、展开、8 秒阅读、寄件、回看和重开；第一折另用真实鼠标拖放通过。

最新范围与同步方法见 [2.1.0 修复与同步](Docs/2.1.0修复与同步.md)，历史 P1 记录见 [P1 接入与验收](Docs/P1接入与验收.md)。配置表为 Docs/步骤配置.csv，音画表为 Docs/音画事件.csv。历史 1.0–1.4 验证记录保留供追溯，不代表当前仍未实现 P1。
