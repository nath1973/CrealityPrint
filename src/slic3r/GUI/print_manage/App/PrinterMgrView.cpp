#include "PrinterMgrView.hpp"

#include "../../I18N.hpp"
#include "../AccountDeviceMgr.hpp"
#include "slic3r/GUI/wxExtensions.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "slic3r/GUI/Notebook.hpp"
#include "libslic3r_version.h"

#include <regex>
#include <string>
#include <wx/sizer.h>
#include <wx/string.h>
#include <wx/toolbar.h>
#include <wx/textdlg.h>
#include "wx/evtloop.h"

#include <slic3r/GUI/Widgets/WebView.hpp>
#include <wx/webview.h>
#include "slic3r/GUI/print_manage/RemotePrinterManager.hpp"
#include <boost/beast/core/detail/base64.hpp>

#include <wx/stdpaths.h>
#include "../utils/cxmdns.h"
#include "slic3r/GUI/print_manage/Utils.hpp"
#include "slic3r/GUI/print_manage/AccountDeviceMgr.hpp"
#include "wx/event.h"
#include "../data/DataCenter.hpp"
#include "../AppMgr.hpp"
#include "../PrinterMgr.hpp"
#include "../TypeDefine.hpp"


namespace pt = boost::property_tree;

namespace Slic3r {
namespace GUI {

PrinterMgrView::PrinterMgrView(wxWindow *parent)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize)
 {
    wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);

      // Create the webview
    m_browser = WebView::CreateWebView(this, "");
    if (m_browser == nullptr) {
        wxLogError("Could not init m_browser");
        return;
    }

    m_browser->Bind(wxEVT_WEBVIEW_ERROR, &PrinterMgrView::OnError, this);
    m_browser->Bind(wxEVT_WEBVIEW_LOADED, &PrinterMgrView::OnLoaded, this);
    Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, &PrinterMgrView::OnScriptMessage, this, m_browser->GetId());

    SetSizer(topsizer);

    topsizer->Add(m_browser, wxSizerFlags().Expand().Proportion(1));

    //Zoom
    m_zoomFactor = 100;

    //Connect the idle events
    Bind(wxEVT_CLOSE_WINDOW, &PrinterMgrView::OnClose, this);

    RegisterHandler("set_device_relate_to_account", [this](const nlohmann::json& json_data) {
        this->handle_set_device_relate_to_account(json_data);
    });

    RegisterHandler("request_update_device_relate_to_account", [this](const nlohmann::json& json_data) {
        this->handle_request_update_device_relate_to_account(json_data);
    });

//#define _DEBUG1
#ifdef _DEBUG1
        this->load_url(wxString("http://localhost:5173/"), wxString());
         m_browser->EnableAccessToDevTools();
     #else
        //wxString url = wxString::Format("http://localhost:%d/deviceMgr/index.html", wxGetApp().get_server_port());
        wxString url = wxString::Format("%s/web/deviceMgr/index.html", from_u8(resources_dir()));
        url.Replace(wxT("\\"), wxT("/"));
        url.Replace(wxT("#"), wxT("%23"));
        wxURI uri(url);
        wxString encodedUrl = uri.BuildURI();
        encodedUrl = wxT("file://")+encodedUrl;
        this->load_url(encodedUrl, wxString());

        // this->load_url(wxString("http://localhost:5173/"), wxString());
        // m_browser->EnableAccessToDevTools();

     #endif
    DM::AppMgr::Ins().Register(m_browser);
    DM::AppMgr::Ins().RegisterEvents(m_browser, std::vector<std::string>{DM::EVENT_SET_CURRENT_DEVICE, DM::EVENT_FORWARD_DEVICE_DETAIL});
 }

PrinterMgrView::~PrinterMgrView()
{
    DM::AppMgr::Ins().UnRegister(m_browser);
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " Start";
    SetEvtHandlerEnabled(false);

    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " End";
}

void PrinterMgrView::load_url(const wxString& url, wxString apikey)
{
    if (m_browser == nullptr)
        return;
    m_apikey = apikey;
    m_apikey_sent = false;
    m_browser->LoadURL(url);
    //m_browser->SetFocus();
    UpdateState();
}

void PrinterMgrView::on_switch_to_device_page()
{
    //update_which_device_is_current();
    forward_init_device_cmd_to_printer_list();
}

