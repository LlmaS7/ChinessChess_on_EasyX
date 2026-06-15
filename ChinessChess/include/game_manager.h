#pragma once
#include "game_state.h"
#include "menu.h"

// 判断当前回合是否轮到本地玩家操作
bool is_my_turn(int menu_choice, int current_side);
// 处理右侧游戏控制按钮点击（认输/悔棋/悔棋响应），返回 true 表示已处理
bool handle_control_click(GameState &state, int px, int py, bool is_network, bool is_host);
// 处理一条网络消息（7 种类型的 switch 分发），返回 true 表示消息已处理
bool handle_network_msg(GameState &state, int menu_choice);
// 启动网络连接（主机等待/客机连接），失败返回 false
bool net_startup(int menu_choice);
// 联机游戏结算：显示结果，等待按键，清理资源
void game_over_sequence(GameState &state, bool is_network, bool is_host);
