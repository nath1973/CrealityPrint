#include "PrinterPresetConfig.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/Preset.hpp"
#include "libslic3r/PresetBundle.hpp"

#include "slic3r/GUI/GUI.hpp"
#include "GUI_App.hpp"
#include "MainFrame.hpp"

namespace Slic3r { namespace GUI {
struct AreaInfo
{
    std::string strModelName;
    std::string strAreaInfo;
    std::string strHeightInfo;
};

static void GetPrinterArea(json& pm, std::map<string, AreaInfo>& mapInfo)
{
    AreaInfo areaInfo;
    areaInfo.strModelName  = "";
    areaInfo.strAreaInfo   = "";
    areaInfo.strHeightInfo = "";

    string strInherits = "";
    string strName     = "";

    if (pm.contains("name")) {
        strName = pm["name"];
    }

    if (pm.contains("printer_model")) {
        areaInfo.strModelName = pm["printer_model"];
    }

    if (pm.contains("inherits")) {
        strInherits = pm["inherits"];
    }

    if (pm.contains("printable_height")) {
        areaInfo.strHeightInfo = pm["printable_height"];
    } else {
        auto it = mapInfo.find(strInherits);
        if (it != mapInfo.end()) {
            areaInfo.strHeightInfo = it->second.strHeightInfo;
        }
    }

    if (pm.contains("printable_area")) {
        string pt0 = "";
        string pt2 = "";
        if (pm["printable_area"].is_array()) {
            if (pm["printable_area"].size() < 5) {
                pt0 = pm["printable_area"][0];
                pt2 = pm["printable_area"][2];
            } else {
                std::vector<Vec2d> vecPt;
                int                size = pm["printable_area"].size();
                for (int i = 0; i < size; i++) {
                    std::string point_str = pm["printable_area"][i].get<std::string>();
                    size_t      pos       = point_str.find('x');
                    double      x         = std::stod(point_str.substr(0, pos));
                    double      y         = std::stod(point_str.substr(pos + 1));
                    vecPt.push_back(Vec2d(x, y));
                }

                Geometry::Circled circle = Geometry::circle_ransac(vecPt);
                double            dRad   = scaled<double>(circle.radius);
                int               dDim   = (int) ((2. * unscaled<double>(dRad)) + 0.1);
                areaInfo.strAreaInfo     = std::to_string(dDim) + "*" + std::to_string(dDim);
            }
        } else if (pm["printable_area"].is_string()) {
            string              printable_area_str = pm["printable_area"];
            std::vector<string> points;
            size_t              start = 0;
            size_t              end   = printable_area_str.find(',');
            while (end != std::string::npos) {
                points.push_back(printable_area_str.substr(start, end - start));
                start = end + 1;
                end   = printable_area_str.find(',', start);
            }
            points.push_back(printable_area_str.substr(start));

            if (points.size() < 5) {
                pt0 = points[0];
                pt2 = points[2];
            }
        }

        if (!pt0.empty() && !pt2.empty()) {
            size_t pos0   = pt0.find('x');
            size_t pos2   = pt2.find('x');
            int    pt0_x  = std::stoi(pt0.substr(0, pos0));
            int    pt0_y  = std::stoi(pt0.substr(pos0 + 1));
            int    pt2_x  = std::stoi(pt2.substr(0, pos2));
            int    pt2_y  = std::stoi(pt2.substr(pos2 + 1));
            int    length = pt2_x - pt0_x;
            int    width  = pt2_y - pt0_y;

            if ((length > 0) && (width > 0)) {
                areaInfo.strAreaInfo = std::to_string(length) + "*" + std::to_string(width);
            }
        }
    } else {
        auto it = mapInfo.find(strInherits);
        if (it != mapInfo.end()) {
            areaInfo.strAreaInfo = it->second.strAreaInfo;
        }
    }

    mapInfo[strName] = areaInfo;
}

static void StringReplace(string& strBase, string strSrc, string strDes)
{
    string::size_type pos    = 0;
    string::size_type srcLen = strSrc.size();
    string::size_type desLen = strDes.size();
    pos                      = strBase.find(strSrc, pos);
    while ((pos != string::npos)) {
        strBase.replace(pos, srcLen, strDes);
        pos = strBase.find(strSrc, (pos + desLen));
    }
}

struct PrinterInfo
{
    std::string name;
    std::string seriesNameList;
};

static bool toLowerAndContains(const std::string& str, const std::string& key)
{
    std::string lowerStr = str;
    std::string lowerKey = key;

    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);

