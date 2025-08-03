#ifndef _NOCOPY_HPP_
#define _NOCOPY_HPP_ 1

class nocopy{
public:
    nocopy() = default;
    nocopy(const nocopy &) = delete;
    const nocopy &operator=(const nocopy &) = delete;
};

#endif