void PrinterMgrView::reload()
{
    m_browser->Reload();
}
/**
 * Method that retrieves the current state from the web control and updates the
 * GUI the reflect this current state.
 */
void PrinterMgrView::UpdateState() {
  // SetTitle(m_browser->GetCurrentTitle());

}

void PrinterMgrView::OnClose(wxCloseEvent& evt)
{
    this->Hide();
}

void PrinterMgrView::SendAPIKey()
{
    if (m_apikey_sent || m_apikey.IsEmpty())
        return;
    m_apikey_sent   = true;
    wxString script = wxString::Format(R"(
    // Check if window.fetch exists before overriding
    if (window.fetch) {
        const originalFetch = window.fetch;
        window.fetch = function(input, init = {}) {
            init.headers = init.headers || {};
            init.headers['X-API-Key'] = '%s';
            return originalFetch(input, init);
        };
    }
)",
                                       m_apikey);
    m_browser->RemoveAllUserScripts();

    m_browser->AddUserScript(script);
    m_browser->Reload();
}

void PrinterMgrView::OnError(wxWebViewEvent &evt)
{
    auto e = "unknown error";
    switch (evt.GetInt()) {
      case wxWEBVIEW_NAV_ERR_CONNECTION:
        e = "wxWEBVIEW_NAV_ERR_CONNECTION";
        break;
      case wxWEBVIEW_NAV_ERR_CERTIFICATE:
        e = "wxWEBVIEW_NAV_ERR_CERTIFICATE";
        break;
      case wxWEBVIEW_NAV_ERR_AUTH:
        e = "wxWEBVIEW_NAV_ERR_AUTH";
        break;
      case wxWEBVIEW_NAV_ERR_SECURITY:
        e = "wxWEBVIEW_NAV_ERR_SECURITY";
        break;
      case wxWEBVIEW_NAV_ERR_NOT_FOUND:
        e = "wxWEBVIEW_NAV_ERR_NOT_FOUND";
        break;
      case wxWEBVIEW_NAV_ERR_REQUEST:
        e = "wxWEBVIEW_NAV_ERR_REQUEST";
        break;
      case wxWEBVIEW_NAV_ERR_USER_CANCELLED:
        e = "wxWEBVIEW_NAV_ERR_USER_CANCELLED";
        break;
      case wxWEBVIEW_NAV_ERR_OTHER:
        e = "wxWEBVIEW_NAV_ERR_OTHER";
        break;
      }
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__<< boost::format(": error loading page %1% %2% %3% %4%") %evt.GetURL() %evt.GetTarget() %e %evt.GetString();
}

void PrinterMgrView::OnLoaded(wxWebViewEvent &evt)
{
    if (evt.GetURL().IsEmpty())
        return;
    SendAPIKey();

    DM::DeviceMgr::Ins().Load();
    AccountDeviceMgr::getInstance().load();

}

