/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "lib/server.h"
#include "server_dialog.h"
#include "wx_util.h"

using boost::shared_ptr;

ServerDialog::ServerDialog (wxWindow* parent)
	: wxDialog (parent, wxID_ANY, _("Server"))
{
	wxFlexGridSizer* table = new wxFlexGridSizer (2, DCPOMATIC_SIZER_X_GAP, DCPOMATIC_SIZER_Y_GAP);
	table->AddGrowableCol (1, 1);

	add_label_to_sizer (table, this, _("Host name or IP address"), true);
	_host = new wxTextCtrl (this, wxID_ANY);
	table->Add (_host, 1, wxEXPAND);

	add_label_to_sizer (table, this, _("Threads to use"), true);
	_threads = new wxSpinCtrl (this, wxID_ANY);
	table->Add (_threads, 1, wxEXPAND);

	_threads->SetRange (0, 256);

	wxBoxSizer* overall_sizer = new wxBoxSizer (wxVERTICAL);
	overall_sizer->Add (table, 1, wxEXPAND | wxALL, 6);

	wxSizer* buttons = CreateSeparatedButtonSizer (wxOK);
	if (buttons) {
		overall_sizer->Add (buttons, wxSizerFlags().Expand().DoubleBorder());
	}

	SetSizer (overall_sizer);
	overall_sizer->Layout ();
	overall_sizer->SetSizeHints (this);
}

void
ServerDialog::set (ServerDescription server)
{
	_host->SetValue (std_to_wx (server.host_name ()));
	_threads->SetValue (server.threads ());
}

ServerDescription
ServerDialog::get () const
{
	ServerDescription server;
	server.set_host_name (wx_to_std (_host->GetValue ()));
	server.set_threads (_threads->GetValue ());
	return server;
}

