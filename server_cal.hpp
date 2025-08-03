#ifndef _SERVER_CAL_HPP_
#define _SERVER_CAL_HPP_ 1;

#include <iostream>
#include "protocol.hpp"

class ServerCal
{
public:
    Response CalculatorHelper(const Request &req)
    {
        Response resp(0, 0);
        switch (req.op_)
        {
        case '+':
            resp.res_ = req.x_ + req.y_;
            break;
        case '-':
            resp.res_ = req.x_ - req.y_;
            break;
        case '*':
            resp.res_ = req.x_ * req.y_;
            break;
        case '/':
        {
            if (req.y_ == 0)
                resp.code_ = divide_by_zero_error;
            else
                resp.res_ = req.x_ / req.y_;
        }
        break;
        case '%':
        {
            if (req.y_ == 0)
                resp.code_ = divide_by_zero_error;
            else
                resp.res_ = req.x_ % req.y_;
        }
        break;
        default:
            resp.code_ = operator_identify;
            break;
        }
        return resp;
    }
    std::string Calculator(std::string &package)
    {
        std::string content;
        if (!Decode(package, content)){
            return "";
        }
        Request req;
        if(!req.Deserialize(content)) return "";
        Response resp = CalculatorHelper(req);
        content = resp.Serialize();
        content = Encode(content);
        return content;
    }

private:
};

#endif