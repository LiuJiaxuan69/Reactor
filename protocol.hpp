#ifndef _PROTOCOL_HPP_
#define _PROTOCOL_HPP_ 1

#include <iostream>
// #include <jsoncpp/json/json.h>


const std::string blank_space_sep = " ";
const std::string protocol_sep = "\n";

enum {
    divide_by_zero_error = 1,
    operator_identify,
};

// num'\n'  'x'  ' '  'op' ' '  'y'  '\n'
bool Decode(std::string &package, std::string &content)
{
    size_t pos = package.find(protocol_sep);
    if(pos == std::string::npos) return false;
    size_t size = std::stoi(package.substr(0, pos));
    // 采用 JSON 就不可以检查这些了，因为
    // size_t n_pos = package.find(protocol_sep, pos + 1);
    size_t total_len = pos + size + 2;
    // if(n_pos + 1 != total_len) {
    //     // 用户发的格式错误（未遵守协议），后面的请求也无法保证正确性，未遵守协议意味着size的大小可信度存疑，故直接清空 content
    //     package.clear();
    //     return false;
    // }
    content = package.substr(pos + 1, size);
    // 删除已被解码的字段
    package.erase(0, total_len);
    return true;
}

std::string Encode(std::string &content)
{
    std::string ret = std::to_string(content.size());
    ret += protocol_sep;
    ret += content;
    ret += protocol_sep;
    return ret;
}

class Request{
public:
    // 若直接传递未传参的 Request 对象，服务端接收后会报错
    Request(int x = 0, int y = 0, char op = '?')
    :x_(x),
    y_(y),
    op_(op){}
    std::string Serialize()
    {
        std::string ret = std::to_string(x_);
        ret += blank_space_sep;
        ret += op_;
        ret += blank_space_sep;
        ret += std::to_string(y_);
        return ret;

        // Json::Value root;
        // root["x"] = x_;
        // root["y"] = y_;
        // root["op"] = op_;
        // Json::StyledWriter w;
        // return w.write(root);
    }
    // "x op y"
    bool Deserialize(std::string &in)
    {
        size_t pos = in.find(blank_space_sep);
        if(pos == std::string::npos) return false;
        x_ = std::stoi(in.substr(0, pos));
        size_t npos = in.find(blank_space_sep, pos + 1);
        if(npos == std::string::npos) return false;
        op_ = in[pos + 1];
        y_ = std::stoi(in.substr(npos + 1));
        return true;

        // Json::Value root;
        // Json::Reader r;
        // r.parse(in, root);
        // x_ = root["x"].asInt();
        // y_ = root["y"].asInt();
        // op_ = root["op"].asInt();
        // return true;
    }
public:
    int x_;
    int y_;
    char op_;
};

class Response{
public:
    Response(int res = 0, int code = 0)
    :res_(res),
    code_(code){}
    // "result code"
    std::string Serialize()
    {
        std::string ret = std::to_string(res_);
        ret += blank_space_sep;
        ret += std::to_string(code_);
        return ret;

        // Json::Value root;
        // root["res"] = res_;
        // root["code"] = code_;
        // Json::StyledWriter w;
        // return w.write(root);
    }
    bool Deserialize(std::string &in)
    {
        size_t pos = in.find(blank_space_sep);
        if(pos == std::string::npos) return false;
        res_ = std::stoi(in.substr(0, pos));
        code_ = std::stoi(in.substr(pos + 1));
        return true;

        // Json::Value root;
        // Json::Reader r;
        // r.parse(in, root);
        // res_ = root["res"].asInt();
        // code_ = root["code"].asInt();
        // return true;
    }
public:
    int res_;
    int code_;
};

#endif