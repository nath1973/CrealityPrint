#include "GUI_Init.hpp"

#include "libslic3r/AppConfig.hpp"

#include "slic3r/GUI/GUI.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/3DScene.hpp"
#include "slic3r/GUI/InstanceCheck.hpp"
#include "slic3r/GUI/format.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "slic3r/GUI/Plater.hpp"

// To show a message box if GUI initialization ends up with an exception thrown.
#include <wx/msgdlg.h>

#include <boost/nowide/iostream.hpp>
#include <boost/nowide/convert.hpp>

#if __APPLE__
    #include <signal.h>
#endif // __APPLE__
#ifdef TARGET_OS_MAC
    //#ifdef USE_BREAKPAD
    #include "client/mac/handler/exception_handler.h"
    //#endif
#endif
namespace Slic3r {
namespace GUI {

int GUI_Run(GUI_InitParams &params)
{
#if __APPLE__
    // On OSX, we use boost::process::spawn() to launch new instances of PrusaSlicer from another PrusaSlicer.
    // boost::process::spawn() sets SIGCHLD to SIGIGN for the child process, thus if a child PrusaSlicer spawns another
    // subprocess and the subrocess dies, the child PrusaSlicer will not receive information on end of subprocess
    // (posix waitpid() call will always fail).
    // https://jmmv.dev/2008/10/boostprocess-and-sigchld.html
    // The child instance of PrusaSlicer has to reset SIGCHLD to its default, so that posix waitpid() and similar continue to work.
    // See GH issue #5507
    signal(SIGCHLD, SIG_DFL);
#endif // __APPLE__

    //BBS: remove the try-catch and let exception goto above
    try {
        //GUI::GUI_App* gui = new GUI::GUI_App(params.start_as_gcodeviewer ? GUI::GUI_App::EAppMode::GCodeViewer : GUI::GUI_App::EAppMode::Editor);
        bool enable_test = false;
        if (params.argc >= 2 && params.argv != nullptr && std::string(params.argv[1]) == std::string("test#157369"))
            enable_test = true;
        GUI::GUI_App* gui = new GUI::GUI_App(enable_test);
        //if (gui->get_app_mode() != GUI::GUI_App::EAppMode::GCodeViewer) {
            // G-code viewer is currently not performing instance check, a new G-code viewer is started every time.
            bool gui_single_instance_setting = gui->app_config->get("app", "single_instance") == "true";
            if (Slic3r::instance_check(params.argc, params.argv, gui_single_instance_setting)) {
                //TODO: do we have delete gui and other stuff?
                return -1;
            }
        //}

//      gui->autosave = m_config.opt_string("autosave");
        GUI::GUI_App::SetInstance(gui);
        gui->init_params = &params;
        
        #ifdef TARGET_OS_MAC
            boost::filesystem::path   tempPath = boost::filesystem::path(wxFileName::GetTempDir().ToStdString());
            auto dmpCallBack = [](const char* dump_dir,
                                   const char* minidump_id,
                                   void* context, bool succeeded) -> bool {
            //wxMessageBox(dump_dir,wxT("Message box caption"),wxNO_DEFAULT|wxYES_NO|wxCANCEL|wxICON_INFORMATION,nullptr);
                if (succeeded) {
                    boost::filesystem::path oldPath(dump_dir);
                    oldPath.append(minidump_id).replace_extension(".dmp");

                    boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
                    // 创建一个 time_facet 对象，用于自定义时间格式
                    boost::posix_time::time_facet* timeFacet = new boost::posix_time::time_facet();
                    std::stringstream              ss;
                    // 设置时间格式为 yyyyMMDD_hhmmss
                    timeFacet->format("%Y%m%d_%H%M%S");
                    ss.imbue(std::locale(std::locale::classic(), timeFacet));
                    ss << now;

                    std::string timeStr        = ss.str();
                    std::string processNameStr = timeStr + std::string("_") + SLIC3R_PROCESS_NAME + std::string("_") + CREALITYPRINT_VERSION +
                                                std::string("_") + PROJECT_VERSION_EXTRA;
                    boost::filesystem::path newPath(dump_dir);
                    newPath.append(processNameStr).replace_extension(".dmp");
                    if (boost::filesystem::exists(oldPath)) {
                        // MessageBox(NULL, newPath.wstring().c_str(), minidump_id, MB_OK | MB_ICONINFORMATION);
                        boost::filesystem::rename(oldPath, newPath);
                        #ifdef TARGET_OS_MAC
                        // 获取当前可执行文件路径
                        wxString exePath = wxStandardPaths::Get().GetExecutablePath();
                        // 提取 .app 包路径
                        wxString appBundlePath;
                        size_t   pos = exePath.rfind(wxT("/Contents/MacOS/"));
                        if (pos != wxString::npos) {
                            appBundlePath = exePath.substr(0, pos);
                        }
                        wxString command = "open -a %s --args \"minidump://file=%s\"";
                        command          = command.Format(command, appBundlePath, newPath.string().c_str());
                        wxExecute(command, wxEXEC_ASYNC);
                        //wxMessageBox(command,wxT("Message box caption"),wxNO_DEFAULT|wxYES_NO|wxCANCEL|wxICON_INFORMATION,nullptr);
                        #endif
                    }

                    
                } 
                return true;
            };
            //#ifdef USE_BREAKPAD
            google_breakpad::ExceptionHandler eh(tempPath.string(), NULL, dmpCallBack, NULL, true, NULL);
            //#endif
        #endif
        if (params.argc > 1) {
            // STUDIO-273 wxWidgets report error when opening some files with specific names
            // wxWidgets does not handle parameters, so intercept parameters here, only keep the app name
            int                 argc = 1;
            std::vector<char *> argv;
            argv.push_back(params.argv[0]);
            return wxEntry(argc, argv.data());
        } else {
            return wxEntry(params.argc, params.argv);
        }
    } catch (const Slic3r::Exception &ex) {
        BOOST_LOG_TRIVIAL(error) << ex.what() << std::endl;
        wxMessageBox(boost::nowide::widen(ex.what()), _L("Creality Print GUI initialization failed"), wxICON_STOP);
    } catch (const std::exception &ex) {
        BOOST_LOG_TRIVIAL(error) << ex.what() << std::endl;
        wxMessageBox(format_wxstr(_L("Fatal error, exception catched: %1%"), ex.what()), _L("Creality Print GUI initialization failed"), wxICON_STOP);
    }
    // error
    return 1;
}
}
}
