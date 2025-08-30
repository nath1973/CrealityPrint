#ifndef slic3r_GUI_ColorSlider_hpp_
#define slic3r_GUI_ColorSlider_hpp_

#include "../wxExtensions.hpp"
#include <wx/wx.h>
#include <wx/slider.h>
#include <wx/dcgraph.h>
#include <wx/settings.h>

class ColorSlider : public wxSlider
{
public:
    ColorSlider(wxWindow*      parent,
                wxWindowID     id,
                int            value,
                int            minValue,
                int            maxValue,
                const wxPoint& pos   = wxDefaultPosition,
                const wxSize&  size  = wxDefaultSize,
                long           style = wxSL_HORIZONTAL);
    void SetValue(int value) override;

private:
    void OnPaint(wxPaintEvent& event);
    void OnEraseBackground(wxEraseEvent& event);
    void             OnScrollUpdate(wxScrollEvent& event);
    void             OnTextChanged(wxCommandEvent& event);
    void             OnFocusChange(wxFocusEvent& event);
    void             OnSysColourChanged(wxSysColourChangedEvent& event);

    void             UpdatePaletteFromSystem();
    bool             IsDarkAppearance() const;

    wxColour         mTrackBg;
    wxColour         mThumbFill;
    wxColour         mThumbRing;

    static const int THUMB_RADIUS = 7;
    wxDECLARE_EVENT_TABLE();
};
#endif // !slic3r_GUI_ColorSlider_hpp_