    return lowerStr.find(lowerKey) != std::string::npos;
}

static bool customComparator(const PrinterInfo& a, const PrinterInfo& b)
{
    static const std::vector<std::string> order = {"flagship", "ender", "cr", "halot"};

    auto getPriority = [](const std::string& name) {
        for (size_t i = 0; i < order.size(); ++i) {
            if (toLowerAndContains(name, order[i])) {
                return i; // �������ȼ�����
            }
        }
        return order.size(); // �����������������������ȼ�
    };

    return getPriority(a.name) < getPriority(b.name);
}

PrinterPresetConfig::PrinterPresetConfig() {}

PrinterPresetConfig::~PrinterPresetConfig() {}

std::vector<std::string> PrinterPresetConfig::getFilament(const std::string& printerName, const std::string& nozzle)
{
    std::vector<std::string> vtFileName;
    /*fs::path filamentPath = fs::path(resources_dir()).append("profiles").append("Creality").append("filament");
    for (auto& entry : fs::directory_iterator(filamentPath)) {
        std::string fileName = entry.path().filename().string();
        if (fileName.find(printerName) != std::string::npos) {
            vtFileName.push_back(fileName);
        }
    }
    */
    fs::path filamentPath = fs::path(resources_dir()).append("profiles").append("Creality.json");
    std::string filamentContent;
    LoadFile(filamentPath.string(), filamentContent);
    json jFilament = json::parse(filamentContent);

    fs::path printerPath = fs::path(resources_dir()).append("profiles").append("Creality").append("machine").append(printerName+".json");
    std::string printerContent;
    LoadFile(printerPath.string(), printerContent);
    json jPrinter = json::parse(printerContent);
    std::string ssDefaultFilament;
    if (jPrinter.contains("default_materials") && jPrinter["default_materials"].is_string()) {
        ssDefaultFilament = jPrinter["default_materials"];
    }
    
    std::string ssPrinter = printerName + " " + nozzle + " nozzle";
    if (jFilament.contains("filament_list") && jFilament["filament_list"].is_array()) {
        for (auto& filament : jFilament["filament_list"]) {
            if (filament.contains("name") && filament["name"].is_string()) {
                std::string fName = filament["name"];
                if (fName.find(ssPrinter) == std::string::npos)
                    continue;
                size_t pos   = fName.find(" @");
                if (pos != std::string::npos) {
                    fName = fName.substr(0, pos);
                    if (fName.length() > 0 && fName.c_str()[fName.length() - 1] == ' ') {
                        fName = fName.substr(0, pos - 1);
                    }
                    if (ssDefaultFilament.find(fName) != std::string::npos) {
                        vtFileName.push_back(filament["name"]);
                    }
                }
            }
        }
    }
    return vtFileName; 
}

int PrinterPresetConfig::LoadProfile() { 
    try {
        m_ProfileJson = json::parse("{}");
        m_ProfileJson["model"]    = json::array();
        m_ProfileJson["machine"]  = json::object();
        m_ProfileJson["filament"] = json::object();
        m_ProfileJson["process"]  = json::array();

        vendor_dir      = (boost::filesystem::path(Slic3r::data_dir()) / PRESET_SYSTEM_DIR).make_preferred();
        rsrc_vendor_dir = (boost::filesystem::path(resources_dir()) / "profiles").make_preferred();

        // BBS: add BBL as default
        // BBS: add json logic for vendor bundle
        auto bbl_bundle_path = vendor_dir;
        bbl_bundle_rsrc      = false;
        if (!boost::filesystem::exists((vendor_dir / PresetBundle::BBL_BUNDLE).replace_extension(".json"))) {
            bbl_bundle_path = rsrc_vendor_dir;
            bbl_bundle_rsrc = true;
        }

        m_MachineJson                       = json::parse("{}");
        m_MachineJson["machine"]            = json::array();
        boost::filesystem::path machinepath = vendor_dir;
        if (!boost::filesystem::exists((vendor_dir / "Creality" / "machineList").replace_extension(".json"))) {
            machinepath = rsrc_vendor_dir;
        }
        LoadMachineJson("machineList", (machinepath / "Creality" / "machineList.json").string());

        // load BBL bundle from user data path
        string                                targetPath = bbl_bundle_path.make_preferred().string();
        boost::filesystem::path               myPath(targetPath);
        boost::filesystem::directory_iterator endIter;
        for (boost::filesystem::directory_iterator iter(myPath); iter != endIter; iter++) {
            if (boost::filesystem::is_directory(*iter)) {
                // cout << "is dir" << endl;
                // cout << iter->path().string() << endl;
            } else {
                // cout << "is a file" << endl;
                // cout << iter->path().string() << endl;

                wxString strVendor    = from_u8(iter->path().string()).BeforeLast('.');
                strVendor             = strVendor.AfterLast('\\');
                strVendor             = strVendor.AfterLast('\/');
                wxString strExtension = from_u8(iter->path().string()).AfterLast('.').Lower();

                if (strVendor.ToStdString() == PresetBundle::BBL_BUNDLE && strExtension.CmpNoCase("json") == 0)
                    LoadProfileFamily(strVendor.ToStdString(), iter->path().string());
            }
        }

        // string                                others_targetPath = rsrc_vendor_dir.string();
        boost::filesystem::directory_iterator others_endIter;
        for (boost::filesystem::directory_iterator iter(rsrc_vendor_dir); iter != others_endIter; iter++) {
            if (boost::filesystem::is_directory(*iter)) {
                // cout << "is dir" << endl;
                // cout << iter->path().string() << endl;
            } else {
                // cout << "is a file" << endl;
                // cout << iter->path().string() << endl;
                wxString strVendor    = from_u8(iter->path().string()).BeforeLast('.');
                strVendor             = strVendor.AfterLast('\\');
                strVendor             = strVendor.AfterLast('\/');
                wxString strExtension = from_u8(iter->path().string()).AfterLast('.').Lower();

                if (strVendor.ToStdString() != PresetBundle::BBL_BUNDLE && strExtension.CmpNoCase("json") == 0)
                    LoadProfileFamily(strVendor.ToStdString(), iter->path().string());
            }
        }

        const auto enabled_filaments = wxGetApp().app_config->has_section(AppConfig::SECTION_FILAMENTS) ?
                                                                wxGetApp().app_config->get_section(AppConfig::SECTION_FILAMENTS) :
                                                                std::map<std::string, std::string>();
        m_appconfig_new.set_vendors(*wxGetApp().app_config);
        m_appconfig_new.set_section(AppConfig::SECTION_FILAMENTS, enabled_filaments);

        for (auto it = m_ProfileJson["model"].begin(); it != m_ProfileJson["model"].end(); ++it) {
            if (it.value().is_object()) {
                json&       temp_model      = it.value();
                std::string model_name      = temp_model["model"];
                std::string vendor_name     = temp_model["vendor"];
                std::string nozzle_diameter = temp_model["nozzle_diameter"];
                std::string selected;
                boost::trim(nozzle_diameter);
                std::string nozzle;
                bool        enabled = false, first = true;
                while (nozzle_diameter.size() > 0) {
                    auto pos = nozzle_diameter.find(';');
                    if (pos != std::string::npos) {
                        nozzle  = nozzle_diameter.substr(0, pos);
                        enabled = m_appconfig_new.get_variant(vendor_name, model_name, nozzle);
                        if (enabled) {
                            if (!first)
                                selected += ";";
                            selected += nozzle;
                            first = false;
                        }
                        nozzle_diameter = nozzle_diameter.substr(pos + 1);
                        boost::trim(nozzle_diameter);
                    } else {
                        enabled = m_appconfig_new.get_variant(vendor_name, model_name, nozzle_diameter);
                        if (enabled) {
                            if (!first)
                                selected += ";";
                            selected += nozzle_diameter;
                        }
                        break;
                    }
                }
                temp_model["nozzle_selected"] = selected;
                // m_ProfileJson["model"][a]["nozzle_selected"]
            }
        }

        if (m_ProfileJson["model"].size() == 1) {
            std::string strNozzle                        = m_ProfileJson["model"][0]["nozzle_diameter"];
            m_ProfileJson["model"][0]["nozzle_selected"] = strNozzle;
        }

        for (auto it = m_ProfileJson["filament"].begin(); it != m_ProfileJson["filament"].end(); ++it) {
            // json temp_filament = it.value();
            std::string filament_name = it.key();
            if (enabled_filaments.find(filament_name) != enabled_filaments.end())
                m_ProfileJson["filament"][filament_name]["selected"] = 1;
        }

        //----region
        m_Region = wxGetApp().app_config->get("region");
        if (m_Region.empty()) {
            wxString strlang = wxGetApp().current_language_code_safe();
            if (strlang == "zh_CN") {
                m_Region = "China";
            } else {
                m_Region = "North America";
            }
        }
        m_ProfileJson["region"] = m_Region;

        m_ProfileJson["network_plugin_install"]     = wxGetApp().app_config->get("app", "installed_networking");
        m_ProfileJson["network_plugin_compability"] = wxGetApp().is_compatibility_version() ? "1" : "0";
        network_plugin_ready                        = wxGetApp().is_compatibility_version();
        // write regin into app_config
        wxGetApp().app_config->set("region", m_Region);
        {
            // update the webview region
            std::vector<wxString> prefs;

            wxString    strlang           = wxGetApp().current_language_code_safe();
            std::string country_code      = m_Region;
            std::string use_inches        = wxGetApp().app_config->get("use_inches");
            std::string syncPresetEnabled = wxGetApp().app_config->get("sync_user_preset") == "true" ? "1" : "0";

            prefs.push_back(strlang);
            prefs.push_back(wxString(country_code));
            prefs.push_back(wxString(use_inches));
            prefs.push_back(wxString(syncPresetEnabled));

            if (wxGetApp().mainframe->m_webview) {
                wxGetApp().mainframe->m_webview->sync_preferences(prefs);
            }
        }
    } catch (std::exception& e) {
        // wxLogMessage("GUIDE: load_profile_error  %s ", e.what());
        // wxMessageBox(e.what(), "", MB_OK);
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ", error: " << e.what() << std::endl;
    }

    std::string strAll = m_ProfileJson.dump(-1, ' ', false, json::error_handler_t::ignore);
    // wxLogMessage("GUIDE: profile_json_s2  %s ", m_ProfileJson.dump(-1, ' ', false, json::error_handler_t::ignore));

    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ", finished, json contents: " << std::endl << strAll;
    return 0;
}

int PrinterPresetConfig::LoadProfileFamily(std::string strVendor, std::string strFilePath)
{
    // wxString strFolder = strFilePath.BeforeLast(boost::filesystem::path::preferred_separator);
    boost::filesystem::path file_path(strFilePath);
    boost::filesystem::path vendor_dir      = boost::filesystem::absolute(file_path.parent_path() / strVendor).make_preferred();
    boost::filesystem::path user_vendor_dir = (boost::filesystem::path(Slic3r::data_dir()) / PRESET_SYSTEM_DIR / strVendor).make_preferred();
    // judge if user has copy vendor dir to data dir
    bool bHasUserVendor = false;
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(",  vendor path %1%.") % vendor_dir.string();
    try {
        // wxLogMessage("GUIDE: json_path1  %s", w2s(strFilePath));

        std::string contents;
        if (strVendor == "Creality") {
            boost::filesystem::path user_vendor_path = (boost::filesystem::path(Slic3r::data_dir()) / PRESET_SYSTEM_DIR / "Creality.json")
                                                           .make_preferred();
            if (boost::filesystem::exists(user_vendor_path)) {
                LoadFile(user_vendor_path.string(), contents);
                bHasUserVendor = true;
            } else {
                LoadFile(strFilePath, contents);
            }

        } else {
            LoadFile(strFilePath, contents);
        }

        // wxLogMessage("GUIDE: json_path1 content: %s", contents);
        json jLocal = json::parse(contents);
        // wxLogMessage("GUIDE: json_path1 Loaded");

        // BBS:Machine
        // std::map<string,string> vecAre;
        std::map<string, AreaInfo> mapInfo;

        json pmachine = jLocal["machine_list"];
        int  nsize    = pmachine.size();
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(",  got %1% machines") % nsize;
        for (int n = 0; n < nsize; n++) {
            json OneMachine = pmachine.at(n);

            std::string s1 = OneMachine["name"];
            std::string s2 = OneMachine["sub_path"];

            // wxString ModelFilePath = wxString::Format("%s\\%s\\%s", strFolder, strVendor, s2);
            boost::filesystem::path sub_path = boost::filesystem::absolute(vendor_dir / s2).make_preferred();
            if (bHasUserVendor) {
                sub_path = boost::filesystem::absolute(user_vendor_dir / s2).make_preferred();
            }

            std::string sub_file = sub_path.string();
            LoadFile(sub_file, contents);
            json pm = json::parse(contents);

            GetPrinterArea(pm, mapInfo);

            std::string strInstant = pm["instantiation"];
            if (strInstant.compare("true") == 0) {
                OneMachine["model"]  = pm["printer_model"];
                OneMachine["nozzle"] = pm["nozzle_diameter"][0];

                // GetPrinterArea(pm, vecAre);

                m_ProfileJson["machine"][s1] = OneMachine;
            }
        }

        // BBS:models
        json pmodels = jLocal["machine_model_list"];
        nsize        = pmodels.size();

        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(",  got %1% machine models") % nsize;

        for (int n = 0; n < nsize; n++) {
            json OneModel = pmodels.at(n);

            OneModel["model"] = OneModel["name"];
            OneModel.erase("name");

            std::string             s1       = OneModel["model"];
            std::string             s2       = OneModel["sub_path"];
            boost::filesystem::path sub_path = boost::filesystem::absolute(vendor_dir / s2).make_preferred();
            if (bHasUserVendor) {
                sub_path = boost::filesystem::absolute(user_vendor_dir / s2).make_preferred();
            }
            std::string sub_file = sub_path.string();

            // wxLogMessage("GUIDE: json_path2  %s", w2s(ModelFilePath));
            LoadFile(sub_file, contents);
            // wxLogMessage("GUIDE: json_path2 content: %s", contents);
            json pm = json::parse(contents);
            // wxLogMessage("GUIDE: json_path2  loaded");

            OneModel["vendor"]    = strVendor;
            std::string NozzleOpt = pm["nozzle_diameter"];
            StringReplace(NozzleOpt, " ", "");
            OneModel["nozzle_diameter"] = NozzleOpt;
            OneModel["materials"]       = pm["default_materials"];

            // wxString strCoverPath = wxString::Format("%s\\%s\\%s_cover.png", strFolder, strVendor, std::string(s1.mb_str()));
            std::string             cover_file = s1 + "_cover.png";
            boost::filesystem::path cover_path = boost::filesystem::absolute(boost::filesystem::path(resources_dir()) / "/profiles/" /
                                                                             strVendor / cover_file)
                                                     .make_preferred();
            if (!boost::filesystem::exists(cover_path)) {
                m_mapMachineThumbnail.count(s1) > 0 ?
                    cover_path = m_mapMachineThumbnail[s1] :
                    cover_path = (boost::filesystem::absolute(boost::filesystem::path(resources_dir()) / "/web/image/printer/") / cover_file)
                                     .make_preferred();
                // cover_path =
                //     (boost::filesystem::absolute(boost::filesystem::path(resources_dir()) / "/web/image/printer/") /
                //      cover_file)
                //         .make_preferred();
            }
            std::string url = cover_path.string();
            std::regex  pattern("\\\\");
            std::string replacement = "/";
            std::string output      = std::regex_replace(url, pattern, replacement);
            std::regex  pattern2("#");
            std::string replacement2 = "%23";
            output                   = std::regex_replace(output, pattern2, replacement2);
            OneModel["cover"]        = output;

            OneModel["nozzle_selected"] = "";

            for (const auto& pair : mapInfo) {
                // k1 max ���⴦������ʾ0.4����Ĵ�ӡ����
                if ("K1 Max" == s1) {
                    if ((pair.second.strModelName == s1) && (pair.first.find("0.4") != string::npos)) {
                        OneModel["area"] = pair.second.strAreaInfo + "*" + pair.second.strHeightInfo;
                        break;
                    }
                } else {
                    if (pair.second.strModelName == s1) {
                        OneModel["area"] = pair.second.strAreaInfo + "*" + pair.second.strHeightInfo;
                        break;
                    }
                }
            }
            m_ProfileJson["model"].push_back(OneModel);
        }

        // BBS:Filament
        json pFilament = jLocal["filament_list"];
        json tFilaList = json::object();
        nsize          = pFilament.size();

        for (int n = 0; n < nsize; n++) {
            json OneFF = pFilament.at(n);

            std::string s1 = OneFF["name"];
            std::string s2 = OneFF["sub_path"];

            tFilaList[s1] = OneFF;
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "Vendor: " << strVendor << ", tFilaList Add: " << s1;
        }

        int nFalse  = 0;
        int nModel  = 0;
        int nFinish = 0;
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(",  got %1% filaments") % nsize;
        for (int n = 0; n < nsize; n++) {
            json OneFF = pFilament.at(n);

            std::string s1 = OneFF["name"];
            std::string s2 = OneFF["sub_path"];

            if (!m_ProfileJson["filament"].contains(s1)) {
                // wxString ModelFilePath = wxString::Format("%s\\%s\\%s", strFolder, strVendor, s2);
                boost::filesystem::path sub_path = boost::filesystem::absolute(vendor_dir / s2).make_preferred();
                if (bHasUserVendor) {
                    sub_path = boost::filesystem::absolute(user_vendor_dir / s2).make_preferred();
                }
                std::string sub_file = sub_path.string();
                LoadFile(sub_file, contents);
                json pm = json::parse(contents);

                std::string strInstant = pm["instantiation"];
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "Load Filament:" << s1 << ",Path:" << sub_file << ",instantiation?"
                                        << strInstant;

                if (strInstant == "true") {
                    std::string sV;
                    std::string sT;

                    int nRet = GetFilamentInfo(vendor_dir.string(), tFilaList, sub_file, sV, sT);
                    if (nRet != 0) {
                        BOOST_LOG_TRIVIAL(info)
                            << __FUNCTION__ << "Load Filament:" << s1 << ",GetFilamentInfo Failed, Vendor:" << sV << ",Type:" << sT;
                        continue;
                    }

                    OneFF["vendor"] = sV;
                    OneFF["type"]   = sT;

                    OneFF["models"] = "";

                    json        pPrinters = pm["compatible_printers"];
                    int         nPrinter  = pPrinters.size();
                    std::string ModelList = "";
                    for (int i = 0; i < nPrinter; i++) {
                        std::string sP = pPrinters.at(i);
                        if (m_ProfileJson["machine"].contains(sP)) {
                            std::string mModel   = m_ProfileJson["machine"][sP]["model"];
                            std::string mNozzle  = m_ProfileJson["machine"][sP]["nozzle"];
                            std::string NewModel = mModel + "++" + mNozzle;

                            ModelList = (boost::format("%1%[%2%]") % ModelList % NewModel).str();
                        }
                    }

                    OneFF["models"]   = ModelList;
                    OneFF["selected"] = 0;

                    m_ProfileJson["filament"][s1] = OneFF;
                } else
                    continue;
            }
        }

        // process
        json pProcess = jLocal["process_list"];
        nsize         = pProcess.size();
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(",  got %1% processes") % nsize;
        for (int n = 0; n < nsize; n++) {
            json OneProcess = pProcess.at(n);

            std::string s2 = OneProcess["sub_path"];
            // wxString ModelFilePath = wxString::Format("%s\\%s\\%s", strFolder, strVendor, s2);
            boost::filesystem::path sub_path = boost::filesystem::absolute(vendor_dir / s2).make_preferred();
            if (bHasUserVendor) {
                sub_path = boost::filesystem::absolute(user_vendor_dir / s2).make_preferred();
            }
            std::string sub_file = sub_path.string();

            LoadFile(sub_file, contents);
            json pm = json::parse(contents);

            std::string bInstall = pm["instantiation"];
            if (bInstall == "true") {
                m_ProfileJson["process"].push_back(OneProcess);
            }
        }

    } catch (nlohmann::detail::parse_error& err) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": parse " << strFilePath
                                 << " got a nlohmann::detail::parse_error, reason = " << err.what();
        return -1;
    } catch (std::exception& e) {
        // wxMessageBox(e.what(), "", MB_OK);
        // wxLogMessage("GUIDE: LoadFamily Error: %s", e.what());
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": parse " << strFilePath << " got exception: " << e.what();
        return -1;
    }

