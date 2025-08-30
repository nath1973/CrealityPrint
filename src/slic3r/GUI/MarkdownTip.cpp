#include "MarkdownTip.hpp"
#include "GUI_App.hpp"
#include "GUI.hpp"
#include "MainFrame.hpp"
#include "Widgets/WebView.hpp"

#include "libslic3r/Utils.hpp"
#include "I18N.hpp"

#include <wx/display.h>
#include <wx/string.h>
#include <wx/tokenzr.h>

namespace fs = boost::filesystem;

namespace Slic3r { namespace GUI {

// CMGUO

static std::string url_encode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;
    for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        std::string::value_type c = (*i);

        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int((unsigned char)c);
        escaped << std::nouppercase;
    }
    return escaped.str();
}
/*
 * Edge browser not support WebViewHandler
 * 
class MyWebViewHandler : public wxWebViewArchiveHandler
{
public:
    MyWebViewHandler() : wxWebViewArchiveHandler("tooltip") {}
    wxFSFile* GetFile(const wxString& uri) override {
        // file:///resources/tooltip/test.md
        wxFSFile* direct = wxWebViewArchiveHandler::GetFile(uri);
        if (direct)
            return direct;
        // file:///data/tooltips.zip;protocol=zip/test.md
        int n = uri.Find("resources/tooltip");
        if (n == wxString::npos)
            return direct;
        set_var_dir(data_dir());
        auto url = var("tooltips.zip");
        std::replace(url.begin(), url.end(), '\\', '/');
        auto uri2 = "file:///" + wxString(url) + ";protocol=zip" + uri.substr(n + 17);
        return wxWebViewArchiveHandler::GetFile(uri2);
    } 
};
*/

/*
TODO:
1. Fix height correctly now h * 1.25 + 50
2. Async RunScript avoid long call stack risc
3. Fetch markdown content in javascript (*)
4. Use scheme handler to support zip archive & make code tidy
*/
static int panelWidth = 242;
static int paddignWidth = 8;

bool ProcessTip::ShowTip(wxString const& tip,
                          wxString const& tooltip_title,
                          wxString const& tooltip_content,
                          wxString const& tooltip_img,
                          wxString const& tooltip_url,
                          wxPoint            pos)
{ 
        processTip()->m_Content.CS_Title    = tooltip_title;
        processTip()->m_Content.CS_Content  = tooltip_content;
        processTip()->m_Content.CS_URL      = tooltip_url;
        processTip()->m_Content.CS_Image    = tooltip_img;
        return processTip()->ShowTip(pos);
}

bool ProcessTip::ShowTip(wxPoint pos) 
{ 
    if (m_Content.CS_Title.empty()) {
        if (pos.x) {
            m_Hide = true;
            this->Dismiss();
        }
        else if (!m_Hide) {
            m_Hide = true;
            m_Timer->StartOnce(300);
            return true;
        }
    }

    bool tipChanged = m_LastTip != m_Content.CS_Title;
    if (tipChanged) 
    {
        if (m_Content.CS_Content.empty()) {
            m_Hide = true;
            //this->Hide();
            return false;
        }

        m_LastTip = m_Content.CS_Title;
    }

    wxSize size = wxDisplay(this).GetClientArea().GetSize();
    if (pos.y + this->GetSize().y > size.y)
        pos.y = size.y - this->GetSize().y;
    this->SetPosition(pos);

    if (tipChanged || m_Hide) {
        m_Hide = false;
        this->Dismiss();
        m_Timer->StartOnce(100);
    }

    return true;
}

void ProcessTip::OnTimer(wxTimerEvent& event)
{
    if (m_Hide) 
    {
        wxPoint pos = ScreenToClient(wxGetMousePosition());
        if (GetClientRect().Contains(pos)) 
        {
            m_Timer->StartOnce();
            return;
        }
        this->Dismiss();
    } 
    else 
    {
        updateUI();
        m_Hide = false;
        this->Popup();
    }
}

ProcessTip* ProcessTip::processTip(bool create) 
{
    static ProcessTip * processTip = nullptr;
    if (processTip == nullptr && create)
        processTip = new ProcessTip;

    //processTip->SetSize(242, -1);
    return processTip;
}
void    ProcessTip::closeTip()
{
    m_Hide = true;
    m_Timer->StartOnce(300);
}
 void ProcessTip::Recreate(wxWindow *parent)
{
    if (auto tip = processTip(false)) {
        tip->Reparent(parent);
    }
}
 // 重写鼠标事件处理
