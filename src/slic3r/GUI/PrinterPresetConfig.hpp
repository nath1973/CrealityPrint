#ifndef slic3r_PrinterPresetConfig_hpp_
#define slic3r_PrinterPresetConfig_hpp_

#include <string>
#include <vector>


#include "../Utils/json_diff.hpp"
#include "libslic3r/AppConfig.hpp"

namespace Slic3r {
namespace GUI {

class PrinterPresetConfig
{
public:
    PrinterPresetConfig();
    ~PrinterPresetConfig();

    std::vector<std::string> getFilament(const std::string& printerName, const std::string& nozzle);
    int                      LoadProfile();
    json                     getProfileJson() const { 
        json m_Res           = json::object();
        m_Res["command"]     = "response_userguide_profile";
        m_Res["sequence_id"] = "10001";
        m_Res["response"]    = m_ProfileJson;
        m_Res["MachineJson"] = m_MachineJson;
        m_Res["user_preset"] = json::array(); 

        return m_Res; 
    }

private:
    // Replace it with ProfileFamilyLoader
    /*int LoadProfileFamily(std::string strVendor, std::string strFilePath);
    int LoadMachineJson(std::string strVendor, std::string strFilePath);*/

    bool LoadFile(std::string jPath, std::string& sContent);
    //int  GetFilamentInfo(std::string VendorDirectory, json& pFilaList, std::string filepath, std::string& sVendor, std::string& sType);

private:
    json m_ProfileJson;
    json m_MachineJson;
    AppConfig m_appconfig_new;

    std::map<std::string, std::string> m_mapMachineThumbnail;
    bool bbl_bundle_rsrc;
    boost::filesystem::path vendor_dir;
    boost::filesystem::path rsrc_vendor_dir;
    std::string m_Region;
    bool network_plugin_ready {false};
};

}
}

#endif