    return 0;
}

int PrinterPresetConfig::LoadMachineJson(std::string strVendor, std::string strFilePath)
{
    // wxString strFolder = strFilePath.BeforeLast(boost::filesystem::path::preferred_separator);
    boost::filesystem::path file_path(strFilePath);
    boost::filesystem::path vendor_dir = boost::filesystem::absolute(file_path.parent_path() / strVendor).make_preferred();
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(",  vendor path %1%.") % vendor_dir.string();
    try {
        std::string contents;
        LoadFile(strFilePath, contents);
        json jLocal = json::parse(contents);

        json pmodels = jLocal["printerList"];
        json series  = jLocal["series"];

        std::vector<PrinterInfo> printers;

        for (const auto& item : series) {
            int         id   = item["id"];
            std::string name = item["name"];

            if (name.empty())
                continue;

            PrinterInfo printerInfo;
            printerInfo.name = name;

            for (const auto& printer : pmodels) {
                int seriesId = printer["seriesId"];
                if (seriesId == id) {
                    std::string str1 = printer["name"];
                    if (str1.find("Creality") == std::string::npos) {
                        str1 = "Creality " + str1;
                    }
                    std::string str2 = printer["printerIntName"];
                    printerInfo.seriesNameList += (str1 + ";" + str2 + ";");
                    std::string printerName = printer["name"];
                    if (printerName.find("Creality") == std::string::npos) {
                        printerName = "Creality " + printerName;
                    }
                    m_mapMachineThumbnail[printerName] = printer["thumbnail"];
                }
            }

            printers.push_back(printerInfo);
        }

        std::sort(printers.begin(), printers.end(), customComparator);

        for (const auto& info : printers) {
            json childList = json::object();

            std::string name  = info.name;
            wxString    sName = _L(name);
            childList["name"] = sName.utf8_str();

            childList["printers"] = info.seriesNameList;
            m_MachineJson["machine"].push_back(childList);
        }

        // wxString strJS = wxString::Format("handleStudioCmd(%s)", m_MachineJson.dump(-1, ' ', true));

    } catch (nlohmann::detail::parse_error& err) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": parse " << strFilePath
                                 << " got a nlohmann::detail::parse_error, reason = " << err.what();
        return -1;
    } catch (std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": parse " << strFilePath << " got exception: " << e.what();
        return -1;
    }

    return 0;
}

