#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include "buffer.h"

class FileMonitor
{
public:
    FileMonitor() = default;
    virtual ~FileMonitor() = default;

    static std::shared_ptr<FileMonitor> create();
    
    void init();
    void add(const char *file_path);
    void remove(const char *file_path);

private:
    void on_timer();

    std::unordered_map<std::string, int> _file_mtime_map;
    Buffer _buffer;
};