void PrinterMgrView::OnScriptMessage(wxWebViewEvent& evt)
{
    try {
        
        if (DM::AppMgr::Ins().Invoke(m_browser, evt.GetString().ToUTF8().data()))
        {
            return;
        }

        wxString strInput = evt.GetString();
        BOOST_LOG_TRIVIAL(trace) << "DeviceDialog::OnScriptMessage;OnRecv:" << strInput.c_str();
        json     j        = json::parse(strInput);

        wxString strCmd = j["command"];
        BOOST_LOG_TRIVIAL(trace) << "DeviceDialog::OnScriptMessage;Command:" << strCmd;

        if (strCmd == "send_gcode") {
            int plateIndex = j["plateIndex"];
            wxString ipAddress  = j["ipAddress"];
            wxString uploadName = j["uploadName"];

            PartPlate* plate = wxGetApp().plater()->get_partplate_list().get_plate(plateIndex);
            if (plate) {
                std::string gcodeFilePath = plate->get_tmp_gcode_path();
                if (wxGetApp().plater()->only_gcode_mode()) {
                    gcodeFilePath = wxGetApp().plater()->get_last_loaded_gcode().ToStdString();
                }

                RemotePrint::RemotePrinterManager::getInstance().pushUploadTasks(ipAddress.ToStdString(), uploadName.ToStdString(), gcodeFilePath,
                    [this](std::string ip, float progress) {
                        // BOOST_LOG_TRIVIAL(info) << "Progress: " << progress << "% for IP: " << ip;

                        wxString jsCode = wxString::Format("window.updateProgress('%s', %f);", ip, progress);

                        wxTheApp->CallAfter([this, jsCode]() {
                            if (m_browser->IsBusy()) {
                                BOOST_LOG_TRIVIAL(trace) << "Browser is busy, delaying script execution.";
                                // Optionally, you can queue the script to run later
                            } else {
                                bool result = WebView::RunScript(m_browser, jsCode);
                                BOOST_LOG_TRIVIAL(trace) << "RunScript result: " << result;
                            }
                        });
                    },
                    [this](std::string ip, int statusCode) {

                        wxString statusJsCode = wxString::Format("window.uploadStatus('%s', %d);", ip, statusCode);

                        wxTheApp->CallAfter([this, statusJsCode]() {
                            if (m_browser->IsBusy()) {
                                BOOST_LOG_TRIVIAL(trace) << "Browser is busy, delaying script execution.";
                                // Optionally, you can queue the script to run later
                            } else {
                                bool result = WebView::RunScript(m_browser, statusJsCode);
                                BOOST_LOG_TRIVIAL(trace) << "RunScript result: " << result;
                            }
                        });
                    }
                );
            }
        }
        else if(strCmd == "down_file")
        {
           std::string url = j["url"];
           std::string name = j["name"];
           std::string file_type = j["file_type"];

           this->down_file(url, name, file_type);
        }
        else if (strCmd == "scan_device")
        {
            scan_device();
        }
        
        else if (m_commandHandlers.find(strCmd.ToStdString()) != m_commandHandlers.end()) {
            m_commandHandlers[strCmd.ToStdString()](j);
        }
        else if(strCmd == "req_device_move_direction")
        {
            std::string presetName = j["preset_name"];
            Preset*     preset = Slic3r::GUI::wxGetApp().preset_bundle->printers.find_preset(presetName);
            if (preset != nullptr && preset->config.has("machine_platform_motion_enable")) {
                const ConfigOption* option = preset->config.option("machine_platform_motion_enable");
                bool b = option->getBool();
                int                 i      = b ? 1 : 0;
                nlohmann::json commandJson;
                commandJson["command"] = "req_device_move_direction";
                commandJson["data"]    = i;
                wxString strJS = wxString::Format("window.handleStudioCmd('%s');", commandJson.dump());
                run_script(strJS.ToStdString());
            }
        }
        else {
            BOOST_LOG_TRIVIAL(trace) << "PrinterMgrView::OnScriptMessage;Unknown Command:" << strCmd;
        }
        

    } catch (std::exception &e) {
       // wxMessageBox(e.what(), "json Exception", MB_OK);
        BOOST_LOG_TRIVIAL(trace) << "DeviceDialog::OnScriptMessage;Error:" << e.what();
    }

}

