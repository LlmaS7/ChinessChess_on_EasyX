#include "../include/mouse_input.h"
#include "../include/chess_engine.h"
#include <cmath>

bool handle_mouse_click(GameState &state, int pX, int pY, Move &out_move)
{
    // 像素坐标转逻辑坐标 可优化？：微调修正
    int col = (pX - BOARD_LEFT + CELL_SIZE / 2) / CELL_SIZE;
    int row = (pY - BOARD_TOP + CELL_SIZE / 2) / CELL_SIZE;

    // 超出棋盘
    if (col < 0 || col >= COL || row < 0 || row >= ROW)
        return false;

    if (state.phase == SELECT_FIRST)
    {
        // 必须选己方棋子
        ChessPiece &p = state.board[row][col];
        if (p.id == SPACE || p.color != state.current_side)
            return false;

        state.selected_x = col;
        state.selected_y = row;
        state.phase = SELECT_SECOND;
        return false; // 还没走完，等下一步点击
    }
    else // SELECT_SECOND
    {
        // 点击同一个棋子 取消选中
        if (col == state.selected_x && row == state.selected_y)
        {
            state.selected_x = -1;
            state.selected_y = -1;
            state.phase = SELECT_FIRST;
            return false;
        }

        // 点击己方棋子 直接切换选中
        ChessPiece &new_piece = state.board[row][col];
        if (new_piece.id != SPACE && new_piece.color == state.current_side)
        {
            state.selected_x = col;
            state.selected_y = row;
            return false;
        }

        // 构造走法
        Move m;
        m.from_x = state.selected_x;
        m.from_y = state.selected_y;
        m.to_x = col;
        m.to_y = row;

        // 验证可行性
        if (!apply_move(state, m.from_x, m.from_y, m.to_x, m.to_y))
        {
            // 非法： 清空选中，回到选棋阶段
            state.selected_x = -1;
            state.selected_y = -1;
            state.phase = SELECT_FIRST;
            return false;
        }

        // 合法： 输出走法并重置阶段
        out_move = m;
        state.selected_x = -1;
        state.selected_y = -1;
        state.phase = SELECT_FIRST;
        return true;
    }
}