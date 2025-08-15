#include <thread>
#include <memory>

#include <miniz.h>
#include <boost/asio.hpp>

#include "TestHelper.hpp"
#include "libslic3r/Utils.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/PartPlate.hpp"
#include "slic3r/GUI/GLCanvas3D.hpp"

using boost::asio::ip::tcp;
using namespace Slic3r;
using namespace Slic3r::GUI;

static const std::pair<unsigned int, unsigned int> THUMBNAIL_SIZE_3MF = {300, 300};

using ASync_Callback_Type = function<void(nlohmann::json)>;
static std::unordered_map<std::string, ASync_Callback_Type> async_callback_list;

namespace Test {

bool enable_test;
/////////////////////////////////////BASE////////////////////////////////////////

//static inline void test_helper_app_respone(std::string cmd, nlohmann::json ret)
//{
//    ADD_TEST_RESPONE("TestHelper", cmd, 0, ret.dump(-1, ' ', true));
//}

class ClientSession
{
public:
    ClientSession(tcp::socket socket) : socket_(std::move(socket)) {}

    void start() { do_read(); }

    void do_write(std::string datas)
    {
        boost::asio::async_write(socket_, boost::asio::buffer(datas.c_str(), datas.size()),
                                 [this](boost::system::error_code ec, std::size_t /*length*/) {                  
            });
    }

    std::function<void(std::string&)> read_callback = [](std::string&) {};

private:
    void do_read()
    {
        socket_.async_read_some(boost::asio::buffer(&readbuff, 2048), [this](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                std::string output(readbuff, length);
                read_callback(output);
            } 
            else {
                if (ec == boost::asio::error::eof || ec == boost::asio::error::connection_reset) {
                    socket_.close();
                    return;
                }
            }
            do_read();
        });
    }

    tcp::socket socket_;
    int         buff_len;
    char readbuff[2048];
};

class TCPServer
{
public:
    TCPServer(boost::asio::io_context& io_context, short port) : acceptor_(io_context)
    {
        boost::system::error_code ec;
        acceptor_.set_option(tcp::acceptor::reuse_address(true), ec);

        tcp::endpoint endpoint(tcp::v4(), port);
        acceptor_.open(endpoint.protocol());
        acceptor_.bind(endpoint);
        acceptor_.listen();
        do_accept();
    }

    std::function<void(ClientSession*)> accept_callback = [](ClientSession*) {};
    std::unique_ptr<ClientSession> client_;

private:
    void do_accept()
    {
        acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                client_.reset();
                client_ = std::make_unique<ClientSession>(std::move(socket));
                accept_callback(client_.get()); 
            }
            do_accept();
        });
    }

    tcp::acceptor acceptor_;
};

class CmdChannel
{
public:
    CmdChannel(short port)
    {
        server                  = new TCPServer(io_context, port);
        server->accept_callback = [this](ClientSession* session) {
            session->read_callback = std::bind(&CmdChannel::parse_string_to_cmd, this, std::placeholders::_1);
            session->start();
        };
    }

    void start()
    {
        try {
            thd = std::thread([this]() { io_context.run(); });
        } catch (std::exception& e) {}
    }
    void stop()
    {
        io_context.stop();
        if (thd.joinable())
            thd.join();
    }
    void cmd_respone(std::string respone) { 
        server->client_->do_write(respone);
    }

private:
    void parse_string_to_cmd(std::string& info)
    {
        auto cmd_index = info.find(";");
        auto cmd       = info.substr(0, cmd_index);
        auto arg       = info.substr(cmd_index+1, info.length() - cmd_index);
        Visitor().call_cmd(cmd, arg);
    }

    boost::asio::io_context io_context;
    TCPServer*              server;
    std::thread             thd;
};


TestHelper::TestHelper(bool enable)
{
    if (enable) {
        register_cmd();
        call_cmd = std::bind(&TestHelper::call_cmd_inner, this, std::placeholders::_1, std::placeholders::_2);
        init_cmd_channel(12345); // open cmd socket
    }
}

TestHelper::~TestHelper()
{
    if (m_cmd_channel) {
        m_cmd_channel->stop();
    }
}

void TestHelper::cmd_respone(std::string cmd, nlohmann::json ret) 
{ 
    m_cmd_channel->cmd_respone(cmd + ";" + ret.dump(-1, ' ', true)); 
}

void TestHelper::init_cmd_channel(short port)
{
    m_cmd_channel = new CmdChannel(port);
    m_cmd_channel->start();
}

void TestHelper::call_cmd_inner(std::string cmd, std::string json_str)
{
    constexpr int  FAILED = 1;
    constexpr int  OK     = 0;
    nlohmann::json ret;
    auto           gen_failed_json = [&ret, FAILED](std::string err) -> nlohmann::json& {
        ret["code"]    = FAILED;
        ret["error"]   = err;
        ret["payload"] = "";
        return ret;
    };
    auto gen_ok_json = [&ret, OK](std::string payload) -> nlohmann::json& {
        ret["code"]    = OK;
        ret["error"]   = "OK";
        ret["payload"] = payload;
        return ret;
    };

    auto it = m_cmd2func.find(cmd);
    if (it == m_cmd2func.end()) {
        cmd_respone(cmd, gen_failed_json("COMMAND NOT FAOUND"));
        return;
    }
    try {
        nlohmann::json arg;
        arg = json::parse(json_str);
        std::string err;
        std::string payload;
        int res = it->second(arg, payload, err);
        if (res == 0)
            cmd_respone(cmd, gen_ok_json(payload));
        else if (res > 0)
            cmd_respone(cmd, gen_failed_json(err));
        else if (res < 0)
            void(); // is async;
    } catch (const nlohmann::json::parse_error& err) {
        cmd_respone(cmd, gen_failed_json("PARSE arg FAILED"));
    } catch (const std::exception& e) {
        cmd_respone(cmd, gen_failed_json(std::string("EXEC CMD ERROR, ") + e.what()));
    }
}

