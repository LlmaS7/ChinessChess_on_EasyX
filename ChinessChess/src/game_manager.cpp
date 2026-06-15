#include "../include/game_manager.h"
#include "../include/chess_engine.h"
#include "../include/draw_board.h"
#include "../include/menu.h"
#include "../network/network.h"
#include <graphics.h>
#include <cstdio>

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

bool is_my_turn(int menu_choice, int current_side)
{
    if (menu_choice == MENU_HOST)
        return current_side == _RED;
    if (menu_choice == MENU_CLIENT)
        return current_side == _BLACK;
    return true; // 单机模式总是本地操作
}

bool handle_network_msg(GameState &state, int menu_choice)
{
    NetMessage msg;
    if (!net_recv_msg_nonblock(msg))
    {
        // 对方断开连接 → 本方获胜
        if (!net_get_state().connected)
        {
            state.game_over = true;
            state.winner = (menu_choice == MENU_HOST) ? _RED : _BLACK;
        }
        return false;
    }

    switch (msg.type)
    {
    case NET_MOVE:
        if (!is_my_turn(menu_choice, state.current_side))
        {
            Move move;
            net_unpack_move(msg, move);
            apply_move(state, move.from_x, move.from_y, move.to_x, move.to_y);
            check_over(state);
        }
        break;
    case NET_UNDO_REQ:
        state.undo_req_pending = true;
        break;
    case NET_UNDO_ACCEPT:
    {
        int my_color = (menu_choice == MENU_HOST) ? _RED : _BLACK;
        undo_to_turn(state, my_color);
        net_set_waiting_undo(false);
        state.undo_req_pending = false;
        break;
    }
    case NET_UNDO_REJECT:
        net_set_waiting_undo(false);
        state.undo_req_pending = false;
        break;
    case NET_RESIGN:
        state.game_over = true;
        state.winner = (menu_choice == MENU_HOST) ? _RED : _BLACK;
        break;
    }
    return true;
}

bool net_startup(int menu_choice)
{
    if (!net_init())
    {
        printf("[main] 网络初始化失败\n");
        return false;
    }

    if (menu_choice == MENU_HOST)
    {
        char buf[128];
        sprintf(buf, "本机IP：%s\n\n正在等待对方连接...", net_get_local_ip());
        initgraph(400, 220);
        setbkcolor(RGB(220, 180, 130));
        cleardevice();
        settextstyle(20, 0, "楷体");
        setbkmode(TRANSPARENT);
        settextcolor(BLACK);
        outtextxy(40, 60, buf);
        outtextxy(40, 120, "请将此 IP 输入到连接窗口");

        net_host(8888);
        closegraph();
    }
    else
    {
        // 等待窗口
        initgraph(400, 200);
        setbkcolor(RGB(220, 180, 130));
        cleardevice();
        settextstyle(24, 0, "楷体");
        setbkmode(TRANSPARENT);
        settextcolor(BLACK);
        const char *wait_msg = "正在连接主机...";
        int tw = textwidth(wait_msg);
        outtextxy((400 - tw) / 2, 80, wait_msg);

        const char *ip = get_connect_ip();
        if (!net_connect(ip, 8888))
        {
            closegraph();
            net_cleanup();
            printf("[main] 连接失败\n");
            return false;
        }
        closegraph();
    }

    if (!net_get_state().connected)
    {
        printf("[main] 未检测到客机连接\n");
        net_cleanup();
        return false;
    }
    return true;
}

void game_over_sequence(GameState &state, bool is_network, bool is_host)
{
    if (is_network)
        net_send_game_over(state.winner);

    draw_board(state, is_network, is_host);
    Sleep(500);
    show_result(state);

    ExMessage em;
    while (true)
    {
        peekmessage(&em, EX_KEY);
        if (em.message == WM_KEYDOWN)
            break;
    }

    if (is_network)
        net_cleanup();
    closegraph();
}
