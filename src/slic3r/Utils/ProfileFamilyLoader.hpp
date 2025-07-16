#ifndef PROFILEFAMILYLOADER_HPP
#define PROFILEFAMILYLOADER_HPP

#include <mutex>
#include <map>
#include <string>

#include <boost/filesystem/path.hpp>

#include "nlohmann/json.hpp"

namespace Slic3r {

using namespace nlohmann;
// ProfileFamilyLoader can load all profile json, such as: machine,filament,process
// PS: no cache
class ProfileFamilyLoader
{
public:
    enum class LOAD_MODEL{
        MODEL       = 0x01, 
        MACHINE     = 0x02,
        FILAMENT    = 0x04, 
        PROCESS     = 0x08, 
        ALL         = MODEL | MACHINE | FILAMENT | PROCESS
    };

    // only support LOAD_MODEL::ALL and LOAD_MODEL::FILAMENT
    int ParallelLoadProfileJson(
        json& output_profile, 
        json& output_machine, 
        bool& bbl_bundle_rsrc,
        std::string load_machine_vendor = "Creality",
        LOAD_MODEL load_flag = LOAD_MODEL::ALL);

private:
    int LoadMachineJson(
        json& output_machine, std::map<std::string, std::string> &mapMachineThumbnail,
        std::string strVendor, std::string strFilePath);
    int LoadProfileFamily(
        std::string strVendor, std::string strFilePath, LOAD_MODEL load_flag,
        json& outputJson, std::map<std::string, std::string> &mapMachineThumbnail);
    bool LoadFile(std::string jPath, std::string& sContent);
    int  GetFilamentInfo(std::string VendorDirectory, json& pFilaList, 
        std::string filepath, std::string& sVendor, std::string& sType);
};

} // namespace Slic3r

#endif // PROFILEFAMILYLOADER_HPP