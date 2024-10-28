///////////////////////////////////////////////////////////////////////////////
// Project:     wxPDFJS
// File Name:   mainframe.cpp
// Purpose:     Implementation of application's main frame
// Author:      PB
// Created:     2024-10-27
// Copyright:   (c) 2024 PB
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#include <wx/wx.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/webview.h>

#if !wxCHECK_VERSION(3, 3, 0)
    #error "wxWidgets 3.3 or newer required"
#endif // #if !wxCHECK_VERSION(3, 3, 0)


#ifdef __WXMSW__
    #include <wx/msw/private/comptr.h>
#endif // #ifdef __WXMSW__

#include "mainframe.h"

#if USING_WEBVIEW_EDGE
    #include <WebView2.h>
    #include <webview2EnvironmentOptions.h>
#endif // #if USING_WEBVIEW_EDGE

#ifdef __WXGTK__
    #include <webkit2/webkit2.h>
#endif // #ifdef __WXGTK__


wxPDFJSMainFrame::wxPDFJSMainFrame(wxWindow* parent, const wxString& PDFJSAsetsFolder)
    : wxFrame(parent, wxID_ANY, wxTheApp->GetAppDisplayName())
{
#ifdef __WXMSW__
    SetIcons(wxIconBundle("appIcon", nullptr));
#endif
    SetMinClientSize(FromDIP(wxSize(600, 400)));

    wxMenu* menu = new wxMenu;

    menu->Append(wxID_OPEN, "Open PDF &file...\tCtrl+O");
    menu->Append(ID_SHOW_DEVTOOLS, "Show &DevTools\tCtrl+D");

    SetMenuBar(new wxMenuBar());
    GetMenuBar()->Append(menu, _("&Test"));

    Bind(wxEVT_MENU, &wxPDFJSMainFrame::OnFileOpen, this, wxID_OPEN);
    Bind(wxEVT_MENU, &wxPDFJSMainFrame::OnShowDevTools, this, ID_SHOW_DEVTOOLS);

    m_assetsFolder = PDFJSAsetsFolder;

    CreateWebView(this);
}

void wxPDFJSMainFrame::CreateWebView(wxWindow* parent)
{
    const wxString viewerURL = wxFileName::FileNameToURL(wxFileName(m_assetsFolder, "viewer.html"));

    m_webViewBackend = wxWebViewBackendDefault;
#if USING_WEBVIEW_EDGE
    m_webViewBackend = wxWebViewBackendEdge;
#endif

    wxWebViewConfiguration config = wxWebView::NewConfiguration(m_webViewBackend);

#if USING_WEBVIEW_EDGE
    ICoreWebView2EnvironmentOptions* webViewOptions =
        (ICoreWebView2EnvironmentOptions*)config.GetNativeConfiguration();
    webViewOptions->put_AdditionalBrowserArguments(L"--disable-web-security");
#endif

    m_webView = wxWebView::New(config);
    if ( !m_webView )
    {
        wxLogError("Could not create wxWebView.");
        return;
    }
    m_webView->Create(parent, wxID_ANY, viewerURL);
    m_webView->EnableContextMenu(false);
    m_webView->EnableHistory(false);

    m_webView->Bind(wxEVT_WEBVIEW_CREATED, [this](wxWebViewEvent&){ ConfigureWebView(); });
    m_webView->Bind(wxEVT_WEBVIEW_LOADED, &wxPDFJSMainFrame::OnWebViewPageLoaded, this);
    m_webView->Bind(wxEVT_WEBVIEW_ERROR, &wxPDFJSMainFrame::OnWebViewError, this);
}

void wxPDFJSMainFrame::ConfigureWebView()
{
    if ( m_webViewConfigured )
        return;

    void* nativeBackend = m_webView->GetNativeBackend();

    if ( !nativeBackend )
        return;

    m_webViewConfigured = true;

#if USING_WEBVIEW_EDGE
    ICoreWebView2* webView2 = static_cast<ICoreWebView2*>(nativeBackend);
    HRESULT hr;
    wxCOMPtr<ICoreWebView2Settings> settings;

    hr = webView2->get_Settings(&settings);
    if ( FAILED(hr) )
    {
        wxLogError(_("Could not obtain WebView2Settings (error code 0x%08lx)."), (long)hr);
        return;
    }
    settings->put_IsBuiltInErrorPageEnabled(FALSE);
    settings->put_IsZoomControlEnabled(FALSE);

    wxCOMPtr<ICoreWebView2Settings3> settings3;

    hr = settings->QueryInterface(wxIID_PPV_ARGS(ICoreWebView2Settings3, &settings3));
    if ( FAILED(hr) )
    {
        wxLogError(_("Could not obtain WebView2Settings3 (error code 0x%08lx)."), (long)hr);
        return;
    }
    settings3->put_AreBrowserAcceleratorKeysEnabled(FALSE);
#elif defined(__WXGTK__)
    WebKitWebView* wkv = static_cast<WebKitWebView*>(nativeBackend);

    if ( wkv )
    {
        /* const char* allowList[] = {"file://", "null", nullptr};

        webkit_web_view_set_cors_allowlist(wkv, allowList); */

        WebKitSettings* settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(wkv));

        if ( !settings )
            wxLogError("Could not obtain WebKitSettings to allow universal access from file URLs.");
        else
            webkit_settings_set_allow_universal_access_from_file_urls(settings, true);
    }
#endif // #if USING_WEBVIEW_EDGE
}

void wxPDFJSMainFrame::OnShowDevTools(wxCommandEvent&)
{
    m_webView->ShowDevTools();
}

void wxPDFJSMainFrame::OnFileOpen(wxCommandEvent&)
{
    const wxString fileName = wxLoadFileSelector("Select file", "PDF Files (*.pdf)|*.pdf", "", this);

    if ( fileName.empty() )
        return;

    const wxString PDFURL = wxFileName::FileNameToURL(wxFileName(fileName));

    /*
    // works with Edge, does not work with WebViewGTK
    const wxString viewerURL = wxFileName::FileNameToURL(wxFileName(m_assetsFolder, "viewer.html"));
    const wxString url = wxString::Format("%s?file=%s", viewerURL, PDFURL);

    m_webView->LoadURL(url);
    */

    /*
    // works with Edge, does not work with WebViewGTK
    const wxString script = wxString::Format("PDFViewerApplication.open({url:'%s'});", PDFURL);

    m_webView->RunScriptAsync(script);
    */

    // works with Edge and WebViewGTK
    const wxString script = R"(
        const url = '%s';
        const fileName = '%s';

        fetch(url)
            .then(response => response.blob())
            .then(PDFBlob => {
                const PDFURL = URL.createObjectURL(PDFBlob);
                const openArgs = { url: PDFURL, originalUrl: fileName };

                console.log(openArgs);
                PDFViewerApplication.open(openArgs);
            })
            .catch(error => console.error("Error fetching PDF:", error));
        )";

    m_webView->RunScriptAsync(wxString::Format(script, PDFURL, wxFileName(fileName).GetFullName()));
}

void wxPDFJSMainFrame::OnWebViewPageLoaded(wxWebViewEvent&)
{
    ConfigureWebView();
}

void wxPDFJSMainFrame::OnWebViewError(wxWebViewEvent&)
{
    wxLogError("Could not load PDF.js.");
    //m_webView->SetPage(R"(<!DOCTYPE html><html><head><meta charset="utf-8"/></head><body>)", "");
}