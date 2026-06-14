#pragma once
#include "../include/chess_def.h"
#include <string>

//  网络消息协议
//  基于 TCP (Winsock2)，走棋双方通过局域网直连
// 消息类型
enum NetMsgType
{
    NET_MOVE,        // 走棋: data = Move 结构体 (7×int)
    NET_UNDO_REQ,    // 悔棋请求: data 为空
    NET_UNDO_ACCEPT, // 同意悔棋: data 为空
    NET_UNDO_REJECT, // 拒绝悔棋: data 为空
    NET_RESIGN,      // 认输: data 为空
    NET_GAME_OVER,   // 游戏结束: data = winner (int)
    NET_DISCONNECT   // 断开连接: data 为空
};

// 网络消息结构体
// 定长包头 + 变长负载
struct NetMessage
{
    int type;       // NetMsgType
    int data_len;   // 负载长度(字节)
    char data[256]; // 负载（最大 256 字节）
};

// 网络角色
enum NetRole
{
    ROLE_NONE = 0,
    ROLE_HOST = 1,  // 主机(红方,先手)
    ROLE_CLIENT = 2 // 客机(黑方,后手)
};

// 网络状态
struct NetState
{
    NetRole role;      // 角色
    bool connected;    // 是否已连接
    bool waiting_undo; // 对方请求悔棋等待响应
};

//  函数声明

//  生命周期
bool net_init();    // 初始化 Winsock
void net_cleanup(); // 清理 Winsock

// 连接
bool net_host(unsigned short port);                    // 创建主机, 等待客机连接
bool net_connect(const char *ip, unsigned short port); // 客机连接主机
void net_disconnect();                                 // 断开连接

//  收发
bool net_send_msg(const NetMessage &msg);    // 发送消息(阻塞)
bool net_recv_msg(NetMessage &msg);          // 接收消息(阻塞, 可超时)
bool net_recv_msg_nonblock(NetMessage &msg); // 接收消息(非阻塞)

//  便捷发送
bool net_send_move(const Move &move); // 发送走棋
bool net_send_undo_req();             // 发送悔棋请求
bool net_send_undo_accept();          // 发送同意悔棋
bool net_send_undo_reject();          // 发送拒绝悔棋
bool net_send_resign();               // 发送认输
bool net_send_game_over(int winner);  // 发送游戏结束

//  封包/解包
void net_pack_move(NetMessage &msg, const Move &move);
void net_unpack_move(const NetMessage &msg, Move &move);

//   工具
const char *net_role_str(NetRole role); // 角色名 -> "红方"/"黑方"
const char *net_msg_type_str(int type); // 消息类型名

const char *net_get_local_ip();          // 获取本机 IP 地址
const NetState &net_get_state();         // 获取网络状态（只读）
void net_set_waiting_undo(bool waiting); // 设置悔棋等待状态
