///////////////////////////////////////////////////////////////////////////////
// Project:     wxPDFJS
// Home:        https://github.com/PBfordev/wxecharts
// File Name:   wxpdfjs.h
// Purpose:     Declaration of the application class
// Author:      PB
// Created:     2024-10-27
// Copyright:   (c) 2024 PB
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/app.h>

class wxPDFJSApp : public wxApp
{
private:
    bool OnInit() override;
    int OnExit() override;

    wxString GetAssetsFolder();
};

wxDECLARE_APP(wxPDFJSApp);