void ProcessTip::OnMouseEvent(wxMouseEvent& event)
    {
        // 如果需要，可以将事件传递给父窗口
        wxWindow* parent = GetParent();
        if (parent)
        {
            wxMouseEvent newEvent(event);
            newEvent.SetEventObject(parent);
            parent->ProcessWindowEvent(newEvent);
        }
        event.Skip();
    }
ProcessTip::ProcessTip()
: wxPopupTransientWindow(wxGetApp().mainframe, wxBORDER_NONE)
{
    m_bitmap_cache = new Slic3r::GUI::BitmapCache;
    this->SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
    m_Timer = new wxTimer;
    m_Timer->Bind(wxEVT_TIMER, &ProcessTip::OnTimer, this);

    wxSizer * mainSizer = new wxBoxSizer(wxVERTICAL);
    //SetSizer(mainSizer);
    SetSizerAndFit(mainSizer);

    bool is_dark = wxGetApp().dark_mode();
    SetBackgroundColour(is_dark ? wxColor("#222222") : wxColor("#F7F8FA"));

    const wxColour fontColor = is_dark ? wxColour("#DBDBDB") : wxColour("#4E5969");

    m_Title_text = new wxStaticText(this, wxID_ANY, m_Content.CS_Title, wxDefaultPosition, wxDefaultSize);
    m_Title_text->SetFont(Label::Head_14);
    m_Title_text->SetForegroundColour(fontColor);
    mainSizer->AddSpacer(FromDIP(8));
    mainSizer->Add(m_Title_text, 0, wxLEFT, FromDIP(8));
    mainSizer->AddSpacer(FromDIP(8));

    m_Content_text = new wxStaticText(this, wxID_ANY, m_Content.CS_Content, wxDefaultPosition, wxDefaultSize);
    m_Content_text->SetSize({226, -1});
    m_Content_text->SetMinSize({226, -1});
    m_Content_text->SetMaxSize({226, -1});
    m_Content_text->Wrap(FromDIP(226));
    m_Content_text->SetFont(Label::Body_12);
    m_Content_text->SetForegroundColour(fontColor);

    wxFont font = m_Content_text->GetFont();
    font.SetPointSize(font.GetPointSize() + 1);
    m_Content_text->SetFont(font);

    int hg = m_Content_text->GetBestHeight(226);
    mainSizer->Add(m_Content_text, 1, wxLEFT | wxRIGHT | wxEXPAND, FromDIP(8));
    mainSizer->AddSpacer(FromDIP(8));

    m_ImgBox = new StaticBox(this, wxID_ANY, wxDefaultPosition);
    m_ImgBox->SetBackgroundColour(is_dark ? wxColour("#313131"):wxColour("#313131"));
    m_ImgBox->SetBorderWidth(1);
    m_ImgBox->SetBorderColor(0x7A7A7F);
    m_ImgBox->SetCornerFlags(0xF);
    //m_ImgBox->SetSize(wxSize(226, 227));
    //m_ImgBox->SetMaxSize(wxSize(226, 227));
    //m_ImgBox->SetMinSize(wxSize(226, 227));
    mainSizer->Add(m_ImgBox, 0, wxLEFT | wxRIGHT, FromDIP(8));
    mainSizer->AddSpacer(FromDIP(8));

    wxSizer * boxSizer = new wxBoxSizer(wxVERTICAL);
    m_ImgBox->SetSizer(boxSizer);

    ScalableBitmap sImg;
    if(!m_Content.CS_Image.empty())
        sImg = ScalableBitmap(m_ImgBox, m_Content.CS_Image.ToStdString(), 32);
    m_ProcessImg = new wxStaticBitmap(m_ImgBox, wxID_ANY, sImg.bmp(), wxDefaultPosition,wxSize(FromDIP(242), FromDIP(334)), 0);
    boxSizer->Add(m_ProcessImg);

    m_Url_text = new wxHyperlinkCtrl(this, wxID_ANY, m_Content.CS_URL, m_Content.CS_URL);
    m_Url_text->SetFont(Label::Body_12);
    //m_Url_text->SetForegroundColour(fontColor);
    m_Url_text->SetNormalColour(fontColor);    // ����״̬��ɫ
    m_Url_text->SetVisitedColour(fontColor);   // ���ʺ���ɫ
    m_Url_text->SetHoverColour(fontColor);     // ��ͣ״̬��ɫ
    //m_Url_text->Wrap(FromDIP(226));

    mainSizer->Add(m_Url_text, 0, wxLEFT | wxRIGHT, FromDIP(8));
    mainSizer->AddSpacer(FromDIP(8));
    SetSize(FromDIP(242), FromDIP(334));
}

