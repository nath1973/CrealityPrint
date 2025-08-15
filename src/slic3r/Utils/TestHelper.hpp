#ifndef __test_helper_hpp_
#define __test_helper_hpp_

#include <string>

#include "nlohmann/json.hpp"

namespace Test {

class CmdChannel;

extern bool enable_test;

class TestHelper
{
    using Cmd_Type = int(nlohmann::json, std::string&, std::string&);

public:
    static TestHelper& instance()
    {
        static TestHelper ins(Test::enable_test);
        return ins;
    }
    std::function<void(std::string, std::string)> call_cmd = [](std::string, std::string) {}; // Î¨Ò»±©Â¶½Ó¿Ú
    
private:
    TestHelper(bool enable);
    ~TestHelper();
    void call_cmd_inner(std::string cmd, std::string json_str);
    void register_cmd();
    void init_cmd_channel(short port);
    void cmd_respone(std::string cmd, nlohmann::json ret);

private:
    std::unordered_map<std::string, std::function<Cmd_Type>> m_cmd2func;
    CmdChannel* m_cmd_channel = nullptr;
};

inline void Init(bool enable)
{
    enable_test = enable;
    TestHelper::instance();
}

inline TestHelper& Visitor() { return TestHelper::instance(); }


} // Test namespace
#endif // __test_helper_hpp_