std::string PrinterMgrView::get_plate_data_on_show()
{
    nlohmann::json json_array = nlohmann::json::array();

    std::vector<std::string> extruder_colors = Slic3r::GUI::wxGetApp().plater()->get_extruder_colors_from_plater_config();
    nlohmann::json           colors_json     = nlohmann::json::array();
    for (const auto& color : extruder_colors) {
        colors_json.push_back(color);
    }

    std::vector<std::string> filament_presets = wxGetApp().preset_bundle->filament_presets;

    std::vector<std::string> filament_types;
    nlohmann::json filament_types_json = nlohmann::json::array();

    for (const auto& preset_name : filament_presets) {
        std::string     filament_type;
        Slic3r::Preset* preset = wxGetApp().preset_bundle->filaments.find_preset(preset_name);
        if (preset) {
            preset->get_filament_type(filament_type);
            filament_types_json.push_back(filament_type);
            filament_types.emplace_back(filament_type);
        }
    }

    for (int i = 0; i < wxGetApp().plater()->get_partplate_list().get_plate_count(); i++) {
        PartPlate* plate = wxGetApp().plater()->get_partplate_list().get_plate(i);
        if (plate && !plate->empty() && plate->is_slice_result_valid() && plate->is_slice_result_ready_for_print() && plate->thumbnail_data.is_valid()) {
            wxImage image(plate->thumbnail_data.width, plate->thumbnail_data.height);
            image.InitAlpha();
            for (unsigned int r = 0; r < plate->thumbnail_data.height; ++r) {
                unsigned int rr = (plate->thumbnail_data.height - 1 - r) * plate->thumbnail_data.width;
                for (unsigned int c = 0; c < plate->thumbnail_data.width; ++c) {
                    unsigned char* px = (unsigned char*) plate->thumbnail_data.pixels.data() + 4 * (rr + c);
                    image.SetRGB((int) c, (int) r, px[0], px[1], px[2]);
                    image.SetAlpha((int) c, (int) r, px[3]);
                }
            }

            wxImage resized_image = image.Rescale(50, 50, wxIMAGE_QUALITY_HIGH);

            wxMemoryOutputStream mem_stream;
            if (!resized_image.SaveFile(mem_stream, wxBITMAP_TYPE_PNG)) {
                wxLogError("Failed to save image to memory stream.");
            }

            auto size = mem_stream.GetSize();
            // wxMemoryBuffer buffer(size);
            auto imgdata = new unsigned char[size];
            mem_stream.CopyTo(imgdata, size);

            std::size_t encoded_size = boost::beast::detail::base64::encoded_size(size);
            std::string img_base64_data(encoded_size, '\0');
            boost::beast::detail::base64::encode(&img_base64_data[0], imgdata, size);

            std::string default_gcode_name = "";

            std::vector<int> plate_extruders = plate->get_extruders(true);

            if (Slic3r::GUI::wxGetApp().plater()->only_gcode_mode()) {
                wxString   last_loaded_gcode = Slic3r::GUI::wxGetApp().plater()->get_last_loaded_gcode();
                wxFileName fileName(last_loaded_gcode);
                default_gcode_name = std::string(fileName.GetName().ToUTF8().data()) + ".gcode";
                plate_extruders    = Slic3r::GUI::wxGetApp().plater()->get_gcode_extruders_in_only_gcode_mode();
            }
            else {
                // {m_pathname=L"Cone_PLA_10m45s.gcode" }
                ModelObjectPtrs plate_objects = plate->get_objects_on_this_plate();
                std::string     obj0_name     = ""; // the first object's name
                if (plate_objects.size() > 0 && nullptr != plate_objects[0]) {
                    obj0_name = plate_objects[0]->name;
                }

                auto                                  plate_print_statistics = plate->get_slice_result()->print_statistics;
                const PrintEstimatedStatistics::Mode& plate_time_mode =
                    plate_print_statistics.modes[static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Normal)];

                if (plate_extruders.size() > 0) {
                    default_gcode_name = obj0_name + "_" + filament_types[plate_extruders[0] - 1] + "_" +
                                         get_bbl_time_dhms(plate_time_mode.time) + ".gcode";
                } else {
                    default_gcode_name = "plate" + std::to_string(i + 1) + ".gcode";
                }
            }

            nlohmann::json json_data;
            json_data["image"]              = "data:image/png;base64," + std::move(img_base64_data);
            json_data["plate_index"]        = plate->get_index();
            json_data["upload_gcode__name"] = std::move(default_gcode_name);

            nlohmann::json extruders_json = nlohmann::json::array();
            for (const auto& extruder : plate_extruders) {
                extruders_json.push_back(extruder);
            }
            json_data["plate_extruders"] = extruders_json;

            json_array.push_back(json_data);
        }
    }

    nlohmann::json top_level_json;
    top_level_json["extruder_colors"] = std::move(colors_json);
    top_level_json["filament_types"]    = std::move(filament_types_json);
    top_level_json["plates"]          = std::move(json_array);

    std::string json_str         = top_level_json.dump(-1, ' ', true);

    // create command to send to the webview
    nlohmann::json commandJson;
    commandJson["command"] = "update_plate_data";
    commandJson["data"]    = RemotePrint::Utils::url_encode(json_str);

    std::string commandStr = commandJson.dump(-1, ' ', true);

    return RemotePrint::Utils::url_encode(commandStr);

}

bool PrinterMgrView::Show(bool show) 
{
    bool result = wxPanel::Show(show);

    if (show) {
        wxString strJS = wxString::Format("window.handleStudioCmd('%s');", get_plate_data_on_show());
        run_script(strJS.ToStdString());
    }

    return result;
}

void PrinterMgrView::run_script(std::string content)
{
    WebView::RunScript(m_browser, content);
}

