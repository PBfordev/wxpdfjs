///////////////////////////////////////////////////////////////////////////////
// Project:     wxPDFJS
// File Name:   mainframe.h
// Purpose:     Declaration of application's main frame
// Author:      PB
// Created:     2024-10-27
// Copyright:   (c) 2024 PB
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/frame.h>

#if !wxUSE_WEBVIEW
  #error "wxWidgets must be built with a support for wxWebView"
#endif

#ifdef __WXMSW__
    #if !wxUSE_WEBVIEW_EDGE
        #error "On Windows, wxUSE_WEBVIEW_EDGE must be set to 1 (or ON) when building wxWidgets"
    #endif// !wxUSE_WEBVIEW_EDGE
    #define USING_WEBVIEW_EDGE 1
#else // #ifdef __WXMSW__
    #define USING_WEBVIEW_EDGE 0
#endif

class wxWebView;
class wxWebViewEvent;

class wxPDFJSMainFrame : public wxFrame
{
public:
    wxPDFJSMainFrame(wxWindow* parent, const wxString& assetsFolder);
private:
    enum
    {
        ID_SHOW_DEVTOOLS = wxID_HIGHEST + 10,
    };

    wxWebView* m_webView{nullptr};
    bool m_webViewConfigured{false};
    wxString m_webViewBackend;
    wxString m_assetsFolder;

    void CreateWebView(wxWindow* parent);
    void ConfigureWebView();

    void OnFileOpen(wxCommandEvent&);
    void OnShowDevTools(wxCommandEvent&);

    void OnWebViewPageLoaded(wxWebViewEvent&);
    void OnWebViewError(wxWebViewEvent&);
    void OnWebViewScriptResult(wxWebViewEvent&);
};