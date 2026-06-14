#include "../include/game_control.h"
#include "../include/chess_engine.h"
#include "../network/network.h"

bool handle_control_click(GameState &state, int px, int py, bool is_network, bool is_host)
{
    // 联机模式：悔棋请求弹窗按钮（接受/拒绝）在任何位置都可触发
    if (is_network && state.undo_req_pending)
    {
        int gap = UI_UNDO_Y - (UI_UNDO_REQ_Y + UI_REQ_BTN_H + 30);
        int req_y = is_host ? UI_UNDO_REQ_Y : UI_UNDO_Y + UI_BUTTON_H + gap + 5;
        if (py >= req_y + 22 && py <= req_y + 22 + UI_REQ_BTN_H)
        {
            // 同意
            if (px >= UI_ACCEPT_X && px <= UI_ACCEPT_X + UI_REQ_BTN_W)
            {
                net_send_undo_accept();
                int opp_color = is_host ? _BLACK : _RED;
                undo_to_turn(state, opp_color);
                state.undo_req_pending = false;
                return true;
            }
            // 拒绝
            if (px >= UI_REJECT_X && px <= UI_REJECT_X + UI_REQ_BTN_W)
            {
                net_send_undo_reject();
                state.undo_req_pending = false;
                return true;
            }
        }
        return true; // 弹窗期间吞掉所有其他点击
    }

    if (px < UI_BTN_X || px > UI_BTN_X + UI_BUTTON_W)
        return false;

    if (is_network)
    {
        // 联机模式：根据角色映射按钮位置
        int my_resign_y, opp_resign_y;
        if (is_host) // 红方：认输在下，空白在上
        {
            my_resign_y = UI_LOSE_R_Y;
            opp_resign_y = UI_LOSE_B_Y;
        }
        else // 黑方：认输在上，空白在下
        {
            my_resign_y = UI_LOSE_B_Y;
            opp_resign_y = UI_LOSE_R_Y;
        }

        // 己方认输
        if (py >= my_resign_y && py <= my_resign_y + UI_BUTTON_H)
        {
            state.game_over = true;
            state.winner = is_host ? _BLACK : _RED;
            net_send_resign(); // OPTIMIZED: 联机时通知对方认输
            return true;
        }
        // 悔棋 // OPTIMIZED: 联机时发送悔棋请求
        if (py >= UI_UNDO_Y && py <= UI_UNDO_Y + UI_BUTTON_H)
        {
            net_send_undo_req();
            net_set_waiting_undo(true);
            return true;
        }
        // 对方认输按钮区域（空白区）被点击时忽略
        if (py >= opp_resign_y && py <= opp_resign_y + UI_BUTTON_H)
            return true;

        return false;
    }

    // 单机模式
    if (py >= UI_LOSE_B_Y && py <= UI_LOSE_B_Y + UI_BUTTON_H)
    {
        // 黑认输
        state.game_over = true;
        state.winner = _RED;
        return true;
    }
    if (py >= UI_UNDO_Y && py <= UI_UNDO_Y + UI_BUTTON_H)
    {
        undo_move(state);
        return true;
    }
    if (py >= UI_LOSE_R_Y && py <= UI_LOSE_R_Y + UI_BUTTON_H)
    {
        // 红认输
        state.game_over = true;
        state.winner = _BLACK;
        return true;
    }
    return false;
}
