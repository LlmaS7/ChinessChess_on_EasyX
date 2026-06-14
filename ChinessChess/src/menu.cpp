#include "../include/menu.h"
#include <graphics.h>

// IP 输入缓冲区
static char g_connect_ip[16] = "";

const char *get_connect_ip()
{
    return g_connect_ip;
}

// 画一个按钮
static void draw_btn(int x, int y, int w, int h, const char *text, COLORREF color)
{
    setfillcolor(color);
    setlinecolor(BLACK);
    fillrectangle(x, y, x + w, y + h);
    settextstyle(24, 0, "楷体");
    setbkmode(TRANSPARENT);
    settextcolor(BLACK);
    int tw = textwidth(text);
    int th = textheight(text);
    outtextxy(x + (w - tw) / 2, y + (h - th) / 2, text);
}

// 判断鼠标是否在按钮区域内
static bool in_btn(int mx, int my, int bx, int by, int bw, int bh)
{
    return mx >= bx && mx <= bx + bw && my >= by && my <= by + bh;
}

// 等待鼠标点击，返回点击坐标（阻塞）
static void wait_click(int &mx, int &my)
{
    while (true)
    {
        if (MouseHit())
        {
            MOUSEMSG m = GetMouseMsg();
            if (m.uMsg == WM_LBUTTONDOWN)
            {
                mx = m.x;
                my = m.y;
                return;
            }
        }
        // 允许关闭窗口
        ExMessage em;
        while (peekmessage(&em, EX_WINDOW))
        {
            if (em.message == WM_CLOSE)
            {
                mx = my = -1;
                return;
            }
        }
    }
}

//  开始菜单（支持单机 / 联机主机 / 联机客机）

int show_start_menu()
{
    initgraph(640, 640);

    const int BTN_W = 200;
    const int BTN_H = 48;
    const int BTN_X = (640 - BTN_W) / 2;

    while (true)
    {
        // 主界面
        setbkcolor(RGB(220, 180, 130));
        cleardevice();

        // 标题
        settextstyle(48, 0, "楷体");
        setbkmode(TRANSPARENT);
        settextcolor(BLACK);
        const char *title = "中 国 象 棋";
        int tw = textwidth(title);
        outtextxy((640 - tw) / 2, 160, title);

        // 按钮
        draw_btn(BTN_X, 280, BTN_W, BTN_H, "单 机 模 式", RGB(180, 210, 160));
        draw_btn(BTN_X, 360, BTN_W, BTN_H, "联 机 模 式", RGB(160, 190, 220));

        // 等待点击
        int mx, my;
        wait_click(mx, my);
        if (mx < 0)
        {
            closegraph();
            return MENU_SINGLE;
        } // 关窗→退出

        if (in_btn(mx, my, BTN_X, 280, BTN_W, BTN_H))
        {
            closegraph();
            return MENU_SINGLE;
        }
        else if (in_btn(mx, my, BTN_X, 360, BTN_W, BTN_H))
        {
            // ---------- 联机子菜单 ----------
            while (true)
            {
                setbkcolor(RGB(200, 170, 120));
                cleardevice();

                settextstyle(36, 0, "楷体");
                setbkmode(TRANSPARENT);
                settextcolor(BLACK);
                outtextxy((640 - textwidth("联 机 对 战")) / 2, 160, "联 机 对 战");

                draw_btn(BTN_X, 260, BTN_W, BTN_H, "创 建 主 机", RGB(180, 210, 160));
                draw_btn(BTN_X, 340, BTN_W, BTN_H, "连 接 主 机", RGB(160, 190, 220));
                draw_btn(BTN_X, 420, BTN_W, BTN_H, "返      回", RGB(200, 200, 200));

                wait_click(mx, my);
                if (mx < 0)
                {
                    closegraph();
                    return MENU_SINGLE;
                }

                if (in_btn(mx, my, BTN_X, 260, BTN_W, BTN_H))
                {
                    closegraph();
                    return MENU_HOST;
                }
                else if (in_btn(mx, my, BTN_X, 340, BTN_W, BTN_H))
                {
                    // 输入 IP
                    InputBox(g_connect_ip, 16, "请输入主机 IP 地址", "连接主机", "192.168.");
                    closegraph();
                    return MENU_CLIENT;
                }
                else if (in_btn(mx, my, BTN_X, 420, BTN_W, BTN_H))
                {
                    break; // 返回主菜单
                }
            }
        }
    }
}
