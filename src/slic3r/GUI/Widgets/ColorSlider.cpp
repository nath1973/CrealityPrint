#include "ColorSlider.hpp"

wxBEGIN_EVENT_TABLE(ColorSlider, wxSlider)
    EVT_PAINT(ColorSlider::OnPaint)
    EVT_ERASE_BACKGROUND(ColorSlider::OnEraseBackground)
    EVT_SCROLL_THUMBTRACK(ColorSlider::OnScrollUpdate)
    EVT_SCROLL_CHANGED(ColorSlider::OnScrollUpdate) 
    EVT_SCROLL_THUMBRELEASE(ColorSlider::OnScrollUpdate) 
    EVT_SET_FOCUS(ColorSlider::OnFocusChange) 
    EVT_KILL_FOCUS(ColorSlider::OnFocusChange)
    EVT_SYS_COLOUR_CHANGED(ColorSlider::OnSysColourChanged)
                    wxEND_EVENT_TABLE()


    ColorSlider::ColorSlider(wxWindow* parent, wxWindowID id, int value, int minValue, int maxValue, const wxPoint& pos, const wxSize& size, long style)
    : wxSlider(parent, id, value, minValue, maxValue, pos, size, style)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT); 
    SetBackgroundColour(GetBackgroundColour());
    CallAfter([this] {
        UpdatePaletteFromSystem();
        Refresh();
    });
}

void ColorSlider::OnEraseBackground(wxEraseEvent& event)
{
    
}

void ColorSlider::OnScrollUpdate(wxScrollEvent& event)
{
    Refresh();  
    Update();
    event.Skip();
}

void ColorSlider::OnFocusChange(wxFocusEvent& event)
{
    Refresh();   
    Update();
    event.Skip();
}

void ColorSlider::SetValue(int value)
{
    wxSlider::SetValue(value);
    Refresh(); 
    Update();
}

void ColorSlider::OnPaint(wxPaintEvent& event)
{
    wxAutoBufferedPaintDC dc(this);
    PrepareDC(dc);

    dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
    dc.Clear();                                       

    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (!gc)
        return;

    wxSize sz     = GetSize();
    int    width  = sz.GetWidth();
    int    height = sz.GetHeight();

    const int trackHeight = 6;
    const int trackRadius = trackHeight / 2;
    const int trackY      = height / 2 - trackHeight / 2;

    double ratio  = static_cast<double>(GetValue() - GetMin()) / (GetMax() - GetMin());

    double    minThumbX = THUMB_RADIUS + 7;
    double    maxThumbX = width - THUMB_RADIUS - 7;

    double thumbX = static_cast<int>(ratio * (maxThumbX - minThumbX)) + minThumbX;
    double thumbY = height / 2;

    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->SetBrush(wxBrush(mTrackBg));
    gc->DrawRoundedRectangle(THUMB_RADIUS, trackY, width - 2 * THUMB_RADIUS, trackHeight, trackRadius);

    gc->SetBrush(wxBrush(mThumbFill)); 
    gc->DrawRoundedRectangle(THUMB_RADIUS, trackY, thumbX - THUMB_RADIUS, trackHeight, trackRadius);

    gc->SetBrush(wxBrush(mThumbFill)); 
    gc->SetPen(wxPen(*wxWHITE, 2));            
    gc->DrawEllipse(thumbX - THUMB_RADIUS, thumbY - THUMB_RADIUS, THUMB_RADIUS * 2, THUMB_RADIUS * 2);
}

bool ColorSlider::IsDarkAppearance() const
{
#if wxCHECK_VERSION(3, 1, 6)
    return wxSystemSettings::GetAppearance().IsDark();
#else
    const wxWindow* ref = GetParent();
    const wxColour  bg  = ref ? ref->GetBackgroundColour() : wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    const double    r = bg.Red() / 255.0, g = bg.Green() / 255.0, b = bg.Blue() / 255.0;
    const double    L = 0.2126 * r + 0.7152 * g + 0.0722 * b;
    return L < 0.5;
#endif
}

void ColorSlider::UpdatePaletteFromSystem()
{
    const bool dark = IsDarkAppearance();
    if (dark) {
        mTrackBg   = wxColour(43, 43, 43);
        mThumbFill = wxColour(31, 202, 99);
        mThumbRing = wxColour(219, 219, 219);
    } else {
        mTrackBg   = wxColour(225, 228, 233);
        mThumbFill = wxColour(21, 192, 89);
        mThumbRing = wxColour(255, 255, 255);
    }
}

void ColorSlider::OnSysColourChanged(wxSysColourChangedEvent& e)
{
    UpdatePaletteFromSystem();
    Refresh();
    e.Skip();
}