﻿/****************************************************************************
 *   Copyright (C) 2014 by Jerry			                           		*
 *                                                                        	*
 *   This file is part of IBrowser.                                        	*
 *                                                                         	*
 *   IBrowser is private software.                                         	*
 *   program.  If not, see <https://github.com/jerrykk/IBrowser>.          	*
 *                                                                         	*
 *   IBrowser website: https://github.com/jerrykk/IBrowser                 	*
 ****************************************************************************/

// ibrowser
#include "ibrowser/ibrowserhandler.h"

// vc
#include <sstream>
#include <string>

// cef
#include "ibrowser/util.h"
#include "include/cef_app.h"
#include "include/cef_runnable.h"

namespace ibrowser
{
	IBrowserHandler* IBrowserHandler::m_instance = NULL;
	CefRefPtr<IBrowserHandler>	IBrowserHandler::m_handler = NULL;

	IBrowserHandler::IBrowserHandler() 
		:	m_mainhwnd(NULL), 
			m_browser(NULL),
			m_browserId(0),
			m_bIsClosing(false),
			browser_list_(NULL)
	{
		
	}

	IBrowserHandler::~IBrowserHandler() 
	{
		m_instance = NULL;
	}

	// static
	IBrowserHandler* IBrowserHandler::GetInstance() 
	{
		return m_instance;
	}

	CefRefPtr<IBrowserHandler> IBrowserHandler::GetHandler()
	{
		return m_handler;
	}

	void IBrowserHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) 
	{
		REQUIRE_UI_THREAD();

		// Add to the list of existing browsers.
		browser_list_.push_back(browser);
	}

	void IBrowserHandler::OnTitleChange(CefRefPtr<CefBrowser> browser,
									  const CefString& title) 
	{
										  REQUIRE_UI_THREAD();

										  CefWindowHandle hwnd = browser->GetHost()->GetWindowHandle();
										  SetWindowText(hwnd, std::wstring(title).c_str());
	}

	void IBrowserHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) 
	{
		REQUIRE_UI_THREAD();

		// Remove from the list of existing browsers.
		BrowserList::iterator bit = browser_list_.begin();
		for (; bit != browser_list_.end(); ++bit) 
		{
			if ((*bit)->IsSame(browser)) 
			{
				browser_list_.erase(bit);
				break;
			}
		}

		if (browser_list_.empty()) 
		{
			// All browser windows have closed. Quit the application message loop.
			CefQuitMessageLoop();
		}
	}

	void IBrowserHandler::OnLoadError(	CefRefPtr<CefBrowser>	browser,
										CefRefPtr<CefFrame>		frame,
										ErrorCode				errorCode,
										const CefString			&errorText,
										const CefString			&failedUrl) 
	{
		REQUIRE_UI_THREAD();

		// Don't display an error for downloaded files.
		if (errorCode == ERR_ABORTED)
			return;

		// Display a load error message.
		std::stringstream ss;
		ss << "<html><body bgcolor=\"white\">"
			"<h2>Failed to load URL " << std::string(failedUrl) <<
			" with error " << std::string(errorText) << " (" << errorCode <<
			").</h2></body></html>";
		frame->LoadString(ss.str(), failedUrl);
	}

	void IBrowserHandler::CloseAllBrowsers(bool force_close) 
	{
		if (!CefCurrentlyOn(TID_UI)) {
			// Execute on the UI thread.
			CefPostTask(TID_UI,
				NewCefRunnableMethod(this, &IBrowserHandler::CloseAllBrowsers,
				force_close));
			return;
		}

		if (browser_list_.empty())
			return;

		BrowserList::const_iterator it = browser_list_.begin();
		for (; it != browser_list_.end(); ++it)
			(*it)->GetHost()->CloseBrowser(force_close);
	}

	CefRefPtr<CefBrowser> IBrowserHandler::GetBrowser() 
	{ 
		return m_browser; 
	}

	void IBrowserHandler::SetMainHwnd(CefWindowHandle hwnd) 
	{
		AutoLock lock_scope(this);
		m_mainhwnd = hwnd;
	}
	
	bool IBrowserHandler::IsClosing()
	{
		 return m_bIsClosing;
	}
}