ProcessTip::~ProcessTip()
{
    delete m_bitmap_cache;
}

void ProcessTip::updateUI()
{
    m_Title_text->SetLabelText(m_Content.CS_Title);
    m_Content_text->SetLabelText(m_Content.CS_Content);
    m_Url_text->SetLabelText(m_Content.CS_URL);
    m_Url_text->SetURL(m_Content.CS_URL);

    if(m_Content.CS_Image.empty())
    {
        m_ImgBox->SetSize(0, 0);
        m_ImgBox->SetMaxSize(wxSize(0, 0));
        m_ImgBox->SetMinSize(wxSize(0, 0));
    }else
    {
        m_ImgBox->SetSize(FromDIP(226), FromDIP(227));
        m_ImgBox->SetMaxSize(wxSize(FromDIP(226), FromDIP(227)));
        m_ImgBox->SetMinSize(wxSize(FromDIP(226), FromDIP(227)));
    }
    m_ImgBox->Layout();
    m_ImgBox->Refresh();
    m_ImgBox->Update();

    int textWidth, textHeight;
    std::function calcLineCount = [this, &textWidth, &textHeight](int wrapWidth, wxControl* control)
    {
        wxClientDC dc(control);
        dc.SetFont(control->GetFont());
        wxString text = control->GetLabel();
        dc.GetTextExtent(text, &textWidth, &textHeight);

        int nCount = text.Freq('\n');
        int lineCount = textWidth/wrapWidth + 1 + nCount;
        return lineCount;
    };
    int lineCount = calcLineCount(FromDIP(226), m_Content_text);

    m_Content_text->SetSize(FromDIP(226), textHeight*lineCount);
    m_Content_text->SetMinSize({FromDIP(226), textHeight*lineCount});
    m_Content_text->SetMaxSize({FromDIP(226), textHeight*lineCount});

    lineCount = calcLineCount(FromDIP(226), m_Url_text);
    m_Url_text->SetSize(FromDIP(226), textHeight * lineCount);
    m_Url_text->SetMinSize({ FromDIP(226), textHeight * lineCount });
    m_Url_text->SetMaxSize({ FromDIP(226), textHeight * lineCount });

    std::function createBitMap = [](const wxString& bmp_name_in, wxWindow* win, const int px_cnt, const wxSize imgSize)
    {
        bool        is_dark = Slic3r::GUI::wxGetApp().dark_mode();
        std::string lang    = wxGetApp().app_config->get("language");
        wxString    imgPath = bmp_name_in;
        return create_scaled_bitmap3(imgPath.ToStdString(), win, 17, imgSize);
    };

    if(!m_Content.CS_Image.empty())
    {
        wxSize imgSize(FromDIP(150), FromDIP(120));
        wxBitmap* bmp = m_bitmap_cache->find(m_Content.CS_Image.ToStdString());
        if(bmp == nullptr)
        {
            wxBitmap bitMap = createBitMap(m_Content.CS_Image, this, 17, wxSize());
            bmp = m_bitmap_cache->insert(m_Content.CS_Image.ToStdString(),bitMap);
        }
        

        m_ProcessImg->SetBitmap(*bmp);
        m_ProcessImg->Refresh();
        m_ProcessImg->Update();
        m_ProcessImg->Layout();
    }

    GetSizer()->Fit(this);
    wxSize size = GetBestSize();
    SetSize(wxSize(FromDIP(242), size.GetHeight()));

    themeChanged();
    Layout();
    Update();
    Refresh();

}