void PrinterMgrView::down_file(std::string download_info, std::string filename, std::string path_type )
{
    FileType file_type = FT_GCODE;
    wxStandardPaths& stdPaths = wxStandardPaths::Get();
    wxString docsDir = stdPaths.GetDocumentsDir();
    if(path_type == "Videos"){
        docsDir = stdPaths.GetUserDir(wxStandardPaths::Dir_Videos);
        file_type  =FT_VIDEO;
    }

    fs::path output_file(filename);
    fs::path output_path;
    wxString extension = output_file.extension().string();
    
    std::string ext = "";
    
    wxFileDialog dlg(this, _L(""), docsDir, from_path(output_file.filename()),
        GUI::file_wildcards(file_type, ext), wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxPD_APP_MODAL);

//     wxFileDialog dlg(this, _L(""),
//         docsDir.ToStdString(),
//         from_path(output_file.filename()),
//         GUI::file_wildcards(file_type, ext),
//         wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxPD_APP_MODAL
//     );

    if (dlg.ShowModal() != wxID_OK) {
        return;
    }

    output_path = into_path(dlg.GetPath());
    output_file = into_path(dlg.GetFilename());

    wxString download_url(download_info.c_str());

    bool download_ok = false;
    int retry_count = 0;
    const int max_retries = 3;

    bool cont = true;
    bool cancel = false;

    int percent = 0;
    boost::filesystem::path target_path = output_path;
    boost::thread import_thread = Slic3r::create_thread([&percent, &cont, &cancel, &retry_count, max_retries, &extension, &target_path, &download_ok, download_url, this] {

        fs::path tmp_path = target_path;
        tmp_path += wxSecretString::Format("%s", ".download");

        auto filesize = 0;
        bool size_limit = false;
        auto http = Http::get(download_url.ToStdString());

        while (cont && retry_count < max_retries) {
            retry_count++;
            http.on_progress([&percent, &cont, &filesize, &size_limit, this](Http::Progress progress, bool& cancel) {

                if (!cont) cancel = true;
                if (progress.dltotal != 0) {

                    if (filesize == 0) {
                        filesize = progress.dltotal;
                        double megabytes = static_cast<double>(progress.dltotal) / (1024 * 1024);
                        //The maximum size of a 3mf file is 500mb
                        if (megabytes > 500) {
                            cont = false;
                            size_limit = true;
                        }
                    }
                    percent = progress.dlnow * 100 / progress.dltotal;

                    nlohmann::json commandJson;
                    commandJson["command"] = "update_download_progress";
                    commandJson["data"] = percent;

                    wxString strJS = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandJson.dump()));
                    wxGetApp().CallAfter([this, strJS] { run_script(strJS.ToStdString()); });
                }
                })
                .on_error([&cont, &retry_count, max_retries](std::string body, std::string error, unsigned http_status) {
                    (void)body;
                    BOOST_LOG_TRIVIAL(error) << wxSecretString::Format("Error getting: `%1%`: HTTP %2%, %3%",
                        body,
                        http_status,
                        error);

                    if (retry_count == max_retries) {
                       
                        cont = false;
                    }
                    })
                    .on_complete([&cont, &download_ok, tmp_path, &target_path, extension](std::string body, unsigned /* http_status */) {
                        fs::fstream file(tmp_path, std::ios::out | std::ios::binary | std::ios::trunc);
                        file.write(body.c_str(), body.size());
                        file.close();
                        
                        fs::rename(tmp_path, target_path);
            
                        cont = false;
                        download_ok = true;
                        }).perform_sync();
        }

        });

    while (cont) {
        wxEventLoopBase::GetActive()->YieldFor(wxEVT_CATEGORY_UI | wxEVT_CATEGORY_USER_INPUT);
        if (download_ok)
            break;
    }

    if (import_thread.joinable())
        import_thread.join();

}

void PrinterMgrView::scan_device() 
{
    boost::thread _thread = Slic3r::create_thread(
        [this] {
             
            std::vector<std::string> prefix;
            prefix.push_back("CXSWBox");
            prefix.push_back("creality");
            prefix.push_back("Creality");
            std::vector<std::string> vtIp;
            auto vtDevice = cxnet::syncDiscoveryService(prefix);
            for (auto& item : vtDevice) {
                vtIp.push_back(item.machineIp);
            }

            nlohmann::json commandJson;
            commandJson["command"] = "scan_device";
            commandJson["data"]    = vtIp;

            wxString strJS = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandJson.dump()));           
            wxGetApp().CallAfter([this, strJS] { run_script(strJS.ToStdString()); });            
        });
 

    if (_thread.joinable())
        _thread.join();
}

