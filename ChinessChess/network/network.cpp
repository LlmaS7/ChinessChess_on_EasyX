#include "network.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdio>

//  静态变量（模块内部状态）

static SOCKET g_listen_sock = INVALID_SOCKET; // 主机监听套接字
static SOCKET g_sock = INVALID_SOCKET;        // 通信套接字
static NetState g_net = {ROLE_NONE, false, false};

// 设置套接字超时的辅助函数
// OPTIMIZED: 复用超时设置逻辑
static bool set_sock_timeout(SOCKET s, int recv_timeout_ms, int send_timeout_ms)
{
    if (recv_timeout_ms > 0)
    {
        int tv = recv_timeout_ms;
        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv)) != 0)
            return false;
    }
    if (send_timeout_ms > 0)
    {
        int tv = send_timeout_ms;
        if (setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tv, sizeof(tv)) != 0)
            return false;
    }
    return true;
}

//  生命周期

bool net_init()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("[net] WSAStartup 失败\n");
        return false;
    }
    printf("[net] Winsock 初始化成功\n");
    return true;
}

void net_cleanup()
{
    net_disconnect();
    WSACleanup();
    printf("[net] Winsock 已清理\n");
}

//  连接

bool net_host(unsigned short port)
{
    // 1，创建TCP监听套数字
    g_listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (g_listen_sock == INVALID_SOCKET)
    {
        printf("[net] socket()失败:%d\n", WSAGetLastError());
        return false;
    }
    // OPTIMIZED: 允许端口立即重用，防止崩溃重启后bind失败
    {
        int opt = 1;
        setsockopt(g_listen_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
    }
    // 2，绑定本机地址+端口
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(g_listen_sock, (sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        printf("[net] bind()失败：%d\n", WSAGetLastError());
        closesocket(g_listen_sock);
        g_listen_sock = INVALID_SOCKET;
        return false;
    }
    // 3,开始监听，最多等队列数1
    if (listen(g_listen_sock, 1) == SOCKET_ERROR)
    {
        printf("[net] listen() 失败: %d\n", WSAGetLastError());
        closesocket(g_listen_sock);
        g_listen_sock = INVALID_SOCKET;
        return false;
    }
    printf("[net] 主机模式：等待客机连接端口 %u ...\n", port);

    // 4，等待客机连接（带超时）
    //  OPTIMIZED: 无限等待客机连接，移除超时限制
    sockaddr_in client_addr;
    int client_len = sizeof(client_addr);
    {
        while (true)
        {
            fd_set accept_set;
            FD_ZERO(&accept_set);
            FD_SET(g_listen_sock, &accept_set);
            struct timeval accept_tv = {0, 200000}; // 200ms
            int sel_ret = select(0, &accept_set, NULL, NULL, &accept_tv);
            if (sel_ret > 0)
            {
                g_sock = accept(g_listen_sock, (sockaddr *)&client_addr, &client_len);
                if (g_sock != INVALID_SOCKET)
                {
                    break;
                }
            }
        }
    }
    // 5,连接建立成功
    // OPTIMIZED: accept完成后关闭监听套接字，释放端口
    closesocket(g_listen_sock);
    g_listen_sock = INVALID_SOCKET;
    // OPTIMIZED: 设置通信套接字收发超时 + 禁用Nagle算法
    set_sock_timeout(g_sock, 100, 10000); // recv 100ms, send 10s
    {
        int flag = 1;
        setsockopt(g_sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&flag, sizeof(flag));
    }
    g_net.role = ROLE_HOST;
    g_net.connected = true;
    printf("[net] 客机已接入！\n");
    return true;
}

bool net_connect(const char *ip, unsigned short port)
{
    // 1,创建TCP套文字
    g_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (g_sock == INVALID_SOCKET)
    {
        printf("[net] socket() 失败： %d\n", WSAGetLastError());
        return false;
    }
    // OPTIMIZED: 设置连接超时，防止20+秒阻塞
    set_sock_timeout(g_sock, 100, 5000); // recv 100ms, send 5s (connect受SO_SNDTIMEO影响)
    // 2,连接到主机
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip); // IP字符串->网络地址
    addr.sin_port = htons(port);
    if (connect(g_sock, (sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        printf("[net] connect() 失败：%d\n", WSAGetLastError());
        closesocket(g_sock);
        g_sock = INVALID_SOCKET;
        return false;
    }
    // OPTIMIZED: 连接成功后设置收发超时 + 禁用Nagle算法
    set_sock_timeout(g_sock, 100, 10000); // recv 100ms, send 10s
    {
        int flag = 1;
        setsockopt(g_sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&flag, sizeof(flag));
    }
    // 3,连接建立
    g_net.role = ROLE_CLIENT;
    g_net.connected = true;
    printf("[net] 已连接到主机！\n");
    return true;
}

void net_disconnect()
{
    // OPTIMIZED: 关闭所有套接字，防止资源泄漏
    if (g_sock != INVALID_SOCKET)
    {
        closesocket(g_sock);
        g_sock = INVALID_SOCKET;
    }
    if (g_listen_sock != INVALID_SOCKET)
    {
        closesocket(g_listen_sock);
        g_listen_sock = INVALID_SOCKET;
    }
    g_net.role = ROLE_NONE;
    g_net.connected = false;
    g_net.waiting_undo = false;
    printf("[net] 已断开\n");
}

//  收发

bool net_send_msg(const NetMessage &msg)
{
    // OPTIMIZED: 检查套接字有效性
    if (g_sock == INVALID_SOCKET)
    {
        printf("[net] send() 套接字无效\n");
        return false;
    }
    // 先发送包头（type+datalen=8字节）
    int header[2] = {msg.type, msg.data_len};
    // OPTIMIZED: 循环处理部分发送
    {
        int total = 0;
        while (total < (int)sizeof(header))
        {
            int ret = send(g_sock, (const char *)header + total, sizeof(header) - total, 0);
            if (ret <= 0)
            {
                printf("[net] send() 包头失败：%d\n", WSAGetLastError());
                return false;
            }
            total += ret;
        }
    }
    // 再发负载数据
    if (msg.data_len > 0)
    {
        // OPTIMIZED: 循环处理部分发送
        int total = 0;
        while (total < msg.data_len)
        {
            int ret = send(g_sock, msg.data + total, msg.data_len - total, 0);
            if (ret <= 0)
            {
                printf("[net] send() 数据失效：%d\n", WSAGetLastError());
                return false;
            }
            total += ret;
        }
    }
    printf("[net] 发送消息： %s(%d 字节)\n", net_msg_type_str(msg.type), msg.data_len);
    return true;
}

bool net_recv_msg(NetMessage &msg) // 阻塞(实际受SO_RCVTIMEO控制超时)
{
    // OPTIMIZED: 检查套接字有效性
    if (g_sock == INVALID_SOCKET)
    {
        printf("[net] recv() 套接字无效\n");
        return false;
    }
    // 1，接收包头
    int header[2];
    // OPTIMIZED: 循环处理部分接收
    int total_hdr = 0;
    while (total_hdr < (int)sizeof(header))
    {
        int ret = recv(g_sock, (char *)header + total_hdr, sizeof(header) - total_hdr, 0);
        if (ret <= 0)
        {
            if (ret == 0)
            {
                printf("[net] 对方已断开连接\n");
            }
            else
            {
                int err = WSAGetLastError();
                if (err != WSAETIMEDOUT)
                    printf("[net] recv() 包头失效：%d\n", err);
            }
            g_net.connected = false;
            return false;
        }
        total_hdr += ret;
    }
    msg.type = header[0];
    msg.data_len = header[1];
    // OPTIMIZED: 消息类型/长度合法性校验
    if (msg.type < NET_MOVE || msg.type > NET_DISCONNECT)
    {
        printf("[net] 非法消息类型：%d\n", msg.type);
        return false;
    }
    if (msg.data_len < 0 || msg.data_len > (int)sizeof(msg.data))
    {
        printf("[net] 非法数据长度：%d\n", msg.data_len);
        return false;
    }
    // 2,接收负载
    if (msg.data_len > 0)
    {
        int total = 0;
        while (total < msg.data_len)
        {
            int ret = recv(g_sock, msg.data + total, msg.data_len - total, 0);
            if (ret <= 0)
            {
                int err = WSAGetLastError();
                if (err != WSAETIMEDOUT)
                    printf("[net] recv() 数据失效：%d\n", err);
                g_net.connected = false;
                return false;
            }
            total += ret;
        }
    }
    printf("[net] 收到消息： %s(%d 字节)\n", net_msg_type_str(msg.type), msg.data_len);
    return true;
}

bool net_recv_msg_nonblock(NetMessage &msg)
{
    // OPTIMIZED: 使用select实现真正的非阻塞接收
    if (g_sock == INVALID_SOCKET)
        return false;
    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(g_sock, &read_set);
    struct timeval tv = {0, 0};
    int sel_ret = select(0, &read_set, NULL, NULL, &tv);
    if (sel_ret <= 0)
        return false;
    return net_recv_msg(msg);
}

bool net_send_move(const Move &move)
{
    NetMessage msg;
    net_pack_move(msg, move);
    return net_send_msg(msg);
}

bool net_send_undo_req()
{
    NetMessage msg;
    msg.type = NET_UNDO_REQ;
    msg.data_len = 0;
    return net_send_msg(msg);
}

bool net_send_undo_accept()
{
    NetMessage msg;
    msg.type = NET_UNDO_ACCEPT;
    msg.data_len = 0;
    return net_send_msg(msg);
}

bool net_send_undo_reject()
{
    NetMessage msg;
    msg.type = NET_UNDO_REJECT;
    msg.data_len = 0;
    return net_send_msg(msg);
}

bool net_send_resign()
{
    NetMessage msg;
    msg.type = NET_RESIGN;
    msg.data_len = 0;
    return net_send_msg(msg);
}

bool net_send_game_over(int winner)
{
    NetMessage msg;
    msg.type = NET_GAME_OVER;
    msg.data_len = sizeof(int);
    *(int *)msg.data = winner;
    return net_send_msg(msg);
}

//  封包 / 解包

void net_pack_move(NetMessage &msg, const Move &move)
{
    msg.type = NET_MOVE;
    msg.data_len = sizeof(Move);
    // 直接按字节拷贝 Move 结构体
    *(Move *)msg.data = move;
}

void net_unpack_move(const NetMessage &msg, Move &move)
{
    move = *(const Move *)msg.data;
}

//  工具

const char *net_role_str(NetRole role)
{
    switch (role)
    {
    case ROLE_HOST:
        return "主机(红方)";
    case ROLE_CLIENT:
        return "客机(黑方)";
    default:
        return "未连接";
    }
}

const char *net_msg_type_str(int type)
{
    switch (type)
    {
    case NET_MOVE:
        return "NET_MOVE";
    case NET_UNDO_REQ:
        return "NET_UNDO_REQ";
    case NET_UNDO_ACCEPT:
        return "NET_UNDO_ACCEPT";
    case NET_UNDO_REJECT:
        return "NET_UNDO_REJECT";
    case NET_RESIGN:
        return "NET_RESIGN";
    case NET_GAME_OVER:
        return "NET_GAME_OVER";
    case NET_DISCONNECT:
        return "NET_DISCONNECT";
    default:
        return "未知消息";
    }
}

//  外部获取 NetState（供 main.cpp 使用）
const char *net_get_local_ip()
{
    static char ip[32] = {0};
    char hostname[256] = {0};
    if (gethostname(hostname, sizeof(hostname)) != 0)
        return "unknown";
    struct hostent *he = gethostbyname(hostname);
    if (!he || !he->h_addr_list[0])
        return "unkown";
    sprintf(ip, "%d.%d.%d.%d",
            (unsigned char)he->h_addr_list[0][0],
            (unsigned char)he->h_addr_list[0][1],
            (unsigned char)he->h_addr_list[0][2],
            (unsigned char)he->h_addr_list[0][3]);
    return ip;
}

// 通过函数暴露给外部，保持 g_net 为模块私有
const NetState &net_get_state()
{
    return g_net;
}

void net_set_waiting_undo(bool waiting)
{
    g_net.waiting_undo = waiting;
}
