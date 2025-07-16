#include "ProfileFamilyLoader.hpp"

#include <mutex>
#include <map>
#include <string>

#include <wx/wx.h>
#include <boost/filesystem/path.hpp>
#include <tbb/parallel_for_each.h>

#include "nlohmann/json.hpp"

#include "libslic3r/Utils.hpp"
#include "libslic3r/Preset.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/AppConfig.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/3DBed.hpp"

using namespace Slic3r;
using namespace GUI;

static std::string w2s(wxString sSrc) { return std::string(sSrc.mb_str()); }

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

static struct AreaInfo
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

static struct PrinterInfo
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
                return i; // 返回优先级索引
            }
        }
        return order.size(); // 如果都不包含，返回最低优先级
    };

    return getPriority(a.name) < getPriority(b.name);
}

int ProfileFamilyLoader::ParallelLoadProfileJson(
    json& output_profile, 
    json& output_machine, 
    bool& bbl_bundle_rsrc,
    std::string load_machine_vendor/* = "Creality"*/,
    LOAD_MODEL load_flag /* = LOAD_MODEL::ALL*/)
{
    std::map<std::string, std::string> mapMachineThumbnail;
    try {
        auto gen_profile_json = [load_flag]() -> json {
            json ret;
            ret             = json::parse("{}");
            ret["filament"] = json::object();
            if (load_flag == LOAD_MODEL::ALL)
            {
                ret["model"]   = json::array();
                ret["machine"] = json::object();
                ret["process"] = json::array();
            }
            return ret;
        };

        std::mutex profile_json_mutex;
        output_profile           = gen_profile_json();
        auto merger_profile_data = [&profile_json_mutex, &output_profile, this](json& patch) {
            std::lock_guard<std::mutex> lk(profile_json_mutex);
            // model
            if (patch.contains("model")) {
                for (const auto& item : patch["model"]) {
                    output_profile["model"].push_back(item);
                }
            }
            // machine
            if (patch.contains("machine")) {
                output_profile["machine"].merge_patch(patch["machine"]);
            }
            // filament
            if (patch.contains("filament")) {
                output_profile["filament"].merge_patch(patch["filament"]);
            }
            // process
            if (patch.contains("process")) {
                for (const auto& item : patch["process"]) {
                    output_profile["process"].push_back(item);
                }
            }
        };

        boost::filesystem::path vendor_dir = (boost::filesystem::path(Slic3r::data_dir()) / PRESET_SYSTEM_DIR).make_preferred();
        boost::filesystem::path rsrc_vendor_dir = (boost::filesystem::path(resources_dir()) / "profiles").make_preferred();

        // BBS: add BBL as default
        // BBS: add json logic for vendor bundle
        auto bbl_bundle_path = vendor_dir;
        bbl_bundle_rsrc      = false;
        if (!boost::filesystem::exists((vendor_dir / PresetBundle::BBL_BUNDLE).replace_extension(".json"))) {
            bbl_bundle_path = rsrc_vendor_dir;
            bbl_bundle_rsrc = true;
        }

        output_machine                      = json::parse("{}");
        output_machine["machine"]           = json::array();
        boost::filesystem::path machinepath = vendor_dir;

        if (!boost::filesystem::exists((vendor_dir / load_machine_vendor / "machineList").replace_extension(".json"))) {
            machinepath = rsrc_vendor_dir;
        }
        LoadMachineJson(
            output_machine,
            mapMachineThumbnail,
            "machineList", 
            (machinepath / load_machine_vendor / "machineList.json").string());
        // load BBL bundle from user data path
        std::string targetPath = bbl_bundle_path.make_preferred().string();
        boost::filesystem::path               myPath(targetPath);
        boost::filesystem::directory_iterator endIter;

        std::vector<fs::path> paths;
        for (boost::filesystem::directory_iterator iter(myPath); iter != endIter; iter++) {
            paths.push_back(iter->path());
        }
        tbb::parallel_for_each(paths.begin(), paths.end(),
                               [this, gen_profile_json, 
                                merger_profile_data, &mapMachineThumbnail, load_flag](const boost::filesystem::path& path) {
                                   if (boost::filesystem::is_directory(path)) {
                                       // cout << "is dir" << endl;
                                       // cout << iter->path().string() << endl;
                                   } else {
                                       // cout << "is a file" << endl;
                                       // cout << iter->path().string() << endl;

                                       wxString strVendor    = from_u8(path.string()).BeforeLast('.');
                                       strVendor             = strVendor.AfterLast('\\');
                                       strVendor             = strVendor.AfterLast('\/');
                                       wxString strExtension = from_u8(path.string()).AfterLast('.').Lower();

                                       if (w2s(strVendor) == PresetBundle::BBL_BUNDLE && strExtension.CmpNoCase("json") == 0) {
                                           json output = gen_profile_json();
                                           LoadProfileFamily(
                                               w2s(strVendor), path.string(), load_flag, output, mapMachineThumbnail);
                                           merger_profile_data(output);
                                       }
                                   }
                               });
        boost::filesystem::directory_iterator others_endIter;
        std::vector<fs::path>                 rsrc_vendor_dirs;
        for (boost::filesystem::directory_iterator iter(rsrc_vendor_dir); iter != endIter; iter++) {
            rsrc_vendor_dirs.push_back(iter->path());
        }

        tbb::parallel_for_each(rsrc_vendor_dirs.begin(), rsrc_vendor_dirs.end(),
                               [this, gen_profile_json, 
                                merger_profile_data, &mapMachineThumbnail, load_flag](const boost::filesystem::path& path) {
                                   if (boost::filesystem::is_directory(path)) {
                                       // cout << "is dir" << endl;
                                       // cout << path.string() << endl;
                                   } else {
                                       /*cout << "is a file" << endl;
                                      cout << path.string() << endl;*/
                                       wxString strVendor    = from_u8(path.string()).BeforeLast('.');
                                       strVendor             = strVendor.AfterLast('\\');
                                       strVendor             = strVendor.AfterLast('\/');
                                       wxString strExtension = from_u8(path.string()).AfterLast('.').Lower();
                                       if (w2s(strVendor) != PresetBundle::BBL_BUNDLE && strExtension.CmpNoCase("json") == 0) {
                                           json output = gen_profile_json();
                                           LoadProfileFamily(w2s(strVendor), path.string(), load_flag, output, mapMachineThumbnail);
                                           merger_profile_data(output);
                                       }
                                   }
                               });
    } catch (std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ", error: " << e.what() << std::endl;
    }
    return 0;
}