void     PrinterMgrView::RequestDeviceListFromDB() 
{
    // create command to send to the webview
    nlohmann::json commandJson;
    commandJson["command"] = "req_device_list";
    commandJson["data"]    = "";

    wxString strJS = wxString::Format("window.handleStudioCmd('%s');", commandJson.dump());

    run_script(strJS.ToStdString());
}

void PrinterMgrView::RegisterHandler(const std::string& command, std::function<void(const nlohmann::json&)> handler)
{
    m_commandHandlers[command] = handler;
}
void PrinterMgrView::UnregisterHandler(const std::string& command)
{
    m_commandHandlers.erase(command);
}
void PrinterMgrView::ExecuteScriptCommand(const std::string& commandInfo, bool async)
{
    if (commandInfo.empty())
        return;
    
    wxString strJS = wxString::Format("window.handleStudioCmd('%s');", commandInfo);

    if (async)
    {
        wxGetApp().CallAfter([this, strJS] { run_script(strJS.ToStdString()); });
    }
    else 
    {
        run_script(strJS.ToStdString());
    }

}

void PrinterMgrView::handle_request_update_device_relate_to_account(const nlohmann::json& json_data)
{
    if (!Slic3r::GUI::wxGetApp().is_login()) {
        return;
    }

    // the account_device_info is possibly empty, if the account is not binded to any printer
    std::string account_device_info = AccountDeviceMgr::getInstance().get_account_device_bind_json_info();

    // Create top-level JSON object
    nlohmann::json top_level_json = {
        {"accout_binded_devices", account_device_info}
    };

    // Create command JSON object
    nlohmann::json commandJson = {
        {"command", "update_device_relate_account_state"},
        {"data", top_level_json.dump()}
    };

    ExecuteScriptCommand(RemotePrint::Utils::url_encode(commandJson.dump()));
}

void PrinterMgrView::handle_set_device_relate_to_account(const nlohmann::json& json_data)
{
    try {
        std::string ipAddress   = json_data["ipAddress"];
        std::string device_mac  = json_data["device_mac"];
        std::string device_name = json_data["device_name"];
        bool        relateState = json_data["relateState"];

        std::string printerGroup = json_data["device_group"];
        std::string device_model = json_data["device_model"];

        int connectType = 0;
        if(json_data.contains("type"))
        {
            connectType = json_data["type"].get<int>();
        }

        if(relateState) {
            AccountDeviceMgr::DeviceInfo device_info;
            device_info.device_unique_id = device_mac;
            device_info.address          = ipAddress;
            device_info.mac              = device_mac;
            device_info.model            = device_model;
            device_info.connectType      = connectType;
            device_info.group            = printerGroup;

            AccountDeviceMgr::getInstance().add_to_my_devices(device_info);
        }
        else {
            AccountDeviceMgr::getInstance().unbind_device(device_mac);
        }
    } catch (std::exception& e) {

    }
}

void PrinterMgrView::request_refresh_all_device()
{
    // Create command JSON object
    nlohmann::json commandJson = {
        {"command", "refresh_all_device"},
        {"data", ""}
    };

    ExecuteScriptCommand(commandJson.dump());
}

void PrinterMgrView::forward_init_device_cmd_to_printer_list()
{
    if (!Slic3r::GUI::wxGetApp().is_login()) {
        return;
    }

    try {
        std::string reload_printer_info = AccountDeviceMgr::getInstance().get_account_device_info_for_printers_init();

        std::string account_device_info = AccountDeviceMgr::getInstance().get_account_device_bind_json_info();

        // Create top-level JSON object
        nlohmann::json top_level_json = {
            {"related_device_info", reload_printer_info},
            {"accout_binded_devices", account_device_info}
        };

        // Create command JSON object
        nlohmann::json commandJson = {
            {"command", "reinit_related_to_account_device"},
            {"data", top_level_json.dump()}
        };

        WebView::RunScript(m_browser, wxString::Format("window.handleStudioCmd('%s')", RemotePrint::Utils::url_encode(commandJson.dump())));
    }
    catch (const std::exception& e) {
        std::cout << "forward_init_device_cmd_to_printer_list failed" << std::endl;
    }

}

} // GUI
} // Slic3r
