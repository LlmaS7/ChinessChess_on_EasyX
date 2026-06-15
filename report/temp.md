4.2.5 联机悔棋状态机

联机悔棋涉及两个线程（用户操作 + 网络消息）对两个状态标志的协作，是程序中最复杂的交互逻辑：

- waiting_undo（位于 NetState，网络层）：本地已发送悔棋请求，等待对方响应。
- undo_req_pending（位于 GameState，游戏层）：本地收到对方的悔棋请求，等待本地点击同意/拒绝。

以 A（主机）主动发起悔棋为例：

A 方（主机）：
  点击悔棋按钮
  → handle_control_click()
  → net_send_undo_req() —— 向 B 发送悔棋请求
  → net_set_waiting_undo(true) —— 本地进入等待状态
  → 棋盘操作被 is_my_turn() 锁定，A 无法走棋

B 方（客机）：
  recv() 收到 NET_UNDO_REQ 消息
  → handle_network_msg()
  → state.undo_req_pending = true —— 标记等待用户决定
  → draw_board() 在界面绘制"同意"/"拒绝"按钮弹窗
  → 用户点击同意
    → handle_control_click()
    → net_send_undo_accept() —— 向 A 发送同意回复
    → undo_to_turn(state, _RED) —— 退 2 步，回到 A 的回合
    → state.undo_req_pending = false —— 清除弹窗

A 方（主机）：
  recv() 收到 NET_UNDO_ACCEPT 消息
  → handle_network_msg()
  → undo_to_turn(state, _RED) —— 退 2 步，回到 A 的回合
  → net_set_waiting_undo(false) —— 解除等待
  → state.undo_req_pending = false
  → 棋盘操作解锁，A 可继续走棋

两个标志保证了双方状态在任何时刻一致：A 等待时不能走棋，B 弹窗未关闭时不能走棋，同意/拒绝后双方同步回到悔棋前 A 的回合。
