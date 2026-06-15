# 基于 EasyX 的中国象棋对战程序 — 设计报告

> **版本**：v0.2.0
> **日期**：2026 年 6 月

---

## 目录

- [一、引言](#一引言)
- [二、需求分析](#二需求分析)
- [三、系统设计](#三系统设计)
  - [3.1 总体设计](#31-总体设计)
  - [3.2 模块概要](#32-模块概要)
- [四、程序实现](#四程序实现)
  - [4.1 主要功能实现](#41-主要功能实现)
  - [4.2 数据结构和算法](#42-数据结构和算法)
  - [4.3 程序核心代码](#43-程序核心代码)
- [五、测试](#五测试)
  - [5.1 测试环境](#51-测试环境)
  - [5.2 测试用例](#52-测试用例)
  - [5.3 测试结果](#53-测试结果)
- [六、结论](#六结论)
- [七、参考文献](#七参考文献)
- [八、附录](#八附录)

---

## 一、引言

中国象棋是中国最古老、最具代表性的棋类运动之一，有着广泛的群众基础。随着计算机技术的普及，在计算机上实现中国象棋对战已成为经典的程序设计课题。本程序基于 Windows 平台，使用 C/C++ 语言和 EasyX 图形库，实现了一款具备完整规则判定、图形化交互界面以及可选局域网联机对战功能的中国象棋对战程序。

本程序的设计目标不仅仅是完成一个"能走棋"的玩具程序，而是追求以下三个层次：

1. **规则完备性**：正确实现中国象棋全部棋子的走法规则、蹩马脚/堵象眼/炮翻山等特殊约束、将军与飞将判定、将死检测等核心逻辑。
2. **交互体验**：提供自然直观的鼠标选棋与走棋操作、合法走法提示、悔棋与认输功能、清晰美观的棋盘渲染。
3. **联机对战**：基于 TCP 协议实现局域网双人对战，支持走棋同步、悔棋请求/同意/拒绝协议、以及断线处理。

通过本程序的开发，可以综合运用数据结构（棋盘建模、走法历史栈）、算法（穷举合法走法、将军判定）、图形编程（EasyX 双缓冲渲染、鼠标事件处理）以及网络编程（Winsock2 套接字通信、自定义应用层协议）等多方面知识，具有较强的学习与实践意义。

---

## 二、需求分析

### 2.1 功能需求

#### 2.1.1 单机双人对弈

- 棋盘初始化：按中国象棋标准布局摆放红黑双方共 32 枚棋子。
- 走棋操作：玩家用鼠标点击己方棋子选中，再次点击目标位置完成走棋；点击同色棋子切换选中，点击空白或非法位置取消选中。
- 规则判定：严格按照中国象棋规则校验每一步走法是否合法，
- 自将防御：任何走法执行后若导致己方被将军，则该走法被判定为非法。
- 将军提示：被将军方九宫中心显示"将军"文字。
- 合法走法提示：选中棋子后，在棋盘上以绿色圆点（可走空位）和红色圆圈（可吃子位）高亮显示该棋子的所有合法走法。
- 将死判定：当某方所有棋子均无合法走法时，判定该方输棋，对方获胜。
- 悔棋：单机模式下可撤销上一步走法，恢复棋盘状态。
- 认输：红方或黑方可主动认输，对方获胜。

#### 2.1.2 联机双人对弈

- 角色分配：主机执红（先手），客机执黑（后手）。
- 连接建立：主机在指定端口监听，等待客机连接；客机输入主机 IP 地址发起连接。
- 走棋同步：本地走棋成功后，自动将走法数据通过网络发送给对方，对方自动执行相同走法。
- 回合锁定：仅当前回合方的玩家可以操作棋盘，另一方的鼠标点击被忽略（UI 按钮除外）。
- 悔棋协议：
  - 任意一方点击悔棋按钮后，向对方发送悔棋请求。
  - 对方同意 → 双方各回退相应步数，回到请求方回合。
  - 对方拒绝 → 撤销请求，游戏继续。
  - 在等待对方响应期间，本地棋盘操作被锁定。
- 认输同步：一方认输后，自动通知对方游戏结束。
- 断线处理：检测到对方断开连接时，本方自动获胜。

### 2.2 性能需求

- **渲染性能**：采用双缓冲机制，避免画面闪烁，确保流畅体验。
- **网络延迟**：局域网环境下，走棋数据包 < 40 字节，延迟应在 10ms 以内。
- **内存占用**：棋盘状态仅为一个 10×9 的二维数组，历史记录最多存储一局棋谱，内存占用应 < 10MB。

---

## 三、系统设计

### 3.1 总体设计

#### 3.1.1 程序组成框图

本程序采用模块化分层架构，按职责分为四层。

> 📊 框图见 `pic/3-1-1-architecture.md`

各模块职责一览：

- **主入口**（`main.cpp`）：仅含 `main()` 函数，负责菜单 → 联机连接 → 主循环（鼠标/网络/渲染/退出） → 结算。流程细节委托给 `game_manager`。
- **菜单**（`menu.h/cpp`）：开始界面渲染与交互（主菜单 + 联机子菜单），阻塞等待用户选择，返回 `MENU_SINGLE/HOST/CLIENT`。
- **鼠标输入**（`mouse_input.h/cpp`）：像素坐标→逻辑坐标转换，`SELECT_FIRST` / `SELECT_SECOND` 两阶段状态机。
- **棋盘渲染**（`draw_board.h/cpp`）：棋盘、棋子、选中高亮、合法走法标记、将军提示、右侧 UI 面板、悔棋弹窗。
- **游戏流程管理**（`game_manager.h/cpp`）：回合归属判断、按钮点击处理、网络消息分发、网络连接启动、游戏结算。
- **核心引擎**（`chess_engine.h/cpp`）：全部规则逻辑：走棋校验（7 类棋子 + 自将防御）、悔棋、将军判定（含飞将）、合法走法枚举、将死检测。
- **基础定义**（`chess_def.h`）：`Piece` / `Color` / `Phase` / `Move` 枚举与结构体、坐标转换函数、UI 布局常量。
- **游戏状态**（`game_state.h`）：`ChessPiece` 结构体（id + color）、`GameState` 结构体（棋盘 + 当前方 + 阶段 + 历史 + 悔棋等待标志）。
- **网络通信**（`network.h/cpp`）：Winsock2 TCP 直连、自定义应用层协议、断连检测（`connected` 标记）。

#### 3.1.2 程序主流程图

单机模式与联机模式流程差异较大，分开绘制：

- 单机模式流程图见 `pic/3-1-2a-single.md`
- 联机模式流程图见 `pic/3-1-2b-network.md`

#### 3.1.3 联机通信架构

双机通过 TCP 直连，使用自定义应用层协议进行同步。

消息格式为定长包头（8 字节）+ 变长负载（最大 256 字节）。协议共定义 6 种消息类型：`NET_MOVE`（走棋）、`NET_UNDO_REQ/ACCEPT/REJECT`（悔棋协议）、`NET_RESIGN`（认输）、`NET_GAME_OVER`（游戏结束）。

断连检测：`recv()` 返回 0 或异常错误时，`net_recv_msg()` 将 `g_net.connected` 置为 `false`。主循环中 `handle_network_msg()` 检测到 `!connected` 后直接判本方获胜。

> 📊 通信拓扑、消息格式与断连检测流程图见 `pic/3-1-3-network.md`

---

### 3.2 模块概要

本程序由 9 个模块组成，按职责分工如下：

`chess_def.h` 提供全项目共享的枚举（Piece / Color / Phase）、`Move` 走法结构体、坐标转换 inline 函数以及 UI 布局常量。`game_state.h` 定义 `ChessPiece` 和 `GameState` 两个核心数据结构——棋盘用 `ChessPiece board[10][9]` 二维数组表示，`GameState` 同时持有当前方、选中状态、走法历史栈和联机悔棋等待标志。

`chess_engine` 是规则核心层，封装了全部象棋逻辑：`init_game()` 按标准布局初始化 32 枚棋子；`apply_move()` 校验并执行走棋（规则检查 → 模拟执行 → 自将防御 → 正式落子）；`undo_move()` 和 `undo_to_turn()` 提供悔棋能力；`is_attacked()` 和 `is_in_check()` 负责将军与飞将判定；`get_legal_moves()` 枚举某棋子的全部合法走法；`check_over()` 通过遍历当前方所有棋子的合法走法来判断将死。

`menu` 负责开始界面：主菜单（单机/联机）和联机子菜单（创建主机/连接主机），通过 `InputBox` 获取客机目标 IP，阻塞等待用户选择后返回会话模式。

`mouse_input` 实现 `SELECT_FIRST → SELECT_SECOND` 两阶段状态机，完成像素坐标到棋盘逻辑坐标的转换，并在第二阶段调用 `apply_move()` 验证走法合法性。

`draw_board` 采用双缓冲渲染，按固定顺序绘制棋盘格线、棋子（含选中高亮）、合法走法标记（绿点/红圈）、将军提示文字、右侧 UI 面板和联机悔棋请求弹窗。UI 面板根据上层传入的 `is_network` / `is_host` 参数决定布局，不直接访问网络状态。

`game_manager` 接管了原 `main.cpp` 中全部辅助函数，负责：回合归属判断、认输/悔棋按钮响应、网络消息分发（`handle_network_msg`）、网络连接启动（`net_startup`）和游戏结算（`game_over_sequence`）。其中 `handle_network_msg` 在接收失败时检测 `connected` 标记实现断连判胜。

`network` 基于 Winsock2 实现 TCP 直连（端口 8888），使用定长包头 + 变长负载的自定义协议，定义了 6 种消息类型（MOVE / UNDO_REQ / UNDO_ACCEPT / UNDO_REJECT / RESIGN / GAME_OVER）。`NetState` 通过 `const` 引用暴露，封装了 `waiting_undo` 的 setter，`connected` 仅由 `net_recv_msg()` 在检测到 FIN 或 RST 时写入。

`main.cpp` 仅含 `main()` 函数（约 55 行），负责顶层调度：菜单选择 → 联机连接 → 图形初始化 → 主循环（鼠标/网络/渲染/退出）→ 结算。循环内的流程决策均委托给 `game_manager`。

---

## 四、程序实现

### 4.1 主要功能实现

#### 4.1.1 `chess_def.h` — 基础定义

本模块不包含函数，仅提供全项目共享的枚举、结构体、坐标转换函数和 UI 布局常量。

- `enum Piece`：棋子类型，`SPACE=-1`，黑方 0-6（車馬象士将砲卒），红方 7-13（俥马相仕帥炮兵）。红黑双方索引偏移 7，可通过 `id / 7` 判断颜色。
- `enum Color`：`_NONE=-1, _RED, _BLACK`（前缀 `_` 避免与 EasyX 宏 `RED`/`BLACK` 重定义冲突）。
- `enum Phase`：`SELECT_FIRST`（等待选棋）、`SELECT_SECOND`（等待走棋）。
- `struct Move`：走法记录，包含 `from_x/y`（起点）、`to_x/y`（终点）、`moved_id/color`（移动棋子）、`captured_id/color`（被吃棋子）。
- `toX(col)` / `toY(row)`：`inline` 函数，将棋盘逻辑坐标转换为像素坐标。公式：`BOARD_LEFT + col × CELL_SIZE`。
- UI 常量：`BOARD_LEFT=60`、`BOARD_TOP=40`、`CELL_SIZE=60`、`UI_BTN_X=610`、`UI_UNDO_Y=290` 等。

#### 4.1.2 `game_state.h` — 游戏状态

`ChessPiece` 结构体包含：
- `int id`（Piece 枚举值）
- `int color`（Color 枚举值）

`GameState` 结构体包含：
- `ChessPiece board[10][9]` —— 棋盘，10行(row) × 9列(col)
- `int current_side` —— 当前回合方（`_RED` / `_BLACK`）
- `int turn_count` —— 回合计数
- `int phase` —— 当前阶段（`SELECT_FIRST` / `SELECT_SECOND`）
- `int selected_x, selected_y` —— 当前选中棋子坐标，`-1` 表示未选中
- `bool game_over` —— 游戏是否结束
- `int winner` —— 胜方颜色（`-1` 表示未结束）
- `vector<Move> history` —— 走法历史栈（悔棋用，每次走棋 `push_back`）
- `bool undo_req_pending` —— 联机：对方请求悔棋，等待本地回应

说明：`undo_req_pending` 原本是全局变量 `g_undo_req_pending`，优化后移入 `GameState`，归属更清晰。

#### 4.1.3 `chess_engine.h/cpp` — 象棋引擎（核心模块）

引擎提供全部游戏规则逻辑，是调用关系最深、被依赖最多的模块。函数调用关系图见 `pic/3-2-3-engine-calls.md`。

- `init_game(state)` — 初始化棋盘 32 枚棋子与游戏状态（当前方=红，phase=`SELECT_FIRST` 等）。参数：`GameState &state`。返回：`void`。
- `apply_move(state, fx, fy, tx, ty)` — 校验并执行走棋。7 类棋子规则校验 → 模拟执行 → `is_in_check()` 自将检查 → 更新棋盘 → `history.push_back()`。参数：`state`, 起点坐标 `(fx,fy)`, 终点坐标 `(tx,ty)`。返回：`bool`。内部调用 `is_in_check()`。
- `undo_move(state)` — 撤销上一步走棋。`history.pop_back()`，恢复起/终点棋子，回退回合。参数：`GameState &state`。返回：`bool`（history 为空时 false）。
- `undo_to_turn(state, target_color)` — 联机悔棋专用：回退到指定颜色方的回合。若当前回合已是 `target` 则退 2 步，否则退 1 步。参数：`state, target_color`。返回：`void`。内部调用 `undo_move()`。
- `is_attacked(state, x, y, attacker_color)` — 遍历 `attacker_color` 方所有棋子，判断是否有棋子能攻击到点 `(x,y)`。覆盖車/馬/砲/象/士/将/卒全部走法，不含飞将。参数：`state`, 目标坐标, 攻击方颜色。返回：`bool`。
- `is_in_check(state, color)` — 判断 `color` 方是否正被将军。先找己方将/帥位置，调 `is_attacked()` 检测普通攻击，再检查对面将是否同列且无遮挡（飞将）。参数：`state`, 被检查方颜色。返回：`bool`。内部调用 `is_attacked()`。
- `get_legal_moves(state, fx, fy)` — 枚举 `(fx,fy)` 处棋子的所有合法走法。对 90 个目标位置逐一遍历，每个位置调 `apply_move()` 试探，成功则记录并立即 `undo_move()` 恢复。参数：`state`, 棋子坐标。返回：`vector<Move>`。内部调用 `apply_move()` → `undo_move()`。
- `check_over(state)` — 将死判定：遍历当前回合方所有棋子，若全部 `get_legal_moves()` 为空，则当前方被将死，判对方获胜。参数：`GameState &state`。返回：`int`（胜方颜色，-1 未结束）。内部调用 `get_legal_moves()`。

**关键算法要点：**

1. **走棋校验（`apply_move`）**：采用"规则校验 → 模拟执行 → 自将检查 → 正式提交"四步流程。模拟执行时暂存起终点旧值，若自将则恢复后返回 `false`，否则覆盖写入。将/帥分支中额外模拟移动来检查对将。

2. **合法走法枚举（`get_legal_moves`）**：`apply_move()` 内部已完成自将检查，外层不能再次调用 `is_in_check()`，否则会产生双重判定导致合法走法被错误过滤（开发中曾因此出现走法提示不显示的 bug）。

3. **将死判定（`check_over`）**：被将军时所有不符合规则的走法均被 `apply_move()` 否定，因此"所有棋子无合法走法"即等价于"被将死"。

#### 4.1.4 `menu.h/cpp` — 开始菜单

- `show_start_menu()` — 主菜单（单机/联机）→ 联机子菜单（创建主机/连接主机），阻塞等待用户点击，连接主机时弹出 `InputBox` 获取 IP。参数：无。返回：`int`，取值为 `MENU_SINGLE(1)` / `MENU_HOST(2)` / `MENU_CLIENT(3)`。
- `get_connect_ip()` — 返回用户在 `InputBox` 中输入的目标 IP 地址字符串。返回：`const char*`。
- `draw_btn()` — 绘制一个矩形按钮。参数：`x, y, w, h, text, color`。返回：`void`。
- `in_btn()` — 判断鼠标坐标是否在按钮区域内。参数：`mx, my, bx, by, bw, bh`。返回：`bool`。
- `wait_click()` — 阻塞等待鼠标左键点击或窗口关闭。参数：`int &mx, &my`（输出）。返回：`void`。

说明：菜单模块于项目优化期间从 `draw_board.cpp` 中独立出来，遵循单一职责原则。

#### 4.1.5 `mouse_input.h/cpp` — 鼠标输入处理

- `handle_mouse_click(state, pX, pY, out_move)` — 两阶段状态机处理鼠标点击。参数：`state`, 像素坐标 `(pX,pY)`, `Move &out_move`（输出）。返回：`bool`（走棋成功时为 true）。

**状态机转换：**

在 `SELECT_FIRST`（选棋）阶段：点击己方棋子则选中，进入 `SELECT_SECOND`；点击空处或对方棋子则忽略。

在 `SELECT_SECOND`（走棋）阶段：点击同一棋子则取消选中，回到 `SELECT_FIRST`；点击己方其他棋子则切换选中，保持 `SELECT_SECOND`；点击目标位置则调用 `apply_move()` 校验——若成功，输出 `Move` 并回到 `SELECT_FIRST`；若失败，取消选中，回到 `SELECT_FIRST`。

坐标转换采用四舍五入：`col = (pX - BOARD_LEFT + CELL_SIZE/2) / CELL_SIZE`。

#### 4.1.6 `draw_board.h/cpp` — 棋盘渲染

- `draw_board(state, is_network, is_host)` — 完整渲染棋盘、棋子、选中高亮、合法走法标记、将军文字、右侧 UI 面板、悔棋弹窗。参数：`state`, 是否联机, 是否主机。返回：`void`。
- `show_result(state)` — 显示白底结算文字框（红胜/黑胜）。参数：`state`。返回：`void`。

**渲染顺序：** 背景 → 将军提示 → 棋盘外框（2px 粗线）→ 横线/竖线（楚河汉界处断开）→ 九宫斜线 → 楚河汉界文字 → 棋子（含选中高亮）→ 合法走法标记（绿色圆点为可走空位，红色圆圈为可吃子位）→ 右侧面板 → 悔棋请求弹窗（联机时）。

说明：右侧面板根据 `is_network` / `is_host` 参数（由 `main()` 传入）决定按钮布局，不直接读取网络状态，解除了 UI 层对网络层的依赖。

#### 4.1.7 `game_manager.h/cpp` — 游戏流程管理

本模块接管了优化前 `main.cpp` 中全部辅助函数，以及网络消息分发和游戏结算流程。`main.cpp` 精简为仅含 `main()` 函数。

- `is_my_turn(menu_choice, current_side)` — 判断当前回合是否轮到本地操作。单机总是返回 `true`；联机主机=红方、客机=黑方。参数：`menu_choice, current_side`。返回：`bool`。
- `handle_control_click(state, px, py, is_network, is_host)` — 处理右侧 UI 按钮点击。单机模式下：黑认输（判红胜）、悔棋（`undo_move`）、红认输（判黑胜）。联机模式下：己方认输则调用 `net_send_resign()` 并判负；悔棋则调用 `net_send_undo_req()` 并用 `net_set_waiting_undo(true)` 锁定本地操作；悔棋弹窗同意/拒绝则调用 `undo_to_turn()` 并发送对应消息。参数：`state`, 像素坐标, `is_network`, `is_host`。返回：`bool`（已处理时为 true）。
- `handle_network_msg(state, menu_choice)` — 非阻塞接收一条网络消息并分发处理，涵盖 `NET_MOVE` / `NET_UNDO_REQ` / `NET_UNDO_ACCEPT` / `NET_UNDO_REJECT` / `NET_RESIGN` 消息类型。接收失败时检查 `net_get_state().connected`：若为 `false`，则直接判本方获胜（对方断连）。参数：`state, menu_choice`。返回：`bool`。
- `net_startup(menu_choice)` — 启动网络连接。主机路径：调用 `net_host(8888)` 并弹出临时窗口显示本机 IP；客机路径：弹窗显示"正在连接主机..."，调用 `net_connect(ip, 8888)`。失败返回 `false`。参数：`menu_choice`。返回：`bool`。
- `game_over_sequence(state, is_network, is_host)` — 游戏结算流程。联机则先发 `net_send_game_over()` → 渲染最后一帧 → `Sleep(500)` → `show_result()` → 等待按键 → 联机则 `net_cleanup()` → `closegraph()`。参数：`state, is_network, is_host`。返回：`void`。

**悔棋协议流程：**

```
A 点击悔棋 → net_send_undo_req() → net_set_waiting_undo(true) → 本地棋盘操作锁定
B 收到 NET_UNDO_REQ → state.undo_req_pending = true → 显示"同意/拒绝"弹窗
B 点击同意 → net_send_undo_accept() → undo_to_turn(state, opp_color)
B 点击拒绝 → net_send_undo_reject()
A 收到回复 → 同意则执行 undo_to_turn() 回退 → 解除锁定；拒绝则仅解除锁定
```

#### 4.1.8 `network.h/cpp` — 网络通信

基于 Winsock2 实现 TCP 直连，使用自定义应用层协议。

- `net_init()` — 调用 `WSAStartup(2,2)` 初始化 Winsock。参数：无。返回：`bool`。
- `net_cleanup()` — 调用 `net_disconnect()` + `WSACleanup()`。参数：无。返回：`void`。
- `net_host(port)` — 创建 TCP 服务器。执行 `socket` → `SO_REUSEADDR` → `bind` → `listen` → `select`+`accept` 循环等待。完成后关闭监听 socket，启用 `TCP_NODELAY`。参数：`port`（默认 8888）。返回：`bool`。
- `net_connect(ip, port)` — 客户端连接。执行 `socket` → `connect`（受 `SO_SNDTIMEO` 控制），成功后启用 `TCP_NODELAY`。参数：`ip, port`。返回：`bool`。
- `net_disconnect()` — 关闭所有 socket，重置 `g_net` 状态。参数：无。返回：`void`。
- `net_send_msg(msg)` — 发送消息，先发 8B 包头（type + data_len），再发负载。循环 `send()` 处理部分发送。参数：`const NetMessage &msg`。返回：`bool`。
- `net_recv_msg(msg)` — 阻塞接收。循环 `recv()` 接收包头（超时 100ms）→ 校验合法性 → 接收负载。`recv()` 失败（含对端断开）时将 `g_net.connected` 置为 `false`。参数：`NetMessage &msg`（输出）。返回：`bool`。
- `net_recv_msg_nonblock(msg)` — `select()` 检查可读后调 `net_recv_msg()`，实现非阻塞。参数：`NetMessage &msg`（输出）。返回：`bool`。
- `net_send_move(move)` 等 6 个便捷函数 — 封装特定消息类型的发送，内部构造 `NetMessage` 后调 `net_send_msg()`。参数：对应数据。返回：`bool`。
- `net_pack_move()` / `net_unpack_move()` — `Move` 结构体与 `char[256]` 直接按字节互拷。参数：`msg, move`。返回：`void`。
- `net_get_state()` — 获取网络状态，返回只读引用 `const NetState&`，防止外部直接修改。参数：无。返回：`const NetState&`。
- `net_set_waiting_undo(bool)` — 设置悔棋等待状态。为 `waiting_undo` 字段的唯一写入口，替代原先外部直接修改 `g_net.waiting_undo` 的方式。参数：`bool`。返回：`void`。
- `net_get_local_ip()` — 通过 `gethostname` + `gethostbyname` 获取本机 IPv4 字符串。参数：无。返回：`const char*`。

**NetState 的封装改进：** `net_get_state()` 返回 `const NetState&`，外部不可直接修改 `role`、`connected`、`waiting_undo` 字段。`waiting_undo` 的写入通过 `net_set_waiting_undo()` 进行，`connected` 的修改仅由 `net_recv_msg()` 检测断连时触发。

**断连检测机制：** `net_recv_msg()` 中 `recv()` 返回 0（FIN，对方正常关闭）或 `SOCKET_ERROR`（RST/异常断连）时，均将 `g_net.connected` 置为 `false`。上层 `handle_network_msg()` 检测到 `!connected` 后直接判本方获胜。

**套接字优化：** `SO_REUSEADDR`（防止崩溃重启后 `bind` 失败）、`TCP_NODELAY`（禁用 Nagle 算法降低延迟）、`SO_RCVTIMEO`（100ms 超时避免永久阻塞）、`SO_SNDTIMEO`（发送超时）。

#### 4.1.9 `main.cpp` — 程序入口

仅含 `main()` 函数（约 55 行），流程如下：

1. `show_start_menu()` 获取用户选择。
2. 若为联机模式，调用 `net_startup()` 建立连接，失败则退出。
3. `initgraph(800, 640)` 初始化图形窗口，`init_game(state)` 初始化游戏。
4. 进入主循环：`MouseHit()` → UI/棋盘分发 → 联机时调 `handle_network_msg()` → 窗口关闭检测 → `BeginBatchDraw()`/`draw_board()`/`EndBatchDraw()` 双缓冲渲染。
5. 退出循环后调用 `game_over_sequence()` 结算。

说明：主循环中所有流程决策函数（`is_my_turn`、`handle_control_click`、`handle_network_msg`、`net_startup`、`game_over_sequence`）均已迁入 `game_manager` 模块，`main()` 仅负责顶层调度。

### 4.2 数据结构和算法

#### 4.2.1 棋盘数据布局

棋盘使用 `ChessPiece board[10][9]` 二维数组表示，`board[row][col]` 对应棋盘上 row 行 col 列的格子。row 0 为黑方底线，row 9 为红方底线，col 0~8 从左到右。初始布局如下（`-` 表示 `SPACE` 空位）：

```
col:  0    1    2    3    4    5    6    7    8
row0: 車   馬   象   士   将   士   象   馬   車      ← 黑方底线
row1: -    -    -    -    -    -    -    -    -
row2: -    砲   -    -    -    -    -    砲   -      ← 黑炮
row3: 卒   -    卒   -    卒   -    卒   -    卒     ← 黑卒
row4: -    -    -    -    -    -    -    -    -      ← 楚河
row5: -    -    -    -    -    -    -    -    -      ← 汉界
row6: 兵   -    兵   -    兵   -    兵   -    兵     ← 红兵
row7: -    炮   -    -    -    -    -    炮   -      ← 红炮
row8: -    -    -    -    -    -    -    -    -
row9: 俥   马   相   仕   帥   仕   相   马   俥      ← 红方底线
```

每个 `ChessPiece` 仅存 `id`（棋子类型）和 `color`（颜色），不存坐标——坐标由数组下标隐式表达。

#### 4.2.2 走法记录与历史栈

每一步走棋用 `Move` 结构体记录完整信息：

```
Move {
    from_x, from_y   // 起点坐标，如 (0, 9) 表示红車原位
    to_x, to_y       // 终点坐标，如 (0, 5)
    moved_id         // 移动棋子类型，如 俥
    moved_color      // 移动棋子颜色，如 _RED
    captured_id      // 被吃棋子类型，如 SPACE（无吃子）
    captured_color   // 被吃棋子颜色，如 _NONE
}
```

所有走法按时间顺序存储在 `vector<Move> history` 中。每次走棋调用 `history.push_back(move)`，悔棋调用 `history.pop_back()` 弹出最后一步，然后从 `Move` 中读取 `moved_id/color` 和 `captured_id/color` 恢复起终点棋子。

**示例——红方开局走炮二平五（炮从 (7,7) 走到 (4,7)），然后悔棋：**

| 步骤 | 操作 | history 状态 |
|------|------|-------------|
| 走棋前 | — | `[]`（空） |
| 走棋 | `apply_move(state, 7, 7, 4, 7)` → `history.push_back(move)` | `[{from=(7,7), to=(4,7), moved=炮_RED, captured=SPACE}]` |
| 悔棋 | `undo_move(state)` → `last = history.back()` → `history.pop_back()` | `[]` |
| 恢复 | `board[7][7] = {炮, _RED}`, `board[4][7] = {SPACE, _NONE}` | 棋盘恢复走棋前状态 |

#### 4.2.3 合法走法枚举算法

`get_legal_moves(state, fx, fy)` 采用**暴力试探法**，对 10×9=90 个棋盘位置逐一遍历，每个位置作为目标调用 `apply_move()` 试探。若返回 `true` 则说明该走法合法，将其加入结果列表后立即 `undo_move()` 恢复棋盘，继续试探下一个位置。

```
算法流程（伪代码）：

输入：棋子坐标 (fx, fy)
输出：该棋子所有合法走法的列表

1. moves ← 空列表
2. for ty = 0 to 9:
3.     for tx = 0 to 8:
4.         if apply_move(state, fx, fy, tx, ty) == true:
5.             Move m = {fx, fy, tx, ty, ...}   // 构造走法
6.             moves.push_back(m)                 // 记入结果
7.             undo_move(state)                   // 恢复棋盘
8. return moves
```

以红方开局炮在 (7,7) 为例，`get_legal_moves(state, 7, 7)` 会遍历 90 个可能的目标点。其中绝大部分因"炮必须走直线"、"炮中间必须无子（移动）/恰有 1 子（吃子）"等约束被 `apply_move()` 否定，最终约返回 12 个合法的走法位置（纵线 col=7 上空位 + 横线 row=7 上空位等），由 `draw_board` 渲染为绿色圆点提示。

#### 4.2.4 将军判定（双重检查）

`is_in_check(state, color)` 执行两重检查：

1. **普通攻击检测**：找到 `color` 方将/帥所在位置 `(kx, ky)`，调用 `is_attacked(state, kx, ky, oppColor)` 遍历对方所有棋子的攻击范围，判断是否有棋子能直接吃掉己方将。

2. **飞将检测**：找到双方将/帥的位置，若二者同列且纵坐标之间无任何棋子阻挡，则当前局面为将帅对面（飞将），也视为被将军。

这种设计将"普通攻击"和"飞将"解耦：`is_attacked()` 专注处理 7 类棋子各自独立攻击判定，`is_in_check()` 额外追加飞将这一特殊规则。在走棋合法性校验中，`apply_move()` 的将/帥分支还会模拟移动后检查是否造成对将。

#### 4.2.5 联机悔棋状态机

联机悔棋涉及两个线程（用户操作 + 网络消息）对两个状态标志的协作，是程序中最复杂的交互逻辑：

| 标志 | 位置 | 含义 |
|------|------|------|
| `waiting_undo` | `NetState`（网络层） | 本地已发送悔棋请求，等待对方响应 |
| `undo_req_pending` | `GameState`（游戏层） | 本地收到对方的悔棋请求，等待本地点击同意/拒绝 |

以 A（主机）主动发起悔棋为例：

```
时间轴 →

A 方（主机）                       B 方（客机）
────────────────────────────────────────────────
点击悔棋按钮
  handle_control_click()
  → net_send_undo_req()
  → net_set_waiting_undo(true)
  → 棋盘操作被 is_my_turn() 锁定

                                  recv() 收到 NET_UNDO_REQ
                                    handle_network_msg()
                                    → state.undo_req_pending = true
                                    → draw_board() 绘制同意/拒绝弹窗
                                    → 点击同意
                                      handle_control_click()
                                      → net_send_undo_accept()
                                      → undo_to_turn(state, _RED)
                                        退 2 步回到 A 回合
                                      → state.undo_req_pending = false

recv() 收到 NET_UNDO_ACCEPT
  handle_network_msg()
  → undo_to_turn(state, _RED)
    退 2 步
  → net_set_waiting_undo(false)
  → state.undo_req_pending = false
  → 棋盘操作解锁
```

两个标志保证了双方状态在任何时刻一致：A 等待时不能走棋，B 弹窗未关闭时不能走棋，同意/拒绝后双方同步回到悔棋前 A 的回合。

### 4.3 程序核心代码

以下选取 `chess_engine.cpp` 中三个最具代表性的函数，展示核心实现逻辑。

#### 4.3.1 棋盘初始化 —— `init_game()`

该函数按中国象棋标准布局摆放红黑双方共 32 枚棋子，并初始化所有游戏状态变量。

```cpp
void init_game(GameState &state)
{
    // 清空棋盘
    for (int r = 0; r < ROW; r++)
        for (int c = 0; c < COL; c++)
            state.board[r][c] = {SPACE, _NONE};

    // 黑方底线: row 0
    int black_back[] = {車, 馬, 象, 士, 将, 士, 象, 馬, 車};
    for (int c = 0; c < COL; c++)
        state.board[0][c] = {black_back[c], _BLACK};
    // 黑炮: row 2, col 1 & 7
    state.board[2][1] = {砲, _BLACK};
    state.board[2][7] = {砲, _BLACK};
    // 黑卒: row 3, col 0,2,4,6,8
    int black_pawns[] = {0, 2, 4, 6, 8};
    for (int i = 0; i < 5; i++)
        state.board[3][black_pawns[i]] = {卒, _BLACK};

    // 红方底线: row 9（镜像布局）
    int red_back[] = {俥, 马, 相, 仕, 帥, 仕, 相, 马, 俥};
    for (int c = 0; c < COL; c++)
        state.board[9][c] = {red_back[c], _RED};
    // 红炮: row 7, col 1 & 7
    state.board[7][1] = {炮, _RED};
    state.board[7][7] = {炮, _RED};
    // 红兵: row 6, col 0,2,4,6,8
    int red_pawns[] = {0, 2, 4, 6, 8};
    for (int i = 0; i < 5; i++)
        state.board[6][red_pawns[i]] = {兵, _RED};

    // 初始化状态
    state.current_side = _RED;
    state.turn_count  = 1;
    state.phase       = SELECT_FIRST;
    state.selected_x  = -1;
    state.selected_y  = -1;
    state.game_over   = false;
    state.winner      = -1;
    state.history.clear();
    state.undo_req_pending = false;
}
```

**说明**：红黑双方采用镜像对称布局——row 0 为黑方底线，row 9 为红方底线。棋子类型使用 `Piece` 枚举（黑方 0-6，红方 7-13），双方索引偏移 7，可通过 `id / 7` 判断颜色。`SELECT_FIRST` 表示游戏从红方先手选棋开始。

#### 4.3.2 走棋校验 —— `apply_move()` 自将检查片段

`apply_move()` 是引擎中最核心的函数，包含 7 类棋子的规则校验和自将防御。以下截取规则校验之后、正式走棋之前的**自将检查**关键片段：

```cpp

    // 上文： 7 种棋子走棋规则

    // 需保证走后不能自己被将, 模拟走棋进行检查
    ChessPiece saved_piece  = piece;   // 暂存移动棋子
    ChessPiece saved_target = target;  // 暂存目标棋子
    target = piece;                    // 模拟落子
    piece  = {SPACE, _NONE};           // 起点置空

    bool in_check = is_in_check(state, saved_piece.color);
    // 必须立刻还原, 否则 return 后棋盘状态已被破坏
    piece  = saved_piece;
    target = saved_target;

    if (in_check)
        return false; // 走后自将 → 非法

    // 正式执行走棋并记录
    Move move;
    move.from_x  = fX;   move.from_y  = fY;
    move.to_x    = tX;   move.to_y    = tY;
    move.moved_id    = piece.id;    move.moved_color    = piece.color;
    move.captured_id = target.id;   move.captured_color = target.color;

    target = piece;                  // 正式落子
    piece  = {SPACE, _NONE};         // 清空起点
    state.history.push_back(move);   // 记录到历史栈

    state.current_side = (state.current_side == _RED) ? _BLACK : _RED;
    state.turn_count++;
    return true;
```

**关键设计要点**：

1. **暂存-模拟-还原-判定的四次操作**必须在 `return` 之前完成还原。代码中特别用注释标注了还原顺序的重要性——若先 `return false` 再还原，棋盘将永远停留在模拟状态，导致后续所有操作基于错误数据。

2. `Move` 结构体记录了移动棋子与被吃棋子的完整信息（包括类型和颜色），这确保了悔棋时能够精确恢复起终点棋子，无需额外查询。

3. `is_in_check()` 内部调用了 `is_attacked()`（遍历对方棋子攻击范围）+ 飞将检查（双方将是否同列且无阻挡），两层检查一次性完成。

#### 4.3.3 悔棋实现 —— `undo_move()` 与历史栈

悔棋通过 `vector<Move> history` 栈实现，核心逻辑简洁：

```cpp
bool undo_move(GameState &state)
{
    if (state.history.empty())
        return false;      // 无棋可悔

    Move last = state.history.back();   // 取最后一步
    state.history.pop_back();           // 出栈

    // 恢复起点棋子：放回被移动的棋子
    state.board[last.from_y][last.from_x] = {last.moved_id, last.moved_color};
    // 恢复终点棋子：放回被吃掉的棋子（或 SPACE）
    state.board[last.to_y][last.to_x]   = {last.captured_id, last.captured_color};

    // 回退回合
    state.current_side = (state.current_side == _RED) ? _BLACK : _RED;
    state.turn_count--;
    return true;
}
```

以及联机悔棋专用的 `undo_to_turn()`：

```cpp
void undo_to_turn(GameState &state, int target_color)
{
    // 当前回合已经是 target_color → 已走 2 步, 退 2 步
    // 否则 → 只走了 1 步, 退 1 步
    if (state.current_side != target_color)
        undo_move(state);   // 退 1 步
    else {
        undo_move(state);
        undo_move(state);   // 退 2 步
    }
}
```

**设计要点**：`Move` 结构体中 `moved_id/color` 和 `captured_id/color` 两组字段使得悔棋操作无需遍历棋盘查找对应棋子——直接从栈顶 `Move` 中读取即可完成还原。`undo_to_turn()` 消除了原先在 `main.cpp` 和 `game_control.cpp` 中两处重复的回退逻辑。

---

## 五、测试

### 5.1 测试环境

*描述测试程序时使用的环境和工具。*

<!-- TODO: 填充内容 -->

### 5.2 测试用例

*列出测试程序时使用的测试用例。*

<!-- TODO: 填充内容 -->

### 5.3 测试结果

*展示测试结果，包括成功和失败的测试用例。*

<!-- TODO: 填充内容 -->

---

## 六、结论

*总结程序的优点、存在的问题以及可能的改进方向。*

<!-- TODO: 填充内容 -->

---

## 七、参考文献

*列出编写报告时参考的书籍、文章等资料。*

<!-- TODO: 填充内容 -->

---

## 八、附录

### 8.1 程序源代码

*附上核心的程序源代码。*

<!-- TODO: 填充内容 -->

### 8.2 测试数据

*附上测试程序时使用的数据。*

<!-- TODO: 填充内容 -->
<!-- / -->
