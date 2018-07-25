local msg_type =
{
    socket_error = 0,
    socket_close = 1,
    socket_accept = 2,
    socket_recv = 3,
    kcp_output = 4,
}

rawset(_G, "msg_type", msg_type)
