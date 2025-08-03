#ifndef _LOGHPP_
#define _LOGHPP_ 1

#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

#define SIZE 1024
#define LogFile "log.txt"

enum Errorlevel
{
    Info,
    Debug,
    Warning,
    Error,
    Fatal
};

enum PrintWay
{
    Screen = 1,
    Onefile,
    Classfile
};

class log
{
public:
    log()
    {
        printMethod = Screen;
        path = "./log/";
    }
    void Enable(int method)
    {
        printMethod = method;
    }
    std::string levelToString(int level)
    {
        switch (level)
        {
        case Info:
            return "Info";
        case Debug:
            return "Debug";
        case Warning:
            return "Warning";
        case Error:
            return "Error";
        case Fatal:
            return "Fatal";
        default:
            return "None";
        }
    }

    void PrintLog(int level, const std::string &logtxt)
    {
        switch (printMethod)
        {
        case Screen:
            printf("%s", logtxt.c_str());
            break;
        case Onefile:
            PrintOneFile(LogFile, logtxt);
            break;
        case Classfile:
            PrintClassFile(level, logtxt);
            break;
        default:
            break;
        }
    }

    void operator()(int level, const char *format, ...)
    {
        time_t t = time(nullptr);
        struct tm *ctime = localtime(&t);
        // left information(error level and the local time)
        char leftbuffer[SIZE];
        snprintf(leftbuffer, sizeof(leftbuffer), "[%s][%d-%d-%d %d:%d:%d]", levelToString(level).c_str(), 1900 + ctime->tm_year, 1 + ctime->tm_mon, ctime->tm_mday,
                 ctime->tm_hour, ctime->tm_min, ctime->tm_sec);
        // right information(other information)
        char rightbuffer[SIZE];
        va_list s;
        va_start(s, format);
        vsnprintf(rightbuffer, sizeof(rightbuffer), format, s);
        va_end(s);

        // merge the two pieces of information
        char log[SIZE * 2];
        snprintf(log, sizeof(log), "%s %s\n", leftbuffer, rightbuffer);
        PrintLog(level, log);
    }
    void PrintOneFile(const std::string &logfile, const std::string &logtxt)
    {
        std::string _logfile = path + logfile;
        int fd = open(_logfile.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0666);
        if (fd < 0)
            return;
        write(fd, logtxt.c_str(), logtxt.size());
        close(fd);
    }
    void PrintClassFile(int level, const std::string &logtxt)
    {
        std::string filename = LogFile;
        filename += '.';
        filename += levelToString(level);
        PrintOneFile(filename, logtxt);
    }
    ~log() {}

private:
    int printMethod;
    std::string path;
};

inline log lg;

#endif