void ProcessTip::themeChanged()
{
    bool is_dark = wxGetApp().dark_mode();
    SetBackgroundColour(is_dark ? wxColor("#222222") : wxColor("#F7F8FA"));

    const wxColour fontColor = is_dark ? wxColour("#DBDBDB") : wxColour("#4E5969");
    m_Title_text->SetForegroundColour(fontColor);
    m_Content_text->SetForegroundColour(fontColor);
    //m_Url_text->SetForegroundColour(fontColor);
    m_ImgBox->SetBackgroundColour(is_dark ? wxColour("#313131"):wxColour("#e1e4e9"));

    m_Url_text->SetNormalColour(fontColor);    // ����״̬��ɫ
    m_Url_text->SetVisitedColour(fontColor);   // ���ʺ���ɫ
    m_Url_text->SetHoverColour(fontColor);     // ��ͣ״̬��ɫ
    m_Url_text->Refresh(); 
}

MarkdownTip::MarkdownTip()
    : wxPopupTransientWindow(wxGetApp().mainframe, wxBORDER_NONE)
{
    wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);

    _tipView = CreateTipView(this);
    topsizer->Add(_tipView, wxSizerFlags().Expand().Proportion(1));

    SetSizer(topsizer);
    SetSize({400, 300});

    LoadStyle();

    _timer = new wxTimer;
    _timer->Bind(wxEVT_TIMER, &MarkdownTip::OnTimer, this);
}

MarkdownTip::~MarkdownTip() { delete _timer; }

void MarkdownTip::LoadStyle()
{
    _language = GUI::into_u8(GUI::wxGetApp().current_language_code());
    fs::path ph(data_dir());
    ph /= "resources/tooltip/common/styled.html";
    _data_dir = true;
    if (!fs::exists(ph)) {
        ph = resources_dir();
        ph /= "tooltip/styled.html";
        _data_dir = false;
    }
    auto url = ph.string();
    std::replace(url.begin(), url.end(), '\\', '/');
    url = "file:///" + url;
    _tipView->LoadURL(from_u8(url));
    _lastTip.clear();
}

bool MarkdownTip::ShowTip(wxPoint pos, std::string const& tip, std::string const& tooltip, std::string img)
{
    auto size = this->GetSize();
    if (tip.empty()) {
        if (_tipView->GetParent() != this)
            return false;
        if (pos.x) {
            _hide = true;
            BOOST_LOG_TRIVIAL(info) << "MarkdownTip::ShowTip: hide soon on empty tip.";
            this->Hide();
        }
        else if (!_hide) {
            _hide = true;
            BOOST_LOG_TRIVIAL(info) << "MarkdownTip::ShowTip: start hide timer (300)...";
            _timer->StartOnce(300);
        }
        return false;
    }
    bool tipChanged = _lastTip != tip;
    if (tipChanged) {
        auto content = LoadTip(tip, tooltip);
        content += "\n";
           
        if (!img.empty())
        {
            std::string url = Slic3r::var(img);
            content += std::string("<img style=\"padding-top:10px\" src=\"file:///") + url + "#pic_center" + std::string("\" width =\"150\" height=\"120\" />");
        }

        if (content.empty()) {
            _hide = true;
            this->Hide();
            BOOST_LOG_TRIVIAL(info) << "MarkdownTip::ShowTip: hide soon on empty content.";
            return false;
        }
        auto script = "window.showMarkdown('" + url_encode(content) + "', true);";
        if (!_pendingScript.empty()) {
            _pendingScript = script;
        }
        else {
            RunScript(script);
        }
        _lastTip = tip;
        if (_tipView->GetParent() == this)
            this->Hide();
    }
    if (_tipView->GetParent() == this) {
        wxSize size = wxDisplay(this).GetClientArea().GetSize();
        _requestPos = pos;
        if (pos.y + this->GetSize().y > size.y)
            pos.y = size.y - this->GetSize().y;
        this->SetPosition(pos);
        if (tipChanged || _hide) {
            _hide = false;
            BOOST_LOG_TRIVIAL(info) << "MarkdownTip::ShowTip: start show timer (500)...";
            _timer->StartOnce(500);
        }
    }
    return true;
}

