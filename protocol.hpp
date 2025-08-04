#ifndef _PROTOCOL_HPP_
#define _PROTOCOL_HPP_ 1

#include <iostream>

// 协议分隔符定义
const std::string blank_space_sep = " ";  // 字段间分隔符（空格）
const std::string protocol_sep = "\n";    // 协议分隔符（换行）

// 错误码枚举
enum {
    divide_by_zero_error = 1,  // 除零错误
    operator_identify,         // 操作符识别错误
};

// 协议解码函数
// 格式: "长度\n内容\n"
// 参数: package - 输入的网络数据包
//       content - 输出的解码后内容
// 返回值: 解码成功返回true，失败返回false
bool Decode(std::string &package, std::string &content)
{
    // 查找第一个分隔符位置
    size_t pos = package.find(protocol_sep);
    if(pos == std::string::npos) return false;
    
    // 获取内容长度
    size_t size = std::stoi(package.substr(0, pos));
    
    // 查找第二个分隔符位置
    size_t n_pos = package.find(protocol_sep, pos + 1);
    size_t total_len = pos + size + 2;  // 计算完整消息长度
    
    // 验证消息格式是否正确
    if(n_pos + 1 != total_len) {
        // 格式错误，清空数据包
        package.clear();
        return false;
    }
    
    // 提取消息内容
    content = package.substr(pos + 1, size);
    
    // 删除已处理的部分
    package.erase(0, total_len);
    return true;
}

// 协议编码函数
// 格式: "长度\n内容\n"
// 参数: content - 要编码的内容
// 返回值: 编码后的字符串
std::string Encode(std::string &content)
{
    std::string ret = std::to_string(content.size());  // 添加长度
    ret += protocol_sep;                               // 添加分隔符
    ret += content;                                    // 添加内容
    ret += protocol_sep;                               // 添加结束分隔符
    return ret;
}

// 请求类（客户端→服务端）
class Request{
public:
    // 构造函数，默认值表示无效请求
    Request(int x = 0, int y = 0, char op = '?')
    :x_(x),
    y_(y),
    op_(op){}
    
    // 序列化方法：将对象转换为字符串
    // 格式: "x op y"（如 "10 + 20"）
    std::string Serialize()
    {
        std::string ret = std::to_string(x_);
        ret += blank_space_sep;
        ret += op_;
        ret += blank_space_sep;
        ret += std::to_string(y_);
        return ret;
    }
    
    // 反序列化方法：从字符串解析对象
    // 参数: in - 输入字符串（格式必须为 "x op y"）
    // 返回值: 解析成功返回true，失败返回false
    bool Deserialize(std::string &in)
    {
        // 查找第一个空格位置
        size_t pos = in.find(blank_space_sep);
        if(pos == std::string::npos) return false;
        
        // 解析第一个操作数
        x_ = std::stoi(in.substr(0, pos));
        
        // 查找第二个空格位置
        size_t npos = in.find(blank_space_sep, pos + 1);
        if(npos == std::string::npos) return false;
        
        // 解析操作符
        op_ = in[pos + 1];
        
        // 解析第二个操作数
        y_ = std::stoi(in.substr(npos + 1));
        return true;
    }
public:
    int x_;     // 第一个操作数
    int y_;     // 第二个操作数
    char op_;   // 操作符（+,-,*,/等）
};

// 响应类（服务端→客户端）
class Response{
public:
    // 构造函数
    Response(int res = 0, int code = 0)
    :res_(res),
    code_(code){}
    
    // 序列化方法：将对象转换为字符串
    // 格式: "result code"（如 "30 0"）
    std::string Serialize()
    {
        std::string ret = std::to_string(res_);
        ret += blank_space_sep;
        ret += std::to_string(code_);
        return ret;
    }
    
    // 反序列化方法：从字符串解析对象
    // 参数: in - 输入字符串（格式必须为 "result code"）
    // 返回值: 解析成功返回true，失败返回false
    bool Deserialize(std::string &in)
    {
        // 查找空格位置
        size_t pos = in.find(blank_space_sep);
        if(pos == std::string::npos) return false;
        
        // 解析计算结果
        res_ = std::stoi(in.substr(0, pos));
        
        // 解析状态码
        code_ = std::stoi(in.substr(pos + 1));
        return true;
    }
public:
    int res_;    // 计算结果
    int code_;   // 状态码（0表示成功，非0表示错误）
};

#endif