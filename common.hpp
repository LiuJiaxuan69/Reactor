#ifndef _COMMON_HPP_
#define _COMMON_HPP_ 1

#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <functional>


enum {
    NON_BLOCK_ERR = 1,
};

void SetNonBlockOrDie(int sock)
{
    int fl = fcntl(sock, F_GETFL);
    if (fl < 0)
        exit(NON_BLOCK_ERR);
    fcntl(sock, F_SETFL, fl | O_NONBLOCK);
}

#endif