void call_when_target_eventloop_exec(std::string cmd, nlohmann::json& arg, ASync_Callback_Type callback)
{
    wxCommandEvent e(EVT_TEST_HELPER_CMD);
    std::srand(std::time(nullptr));
    std::string cmdkey = std::to_string(std::rand());
    cmd += cmdkey;
    arg["cmd"] = cmd;
    e.SetString(arg.dump(-1, ' ', true));
    async_callback_list.insert({arg["cmd"], callback});
    wxPostEvent(&wxGetApp(), e);
}

/////////////////////////////////////CMD////////////////////////////////////////

static int capture(nlohmann::json arg, std::string& payload, std::string& error)
{
    nlohmann::json output;
    auto plater = wxGetApp().plater(); 
    if (!plater) 
    {
        error = "plater object is nullptr";
        return 1;
    }

    auto callback = [](nlohmann::json arg) {
        std::string save_as_fmt, type;
        int         capture_w, capture_h;
        nlohmann::json output;
        try {
            save_as_fmt = arg["save_as"].get<std::string>();
            type        = arg["type"].get<std::string>();
            capture_w   = arg["w"].get<int>();
            capture_h   = arg["h"].get<int>();
        } catch (nlohmann::json::parse_error& e) {
            nlohmann::json output;
            output["ret"] = 1;
            Test::Visitor().call_cmd("cmd_respone", output.dump(-1, ' ', true));
            return;
        }

        auto                                    plater = wxGetApp().plater();
        decltype(plater->get_view3D_canvas3D()) canvas_ins  = nullptr;
        if (arg["type"].get<std::string>() == "preview")
            canvas_ins = plater->get_preview_canvas3D();
        else if (arg["type"].get<std::string>() == "ready")
            canvas_ins = plater->get_view3D_canvas3D();
        else {
            output["ret"] = 1;
            output["error"] = "not support capture type";
            Test::Visitor().call_cmd("cmd_respone", output.dump(-1, ' ', true));
            return;
        }
        auto plater_size = plater->get_partplate_list().get_plate_count();
        for (auto i = 0; i < plater_size; ++i) {
            int                    THUMBNAIL_SIZE_3MF[] = {capture_w, capture_h};
            ThumbnailData          data;
            const ThumbnailsParams thumbnail_params = {{}, false, true, true, true, i};
            canvas_ins->render_thumbnail(data, THUMBNAIL_SIZE_3MF[0], THUMBNAIL_SIZE_3MF[1], thumbnail_params, Camera::EType::Ortho);

            size_t png_size = 0;
            void*  png_data = tdefl_write_image_to_png_file_in_memory_ex((const void*) data.pixels.data(), data.width, data.height, 4,
                                                                         &png_size, MZ_DEFAULT_COMPRESSION, 1);
            // save as png
            std::string save_as = save_as_fmt;
            auto        rep_inx = save_as.find("%s");
            save_as.replace(rep_inx, 2, std::to_string(i + 1));
            std::ofstream ofs(save_as, std::ofstream::binary);
            ofs.write((char*) png_data, png_size);
            ofs.close();
        }

        output["ret"] = 0;
        Test::Visitor().call_cmd("cmd_respone", output.dump(-1, ' ', true));
    };

    call_when_target_eventloop_exec("capture", arg, callback);
    return -1;
}

static int handle_app_cmd(nlohmann::json arg, std::string& payload, std::string& error)
{
    std::string cmd = arg["cmd"].get<std::string>();
    async_callback_list[cmd](arg);
    async_callback_list.erase(cmd);
    return -1;
}

static int cmd_respone_wrapper(nlohmann::json arg, std::string& payload, std::string& error)
{
    int ret = 0;
    if (arg.contains("ret"))
    {
        ret = arg["ret"].get<int>();
        arg.erase("ret");
    }
    if (arg.contains("error"))
    {
        error = arg["error"].get<std::string>();
        arg.erase("error");
    }

    payload = arg.dump(-1, ' ', true);
    return ret;
}

static int empty_respone(nlohmann::json arg, std::string& payload, std::string& error)
{
    return 0;
}

static int trigger_load_project(nlohmann::json arg, std::string& payload, std::string& error) 
{
    call_when_target_eventloop_exec(
        "trigger_load_project", 
        arg, 
        [](nlohmann::json arg) {
            wxGetApp().plater()->load_project(arg["file"].get<std::string>());
        });
    return 0; 
}

void TestHelper::register_cmd() 
{ 
    m_cmd2func["cmd_respone"]          = cmd_respone_wrapper;
    m_cmd2func["handle_app_cmd"]       = handle_app_cmd;
    m_cmd2func["capture"] = capture; 
    m_cmd2func["ping"] = empty_respone;
    m_cmd2func["trigger_load_project"] = trigger_load_project;
}

} // namespace Test