bool MarkdownTip::ShowTip(wxPoint pos, std::string const& tip) 
{
    auto size = this->GetSize();
    if (tip.empty()) {
        if (_tipView->GetParent() != this)
            return false;
        if (pos.x) {
            _hide = true;
            BOOST_LOG_TRIVIAL(info) << "MarkdownTip::ShowTip: hide soon on empty tip.";
            this->Hide();
        } else if (!_hide) {
            _hide = true;
            BOOST_LOG_TRIVIAL(info) << "MarkdownTip::ShowTip: start hide timer (300)...";
            _timer->StartOnce(300);
        }
        return false;
    }
    bool tipChanged = _lastTip != tip;
    if (tipChanged) {
        auto content = LoadTip(tip, m_Content.CS_Title + m_Content.CS_Content);
        content += "\n";

        if (!m_Content.CS_Image.empty()) {
            std::string url = Slic3r::var(m_Content.CS_Image);
            content += std::string("<div style=\"background-color:#313131; border-radius:0px; padding:0px; display:flex; justify-content:center; align-items:center; width:226px; height:227px;\">") +
                       std::string("<img style=\"margin:0 auto; display:block;\" src=\"file:///") + url +
                       std::string("\" width=\"150\" height=\"120\" /></div>");
        }
        content += "\n\n";
        content += m_Content.CS_URL;

        if (content.empty()) {
            _hide = true;
            this->Hide();
            BOOST_LOG_TRIVIAL(info) << "MarkdownTip::ShowTip: hide soon on empty content.";
            return false;
        }
        auto script = "window.showMarkdown('" + url_encode(content) + "', true);";
        if (!_pendingScript.empty()) {
            _pendingScript = script;
        } else {
            RunScript(script);
        }
        _lastTip = tip;
        if (_tipView->GetParent() == this)
            this->Hide();
    }
    if (_tipView->GetParent() == this) {
        wxSize size = wxDisplay(this).GetClientArea().GetSize();
        _requestPos = pos;
        if (pos.y + this->GetSize().y > size.y)
            pos.y = size.y - this->GetSize().y;
        this->SetPosition(pos);
        if (tipChanged || _hide) {
            _hide = false;
            BOOST_LOG_TRIVIAL(info) << "MarkdownTip::ShowTip: start show timer (500)...";
            _timer->StartOnce(500);
        }
    }
    return true;
}

std::string MarkdownTip::LoadTip(std::string const &tip, std::string const &tooltip)
{
    fs::path ph;
    wxString file;
    wxFile   f;
    if (_data_dir) {
        if (!_language.empty()) {
            ph = data_dir();
            ph /= "resources/tooltip/" + _language +  "/" + tip + ".md";
            file = from_u8(ph.string());
            if (wxFile::Exists(file) && f.Open(file)) {
                std::string content(f.Length(), 0);
                f.Read(&content[0], content.size());
                return content;
            }
        }
        ph = data_dir();
        ph /= "resources/tooltip/common/" + tip + ".md";
        file = from_u8(ph.string());
        if (wxFile::Exists(file) && f.Open(file)) {
            std::string content(f.Length(), 0);
            f.Read(&content[0], content.size());
            return content;
        }
    }
    /*
    file = var("tooltips.zip");
    if (wxFile::Exists(file) && f.Open(file)) {
        wxFileInputStream fs(f);
        wxZipInputStream zip(fs);
        file = tip + ".md";
        while (auto e = zip.GetNextEntry()) {
            if (e->GetName() == file) {
                if (zip.OpenEntry(*e)) {
                    std::string content(f.Length(), 0);
                    zip.Read(&content[0], content.size());
                    return content;
                }
                break;
            }
        }
    }
    */
    ph = resources_dir();
    ph /= "tooltip/" + _language + "/" + tip + ".md";
    file = from_u8(ph.string());
    if (wxFile::Exists(file) && f.Open(file)) {
        std::string content(f.Length(), 0);
        f.Read(&content[0], content.size());
        return content;
    }
    ph = resources_dir();
    ph /= "tooltip/" + tip + ".md";
    file = from_u8(ph.string());
    if (wxFile::Exists(file) && f.Open(file)) {
        std::string content(f.Length(), 0);
        f.Read(&content[0], content.size());
        return content;
    }
    if (!tooltip.empty()) return "#### " + _utf8(tip) + "\n" + tooltip;
    return (_tipView->GetParent() == this && tip.empty()) ? "" : LoadTip("", "");
}

void MarkdownTip::RunScript(std::string const& script)
{
    WebView::RunScript(_tipView, script);
}

