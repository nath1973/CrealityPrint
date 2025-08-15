#include "Check3mfVendor.hpp"
#include <list>
#include "I18N.hpp"
#include "GUI_App.hpp"
#include "MainFrame.hpp"
#include "ParamsPanel.hpp"
#include "Tab.hpp"
#include "libslic3r/common_header/common_header.h"
#include <expat.h>

namespace Slic3r {
namespace GUI {

Check3mfVendor::Check3mfVendor() {}
Check3mfVendor* Check3mfVendor::getInstance()
{
    static Check3mfVendor instance;
    return &instance;
}

bool Check3mfVendor::check(const std::string& fileName, const std::string& printerSettingId, BusyCursor* busy)
{
    bool bRet = false;
    m_isCreality3mf = false;
    bRet = isCreality3mf(fileName);
    m_isCreality3mf = bRet;
    if (!bRet) {
        if (busy != nullptr)
            busy->reset();
        ChoosePresetDlg dlg((wxWindow*)wxGetApp().mainframe, printerSettingId);
        dlg.ShowModal();
        if (busy != nullptr)
            busy->set();

        if (dlg.m_printerPresetIdx != wxGetApp().plater()->sidebar_printer().get_selection_combo_printer()) {
            wxGetApp().plater()->sidebar_printer().select_printer_preset(dlg.m_printerPresetName, dlg.m_printerPresetIdx);
        }
    }
    return bRet;
}

bool Check3mfVendor::isCreality3mf() {
    return m_isCreality3mf;
}

void Check3mfVendor::setCreality3mf(bool isCreality3mf) {
    m_isCreality3mf = isCreality3mf;
}

bool Check3mfVendor::isCreality3mf(const std::string& fileName)
{
    bool bRet = false;

    mz_zip_archive archive;
    mz_zip_zero_struct(&archive);
    do {
        if (!open_zip_reader(&archive, fileName)) {
            break;
        }
        //  1、判断是否有Metadata/creality.config，如果有说明是creality3mf保存的文件
        //  2、判断Creality字样是否在3D/3dmodel.model中
        const char* crealityConfig = "Metadata/creality.config";
        int file_index = mz_zip_reader_locate_file(&archive, crealityConfig, nullptr, 0);
        if (file_index < 0) {
            if (!isCrealityIn3dModel(&archive)) {
                break;
            }
        }
        bRet = true;
    } while (0);
    close_zip_reader(&archive);

    return bRet;
}

static bool isApplicationField = false;
static void startTag(void* userData, const char* name, const char** atts)
{
    if (strcmp(name, "metadata") == 0) {
        for (int i = 0; atts[i]; i += 2) {
            if (strcmp(atts[i], "name") == 0 && strcmp(atts[i + 1], "Application") == 0) {
                isApplicationField = true;
            }
        }
    }
}
static void endTag(void* userData, const char* name)
{
    if (strcmp(name, "metadata") == 0) {
        if (isApplicationField) {
            isApplicationField = false;
        }
    }
}
static void characterData(void* userData, const XML_Char* s, int len)
{
    Check3mfVendor* data = (Check3mfVendor*) userData;
    if (isApplicationField) {
        std::string itemValue(s, len);
        if (itemValue.find("Creality") != std::string::npos) {
            //isCreality3mf = true;
            data->setCreality3mf(true);
        }
    }
}

bool Check3mfVendor::isCrealityIn3dModel(mz_zip_archive* pArchive)
{
    bool bRet = false;
    isApplicationField = false;

    const std::string model_file       = "3D/3dmodel.model";
    int               model_file_index = mz_zip_reader_locate_file(pArchive, model_file.c_str(), nullptr, 0);
    if (model_file_index != -1) {
        int depth = 0;
        XML_Parser m_parser;
        do {
            m_parser  = XML_ParserCreate(nullptr);
            XML_SetUserData(m_parser, (void*)this);
            XML_SetElementHandler(m_parser, startTag, endTag);
            XML_SetCharacterDataHandler(m_parser, characterData);

            mz_zip_archive_file_stat stat;
            if (!mz_zip_reader_file_stat(pArchive, model_file_index, &stat))
            break;

            void* parser_buffer = XML_GetBuffer(m_parser, (int)stat.m_uncomp_size);
            if (parser_buffer == nullptr)
            break;

            mz_bool res = mz_zip_reader_extract_file_to_mem(pArchive, stat.m_filename, parser_buffer, (size_t)stat.m_uncomp_size, 0);
            if (res == 0)
            break;

            XML_ParseBuffer(m_parser, (int)stat.m_uncomp_size, 1);

            bRet = m_isCreality3mf;
        } while (0);
        XML_ParserFree(m_parser);
    }
    return bRet;
}

ChoosePresetDlg::ChoosePresetDlg(wxWindow* parent, const std::string& printerSettingId)
    : DPIDialog(parent ? parent : nullptr, wxID_ANY, _L("Tips"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX)
{
    std::string icon_path = (boost::format("%1%/images/%2%.ico") % resources_dir() % Slic3r::CxBuildInfo::getIconName()).str();
    this->SetIcon(wxIcon(encode_path(icon_path.c_str()), wxBITMAP_TYPE_ICO));

    this->SetBackgroundColour(*wxWHITE);
    this->SetMinSize(wxSize(FromDIP(600), FromDIP(280)));
    this->SetMaxSize(wxSize(FromDIP(600), FromDIP(280)));
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText* tip = new wxStaticText(this, wxID_ANY, _L("This project file is not from Creality Print. Please select the printer preset you want to use for slicing."));
    tip->SetMinSize(wxSize(FromDIP(520), FromDIP(44)));
    tip->SetMaxSize(wxSize(FromDIP(520), FromDIP(44)));
    main_sizer->AddSpacer(FromDIP(20));
    main_sizer->Add(tip, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(40));
    m_combo = new ::ComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(), 0, nullptr, wxCB_READONLY);
    m_combo->SetMinSize(wxSize(FromDIP(520), FromDIP(40)));
    m_combo->SetMaxSize(wxSize(FromDIP(520), FromDIP(40)));
    m_combo->Bind(wxEVT_COMBOBOX, &ChoosePresetDlg::OnComboboxSelected, this);

    /* std::list<std::string> userPresets;
    std::list<std::string> systemPresets;
    const std::deque<Preset>& presets = wxGetApp().preset_bundle->printers.get_presets();
    for (auto& preset : presets) {
        if (!preset.is_visible)
            continue;
        if (preset.is_user()) {
            userPresets.push_back(preset.name);
        } else if (preset.is_system) {
            systemPresets.push_back(preset.name);
        }
    }
    if (userPresets.size() > 0) {
        m_combo->AppendString("-----" + _L("User presets") + "-----");
        m_vtComboText.push_back("-----User Presets-----");
        for (auto& preset : userPresets) {
            m_combo->AppendString(wxString::FromUTF8(preset.c_str()));
            m_vtComboText.push_back(preset);
        }
    }
    if (systemPresets.size() > 0) {
        m_combo->AppendString("-----" + _L("System presets") + "-----");
        m_vtComboText.push_back("-----System Presets-----");
        for (auto& preset : systemPresets) {
            m_combo->AppendString(wxString::FromUTF8(preset.c_str()));
            m_vtComboText.push_back(preset);
        }
    }*/
    m_vtComboText = wxGetApp().plater()->sidebar_printer().texts_of_combo_printer();
    bool isProjectPreset = false;
    int  firstCrealityPresetIdx = -1;
    int  defaultPrinterSettingId = -1;
    for (int i = 0; i < m_vtComboText.size(); ++i) {
        auto& item = m_vtComboText[i];
        if (wxString::FromUTF8(item) == wxString("------ " + _L("User presets") + " ------") ||
            wxString::FromUTF8(item) == wxString("------ " + _L("System presets") + " ------")) {
            isProjectPreset = false;
        }
        if (wxString::FromUTF8(item) == wxString("------ " + _L("Project-inside presets") + " ------")) {
            isProjectPreset = true;
        }
        if (isProjectPreset) {
            m_projectPresetCount++;
            continue;
        }
        if (wxString::FromUTF8(item) == wxString("------ " + _L("Select/Remove printers(system presets)") + " ------") ||
            wxString::FromUTF8(item) == wxString("------ " + _L("Create Nozzle") + " ------") ||
            wxString::FromUTF8(item) == wxString("------ " + _L("Create printer") + " ------")) {
            continue;
        }

        auto preset = wxGetApp().preset_bundle->printers.find_preset(item);
        std::string presetName = item;
        if (preset == nullptr && item.c_str()[0] == '*') {
            preset = wxGetApp().preset_bundle->printers.find_preset(item.substr(2));
            presetName = item.substr(2);
        }

        //  获取到第一个创想的系统预设
        if (firstCrealityPresetIdx == -1 && /*item.find("Creality") != std::string::npos*/
            preset != nullptr && preset->is_system && 
            preset->name.find("Creality") != std::string::npos) {
            firstCrealityPresetIdx = i;
        }

        //  获取到3mf文件中的预设
        if (!printerSettingId.empty() && defaultPrinterSettingId == -1 && presetName == printerSettingId &&
            preset != nullptr && !preset->is_project_embedded){
            defaultPrinterSettingId = i;
        }

        m_combo->AppendString(wxString::FromUTF8(item.c_str()));
        m_combo->SetItemTooltip(m_combo->GetCount() - 1, wxString::FromUTF8(item.c_str()));
    }
    int idx = wxGetApp().plater()->sidebar_printer().get_selection_combo_printer();
    std::string leftSelectedPresetName;
    if (idx >= 0 && idx < m_vtComboText.size()) {
        leftSelectedPresetName = m_vtComboText[idx];
    }

    if (defaultPrinterSettingId != -1) {//如果找到3mf文件中的预设，则选择该预设
        m_comboLastSelected = defaultPrinterSettingId;
    } else {
        auto preset = wxGetApp().preset_bundle->printers.find_preset(leftSelectedPresetName);
        if (preset != nullptr && preset->is_system &&
            preset->name.find("Creality") != std::string::npos) { // 如果找到3mf文件中的预设，则判断当前选择中的是创想的系统预设
            m_comboLastSelected = idx;
        } else {
            if (firstCrealityPresetIdx != -1) {//如果没有找到3mf文件中的预设，且当前选中的不是创想的系统预设，则选择第一个创想的系统预设
                m_comboLastSelected = firstCrealityPresetIdx;
            } else { 
                if (idx >= 0 && idx < m_vtComboText.size()){
                    m_comboLastSelected = idx+m_projectPresetCount;
                }
                else {
                    m_comboLastSelected = 1+m_projectPresetCount;
                }
            }
        }
    }

    m_combo->SetSelection(m_comboLastSelected - m_projectPresetCount);
    m_printerPresetName = m_vtComboText[m_comboLastSelected];
    m_printerPresetIdx  = m_comboLastSelected;
    m_combo->SetToolTip(wxString::FromUTF8(m_printerPresetName));
    main_sizer->Add(m_combo, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(40));
    Button* btnOk = new Button(this, _L("OK"));
    StateColor btn_bg_green(std::pair<wxColour, int>(wxColour(21, 191, 89), StateColor::Pressed),
                            std::pair<wxColour, int>(wxColour(21, 191, 89), StateColor::Hovered),
                            std::pair<wxColour, int>(wxColour(21, 191, 89), StateColor::Normal));

    btnOk->SetBackgroundColor(btn_bg_green);
    btnOk->SetBorderColor(wxColour(255, 255, 255));
    btnOk->SetTextColor(wxColour("#FFFFFE"));
    btnOk->SetMinSize(wxSize(FromDIP(104), FromDIP(32)));
    btnOk->SetMaxSize(wxSize(FromDIP(104), FromDIP(32)));
    btnOk->Bind(wxEVT_LEFT_DOWN, &ChoosePresetDlg::OnOk, this);

    main_sizer->AddSpacer(FromDIP(70));
    main_sizer->Add(btnOk, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(248));

    this->SetSizer(main_sizer);
    wxGetApp().UpdateDlgDarkUI(this);
    Layout();
    Fit();
}

void ChoosePresetDlg::on_dpi_changed(const wxRect& suggested_rect) {}

void ChoosePresetDlg::OnComboboxSelected(wxCommandEvent& evt)
{
    auto selected_item = evt.GetSelection();
    if (selected_item < 0 || (selected_item+m_projectPresetCount) >= m_vtComboText.size()) {
        return;
    }
    static bool hasSelected = false;
    if (wxString::FromUTF8(m_vtComboText[selected_item + m_projectPresetCount]) == wxString("------ " + _L("Project-inside presets") + " ------") ||
        wxString::FromUTF8(m_vtComboText[selected_item + m_projectPresetCount]) == wxString("------ " + _L("User presets") + " ------") ||
        wxString::FromUTF8(m_vtComboText[selected_item + m_projectPresetCount]) == wxString("------ " + _L("System presets") + " ------")) {
        if (!hasSelected)
            m_combo->SetSelection(m_comboLastSelected - m_projectPresetCount);
        else
            m_combo->SetSelection(m_comboLastSelected);
    } else {
        hasSelected = true;
        m_comboLastSelected = selected_item;
        m_printerPresetName = m_vtComboText[selected_item + m_projectPresetCount];
        m_printerPresetIdx  = selected_item + m_projectPresetCount;
        m_combo->SetToolTip(wxString::FromUTF8(m_printerPresetName));
    }
    evt.Skip();
}

void ChoosePresetDlg::OnOk(wxMouseEvent& event) {
    EndModal(wxID_OK);
}

} // namespace GUI
}
