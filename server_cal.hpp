#ifndef _SERVER_CAL_HPP_
#define _SERVER_CAL_HPP_ 1;

#include <iostream>
#include "protocol.hpp"  // 包含之前定义的自定义协议头文件

// 计算器服务端类
class ServerCal
{
public:
    // 辅助计算函数，处理具体运算逻辑
    // 参数: req - 包含计算请求的Request对象
    // 返回值: 包含计算结果和状态的Response对象
    Response CalculatorHelper(const Request &req)
    {
        Response resp(0, 0);  // 初始化响应对象，默认结果0，状态码0(成功)
        
        // 根据操作符执行不同运算
        switch (req.op_)
        {
        case '+':
            resp.res_ = req.x_ + req.y_;  // 加法运算
            break;
        case '-':
            resp.res_ = req.x_ - req.y_;  // 减法运算
            break;
        case '*':
            resp.res_ = req.x_ * req.y_;  // 乘法运算
            break;
        case '/':
        {
            // 除法运算，检查除数是否为0
            if (req.y_ == 0)
                resp.code_ = divide_by_zero_error;  // 除零错误
            else
                resp.res_ = req.x_ / req.y_;  // 正常除法
        }
        break;
        case '%':
        {
            // 取模运算，检查模数是否为0
            if (req.y_ == 0)
                resp.code_ = divide_by_zero_error;  // 除零错误
            else
                resp.res_ = req.x_ % req.y_;  // 正常取模
        }
        break;
        default:
            resp.code_ = operator_identify;  // 未知操作符错误
            break;
        }
        return resp;
    }

    // 主计算函数，处理协议解码和编码
    // 参数: package - 网络接收到的原始数据包
    // 返回值: 编码后的响应字符串，出错返回空字符串
    std::string Calculator(std::string &package)
    {
        std::string content;
        
        // 1. 解码网络数据包
        if (!Decode(package, content)){
            return "";  // 解码失败返回空字符串
        }
        
        // 2. 反序列化请求内容
        Request req;
        if(!req.Deserialize(content)) return "";  // 反序列化失败返回空字符串
        
        // 3. 执行计算逻辑
        Response resp = CalculatorHelper(req);

        // 4. 序列化响应结果
        content = resp.Serialize();
        
        // 5. 编码为网络协议格式
        content = Encode(content);
        
        return content;
    }

};

#endif