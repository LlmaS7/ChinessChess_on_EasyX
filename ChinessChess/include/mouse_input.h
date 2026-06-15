#pragma once
#include "game_state.h"

// 处理鼠标点击，return true 则成功 
// pX/pY: 鼠标像素坐标
// out_move: 输出走法
bool handle_mouse_click(GameState &state, int pX, int pY, Move &out_move);
