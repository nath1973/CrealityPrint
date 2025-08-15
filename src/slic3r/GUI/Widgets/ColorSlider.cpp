#include "ColorSlider.hpp"

wxBEGIN_EVENT_TABLE(ColorSlider, wxSlider)
    EVT_PAINT(ColorSlider::OnPaint)
    EVT_ERASE_BACKGROUND(ColorSlider::OnEraseBackground)
    EVT_SCROLL_THUMBTRACK(ColorSlider::OnScrollUpdate)
    EVT_SCROLL_CHANGED(ColorSlider::OnScrollUpdate) 
    EVT_SCROLL_THUMBRELEASE(ColorSlider::OnScrollUpdate) 
    EVT_SET_FOCUS(ColorSlider::OnFocusChange) 
    EVT_KILL_FOCUS(ColorSlider::OnFocusChange)
                    wxEND_EVENT_TABLE()


    ColorSlider::ColorSlider(wxWindow* parent, wxWindowID id, int value, int minValue, int maxValue, const wxPoint& pos, const wxSize& size, long style)
    : wxSlider(parent, id, value, minValue, maxValue, pos, size, style)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT); 
    SetBackgroundColour(wxColour(75, 75, 77));
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
    gc->SetBrush(wxBrush(wxColour(52, 52, 54)));
    gc->DrawRoundedRectangle(THUMB_RADIUS, trackY, width - 2 * THUMB_RADIUS, trackHeight, trackRadius);

    gc->SetBrush(wxBrush(wxColour(31, 202, 99))); 
    gc->DrawRoundedRectangle(THUMB_RADIUS, trackY, thumbX - THUMB_RADIUS, trackHeight, trackRadius);

    gc->SetBrush(wxBrush(wxColour(31, 202, 99))); 
    gc->SetPen(wxPen(*wxWHITE, 2));            
    gc->DrawEllipse(thumbX - THUMB_RADIUS, thumbY - THUMB_RADIUS, THUMB_RADIUS * 2, THUMB_RADIUS * 2);
}