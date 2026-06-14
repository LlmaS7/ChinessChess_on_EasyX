#pragma once

#define MENU_SINGLE 1 // 单机模式
#define MENU_HOST 2   // 联机-创建主机
#define MENU_CLIENT 3 // 联机-连接主机

int show_start_menu();        // 返回 MENU_SINGLE / MENU_HOST / MENU_CLIENT
const char *get_connect_ip(); // 仅 MENU_CLIENT 时有效，返回目标 IP
