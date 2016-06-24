// gcc -o dialog dialog.cpp -std=c++11 -lstdc++ -ldialog

#include "dialog.h"
#include <dialog/dialog.h>
#include <chrono>
#include <thread>


static void *g_pProgress = nullptr;
std::string g_strTitle;


bool open_progress(const std::string& strTitle)
{
	g_strTitle = strTitle;
	close_progress();

	init_dialog(stdin, stdout);
	dlg_clear();

	g_pProgress = dlg_allocate_gauge(g_strTitle.c_str(), "", 10, 50, 0);
	if(g_pProgress)
		return true;
	return false;
}

void close_progress()
{
	if(g_pProgress)
	{
		dlg_free_gauge(g_pProgress);
		g_pProgress = nullptr;
	}
	end_dialog();
}

void set_progress(int iPerc, const std::string& strTxt)
{
	if(!g_pProgress) return;

	g_pProgress = dlg_reallocate_gauge(g_pProgress, g_strTitle.c_str(), strTxt.c_str(), 10, 50, iPerc);
	dlg_update_gauge(g_pProgress, iPerc);
}


/*int main(int argc, char** argv)
{
	open_progress("Test");
	set_progress(10, "abc\ndef\nghi");
	std::this_thread::sleep_for(std::chrono::milliseconds(250));
	set_progress(20, "xyz\n123\n456");
	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	close_progress();
	return 0;
}*/
