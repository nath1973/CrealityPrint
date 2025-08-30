#ifndef slic3r_Check3mfVendor_hpp_
#define slic3r_Check3mfVendor_hpp_

#include <string>
#include "GUI_Utils.hpp"
#include "Widgets/ComboBox.hpp"
#include "miniz/miniz.h"
#include "libslic3r/miniz_extension.hpp"
#include "Plater.hpp"

namespace Slic3r {
namespace GUI {

class ChoosePresetDlg;

class Check3mfVendor
{
public:
    static Check3mfVendor* getInstance();
    // 返回值：true为创想的3mf,false为其它厂商的3mf
    bool check(const std::string& fileName, const std::string& printerSettingId, BusyCursor* busy);
    void doSelectPrinterPreset();
    bool isCreality3mf();
    void setCreality3mf(bool isCreality3mf);

private:
    Check3mfVendor();
    bool isCreality3mf(const std::string& fileName);
    bool isCrealityIn3dModel(mz_zip_archive* pArchive);

private:
    bool m_isCreality3mf = false;
    bool m_bNeedSelectPrinterPreset = false;
    std::string m_printerPresetName = "";
    int m_printerPresetIdx = -1;
};

class ChoosePresetDlg : public DPIDialog
{
public:
    ChoosePresetDlg(wxWindow* parent, const std::string& printerSettingId);
    std::string m_printerPresetName = "";
    int m_printerPresetIdx = -1;

protected:
    void on_dpi_changed(const wxRect& suggested_rect)override;
    void OnComboboxSelected(wxCommandEvent& evt);
    void OnOk(wxMouseEvent& event);

private:
    ::ComboBox* m_combo = nullptr;
    std::vector<std::string> m_vtComboText;
    int m_comboLastSelected = -1;
    int m_projectPresetCount = 0;
};

}
}

#endif