wxWebView* MarkdownTip::CreateTipView(wxWindow* parent)
{
    wxWebView *tipView = WebView::CreateWebView(parent, "");
    Bind(wxEVT_WEBVIEW_LOADED, &MarkdownTip::OnLoaded, this);
    Bind(wxEVT_WEBVIEW_TITLE_CHANGED, &MarkdownTip::OnTitleChanged, this);
    Bind(wxEVT_WEBVIEW_ERROR, &MarkdownTip::OnError, this);
    return tipView;
}

void MarkdownTip::OnLoaded(wxWebViewEvent& event)
{
}

void MarkdownTip::OnTitleChanged(wxWebViewEvent& event)
{
    if (!_pendingScript.empty()) {
        RunScript(_pendingScript);
        _pendingScript.clear();
        return;
    }
#ifdef __linux__
    wxString str = "0";
#else
    wxString str = event.GetString();
#endif
    double height = 0;
    if (str.ToDouble(&height)) {
        if (height > _lastHeight - 10 && height < _lastHeight + 10)
            return;
        _lastHeight = height;
        height *= 1.25; height += 50;
        wxSize size = wxDisplay(this).GetClientArea().GetSize();
        if (height > size.y)
            height = size.y;
        wxPoint pos = _requestPos;
        if (pos.y + height > size.y)
            pos.y = size.y - height;
        this->SetSize({242, (int)height });
        this->SetPosition(pos);
    }
}
void MarkdownTip::OnError(wxWebViewEvent& event)
{
}

void MarkdownTip::OnTimer(wxTimerEvent& event)
{
    if (_hide) {
        wxPoint pos = ScreenToClient(wxGetMousePosition());
        if (GetClientRect().Contains(pos)) {
            BOOST_LOG_TRIVIAL(info) << "MarkdownTip::OnTimer: restart hide timer...";
            _timer->StartOnce();
            return;
        }
        BOOST_LOG_TRIVIAL(info) << "MarkdownTip::OnTimer: hide.";
        this->Hide();
    } else {
        BOOST_LOG_TRIVIAL(info) << "MarkdownTip::OnTimer: show.";
        this->Show();
    }
}

MarkdownTip* MarkdownTip::markdownTip(bool create)
{
    static MarkdownTip * markdownTip = nullptr;
    if (markdownTip == nullptr && create)
        markdownTip = new MarkdownTip;
    return markdownTip;
}

bool MarkdownTip::ShowTip(std::string const& tip, std::string const& tooltip, wxPoint pos, std::string img /*= ""*/)
{
#ifdef NDEBUG
    return false;
#endif
    return markdownTip()->ShowTip(pos, tip, tooltip, img);
}

bool MarkdownTip::ShowTip(std::string const& tip,
             std::string const& tooltip_title,
             std::string const& tooltip_content,
             std::string const& tooltip_img,
             std::string const& tooltip_url,
             wxPoint            pos)
{
    markdownTip()->m_Content.CS_Title = tooltip_title;
    markdownTip()->m_Content.CS_Content = tooltip_content;
    markdownTip()->m_Content.CS_URL   = tooltip_url;
    markdownTip()->m_Content.CS_Image = tooltip_img;

#ifdef NDEBUG
    return false;
#endif
    return markdownTip()->ShowTip(pos, tip);
}

void MarkdownTip::ExitTip()
{
    //if (auto tip = markdownTip(false))
    //    tip->Destroy();
}

void MarkdownTip::Reload()
{
    if (auto tip = markdownTip(false)) 
        tip->LoadStyle();
}

void MarkdownTip::Recreate(wxWindow *parent)
{
    if (auto tip = markdownTip(false)) {
        tip->Reparent(parent);
        tip->LoadStyle(); // switch language
    }
}

wxWindow* MarkdownTip::AttachTo(wxWindow* parent)
{
    MarkdownTip& tip = *markdownTip();
    tip._tipView = tip.CreateTipView(parent);
    tip._pendingScript = " ";
    return tip._tipView;
}

wxWindow* MarkdownTip::DetachFrom(wxWindow* parent)
{
    MarkdownTip& tip = *markdownTip();
    if (tip._tipView->GetParent() == parent) {
        tip.Destroy();
    }
    return NULL;
}

MarkdownTip* MarkdownTip::instance() 
{ return markdownTip(); }
}
}
