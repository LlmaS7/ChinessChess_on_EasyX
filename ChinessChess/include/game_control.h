#pragma once
#include "game_state.h"

// 处理右侧游戏控制按钮点击（认输/悔棋/悔棋响应），返回 true 表示已处理
bool handle_control_click(GameState &state, int px, int py, bool is_network, bool is_host);