bool PrinterPresetConfig::LoadFile(std::string jPath, std::string& sContent)
{
    try {
        boost::nowide::ifstream t(jPath);
        std::stringstream       buffer;
        buffer << t.rdbuf();
        sContent = buffer.str();
        BOOST_LOG_TRIVIAL(trace) << __FUNCTION__ << boost::format(", load %1% into buffer") % jPath;
    } catch (std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ",  got exception: " << e.what();
        return false;
    }

    return true;
}

int PrinterPresetConfig::GetFilamentInfo(
    std::string VendorDirectory, json& pFilaList, std::string filepath, std::string& sVendor, std::string& sType)
{
    // GetStardardFilePath(filepath);
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " GetFilamentInfo:VendorDirectory - " << VendorDirectory << ", Filepath - " << filepath;

    try {
        std::string contents;
        LoadFile(filepath, contents);
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ": Json Contents: " << contents;
        json jLocal = json::parse(contents);

        if (sVendor == "") {
            if (jLocal.contains("filament_vendor"))
                sVendor = jLocal["filament_vendor"][0];
            else {
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << filepath << " - Not Contains filament_vendor";
            }
        }

        if (sType == "") {
            if (jLocal.contains("filament_type"))
                sType = jLocal["filament_type"][0];
            else {
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << filepath << " - Not Contains filament_type";
            }
        }

        if (sVendor == "" || sType == "") {
            if (jLocal.contains("inherits")) {
                std::string FName = jLocal["inherits"];

                if (!pFilaList.contains(FName)) {
                    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "pFilaList - Not Contains inherits filaments: " << FName;
                    return -1;
                }

                std::string FPath = pFilaList[FName]["sub_path"];
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " Before Format Inherits Path: VendorDirectory - " << VendorDirectory
                                        << ", sub_path - " << FPath;
                wxString                strNewFile = wxString::Format("%s%c%s", wxString(VendorDirectory.c_str(), wxConvUTF8),
                                                       boost::filesystem::path::preferred_separator, FPath);
                boost::filesystem::path inherits_path(strNewFile.ToStdString());

                // boost::filesystem::path nf(strNewFile.c_str());
                if (boost::filesystem::exists(inherits_path))
                    return GetFilamentInfo(VendorDirectory, pFilaList, inherits_path.string(), sVendor, sType);
                else {
                    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " inherits File Not Exist: " << inherits_path;
                    return -1;
                }
            } else {
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << filepath << " - Not Contains inherits";
                if (sType == "") {
                    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << "sType is Empty";
                    return -1;
                } else
                    sVendor = "Generic";
                return 0;
            }
        } else
            return 0;
    } catch (nlohmann::detail::parse_error& err) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": parse " << filepath
                                 << " got a nlohmann::detail::parse_error, reason = " << err.what();
        return -1;
    } catch (std::exception& e) {
        // wxLogMessage("GUIDE: load_profile_error  %s ", e.what());
        // wxMessageBox(e.what(), "", MB_OK);
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": parse " << filepath << " got exception: " << e.what();
        return -1;
    }

    return 0;
}

}}