int ProfileFamilyLoader::LoadMachineJson(
    json& output_machine, std::map<std::string, std::string> &mapMachineThumbnail,
    std::string strVendor, std::string strFilePath)
{
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
                    mapMachineThumbnail[printerName] = printer["thumbnail"];
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
            output_machine["machine"].push_back(childList);
        }
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

int ProfileFamilyLoader::LoadProfileFamily(
    std::string strVendor, std::string strFilePath, LOAD_MODEL load_flag,
    json& outputJson, std::map<std::string, std::string> &mapMachineThumbnail)
{
    boost::filesystem::path file_path(strFilePath);
    boost::filesystem::path vendor_dir = boost::filesystem::absolute(file_path.parent_path() / strVendor).make_preferred();
    boost::filesystem::path user_vendor_dir = (boost::filesystem::path(Slic3r::data_dir()) / PRESET_SYSTEM_DIR / strVendor).make_preferred();
    // judge if user has copy vendor dir to data dir
    bool bHasUserVendor = false;
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(",  vendor path %1%.") % vendor_dir.string();
    try {
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

        json jLocal = json::parse(contents);

        // BBS:Machine
        std::map<string, AreaInfo> mapInfo;
        std::mutex                 mapInfoMutex;
        std::mutex                 outputJsonMutex;
        if ((int) load_flag & (int) LOAD_MODEL::MACHINE)
        {
            json pmachine = jLocal["machine_list"];
            int  nsize    = pmachine.size();
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(",  got %1% machines") % nsize;
            tbb::parallel_for(tbb::blocked_range<int>(0, nsize),
                              [this, &pmachine, vendor_dir, user_vendor_dir, bHasUserVendor, &mapInfo, &outputJson, &mapInfoMutex,
                               &outputJsonMutex](const tbb::blocked_range<int>& range) {
                                  for (auto n = range.begin(); n != range.end(); ++n) {
                                      json OneMachine = pmachine.at(n);

                                      std::string s1 = OneMachine["name"];
                                      std::string s2 = OneMachine["sub_path"];

                                      // wxString ModelFilePath = wxString::Format("%s\\%s\\%s", strFolder, strVendor, s2);
                                      boost::filesystem::path sub_path = boost::filesystem::absolute(vendor_dir / s2).make_preferred();
                                      if (bHasUserVendor) {
                                          sub_path = boost::filesystem::absolute(user_vendor_dir / s2).make_preferred();
                                      }

                                      std::string sub_file = sub_path.string();
                                      std::string contents;
                                      LoadFile(sub_file, contents);
                                      try {
                                          json pm = json::parse(contents);

                                          std::map<string, AreaInfo> mapInfoTmp;
                                          GetPrinterArea(pm, mapInfoTmp);
                                          {
                                              std::lock_guard lk(mapInfoMutex);
                                              mapInfo.merge(mapInfoTmp);
                                          }

                                          std::string strInstant = pm["instantiation"];
                                          if (strInstant.compare("true") == 0) {
                                              OneMachine["model"]  = pm["printer_model"];
                                              OneMachine["nozzle"] = pm["nozzle_diameter"][0];

                                              // GetPrinterArea(pm, vecAre);
                                              {
                                                  std::lock_guard lk(outputJsonMutex);
                                                  outputJson["machine"][s1] = OneMachine;
                                              }
                                          }
                                      } catch (nlohmann::detail::parse_error& err) {
                                      } catch (std::exception& e) {}
                                  }
                              });
        }

        // BBS:models
        if ((int)load_flag & (int)LOAD_MODEL::MODEL)
        {
            json pmodels = jLocal["machine_model_list"];
            int nsize        = pmodels.size();

            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(",  got %1% machine models") % nsize;
            tbb::parallel_for(tbb::blocked_range{0, nsize}, [this, &pmodels, bHasUserVendor, vendor_dir, user_vendor_dir, &outputJsonMutex,
                                                             strVendor, &mapInfo, &outputJson,
                                                             &mapMachineThumbnail](const tbb::blocked_range<int>& range) {
                for (auto n = range.begin(); n != range.end(); ++n) {
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

                    std::string contents;
                    LoadFile(sub_file, contents);
                    json pm = json::parse(contents);

                    OneModel["vendor"]    = strVendor;
                    std::string NozzleOpt = pm["nozzle_diameter"];
                    StringReplace(NozzleOpt, " ", "");
                    OneModel["nozzle_diameter"] = NozzleOpt;
                    OneModel["materials"]       = pm["default_materials"];

                    std::string             cover_file = s1 + "_cover.png";
                    boost::filesystem::path cover_path = boost::filesystem::absolute(boost::filesystem::path(resources_dir()) /
                                                                                     "/profiles/" / strVendor / cover_file)
                                                             .make_preferred();
                    if (!boost::filesystem::exists(cover_path)) {
                        mapMachineThumbnail.count(s1) > 0 ?
                            cover_path = mapMachineThumbnail[s1] :
                            cover_path = (boost::filesystem::absolute(boost::filesystem::path(resources_dir()) / "/web/image/printer/") /
                                          cover_file)
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
                        // k1 max 特殊处理，显示0.4喷嘴的打印区域
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
                    {
                        std::lock_guard lk(outputJsonMutex);
                        outputJson["model"].push_back(OneModel);
                    }
                }
            });
        }

        // BBS:Filament
        if ((int)load_flag & (int)LOAD_MODEL::FILAMENT)
        {
            json pFilament = jLocal["filament_list"];
            json tFilaList = json::object();
            int  nsize     = pFilament.size();

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

            tbb::parallel_for(tbb::blocked_range{0, nsize}, [this, &pFilament, &outputJson, &outputJsonMutex, bHasUserVendor, vendor_dir,
                                                             user_vendor_dir, &tFilaList](const tbb::blocked_range<int>& range) {
                for (auto n = range.begin(); n != range.end(); n++) {
                    json OneFF = pFilament.at(n);

                    std::string s1 = OneFF["name"];
                    std::string s2 = OneFF["sub_path"];

                    outputJsonMutex.lock();
                    auto elem_exists = outputJson["filament"].contains(s1);
                    outputJsonMutex.unlock();
                    if (!elem_exists) {
                        boost::filesystem::path sub_path = boost::filesystem::absolute(vendor_dir / s2).make_preferred();
                        if (bHasUserVendor) {
                            sub_path = boost::filesystem::absolute(user_vendor_dir / s2).make_preferred();
                        }
                        std::string sub_file = sub_path.string();
                        std::string contents;
                        LoadFile(sub_file, contents);
                        json pm = json::parse(contents);

                        std::string strInstant = pm["instantiation"];
                        BOOST_LOG_TRIVIAL(info)
                            << __FUNCTION__ << "Load Filament:" << s1 << ",Path:" << sub_file << ",instantiation?" << strInstant;

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
                                if (outputJson["machine"].contains(sP)) {
                                    std::string mModel   = outputJson["machine"][sP]["model"];
                                    std::string mNozzle  = outputJson["machine"][sP]["nozzle"];
                                    std::string NewModel = mModel + "++" + mNozzle;

                                    ModelList = (boost::format("%1%[%2%]") % ModelList % NewModel).str();
                                }
                            }

                            OneFF["models"]   = ModelList;
                            OneFF["selected"] = 0;

                            {
                                std::lock_guard lk(outputJsonMutex);
                                outputJson["filament"][s1] = OneFF;
                            }
                        } else
                            continue;
                    }
                }
            });
        }
        
        // process
        if ((int)load_flag & (int)LOAD_MODEL::PROCESS)
        {
            json pProcess = jLocal["process_list"];
            int  nsize    = pProcess.size();
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(",  got %1% processes") % nsize;
            tbb::parallel_for(tbb::blocked_range{0, nsize}, [this, &pProcess, bHasUserVendor, vendor_dir, user_vendor_dir, &outputJson,
                                                             &outputJsonMutex](const tbb::blocked_range<int>& range) {
                for (auto n = range.begin(); n != range.end(); n++) {
                    json OneProcess = pProcess.at(n);

                    std::string s2 = OneProcess["sub_path"];
                    boost::filesystem::path sub_path = boost::filesystem::absolute(vendor_dir / s2).make_preferred();
                    if (bHasUserVendor) {
                        sub_path = boost::filesystem::absolute(user_vendor_dir / s2).make_preferred();
                    }
                    std::string sub_file = sub_path.string();

                    std::string contents;
                    LoadFile(sub_file, contents);
                    json pm = json::parse(contents);

                    std::string bInstall = pm["instantiation"];
                    if (bInstall == "true") {
                        std::lock_guard lk(outputJsonMutex);
                        outputJson["process"].push_back(OneProcess);
                    }
                }
            });

        }
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

bool ProfileFamilyLoader::LoadFile(std::string jPath, std::string& sContent)
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

int ProfileFamilyLoader::GetFilamentInfo(
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
                boost::filesystem::path inherits_path(w2s(strNewFile));

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
