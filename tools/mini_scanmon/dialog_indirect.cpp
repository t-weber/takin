// gcc -o dialog dialog_indirect.cpp -std=c++11 -lstdc++ -lboost_iostreams

#include "dialog.h"
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <chrono>
#include <thread>

namespace ios = boost::iostreams;


static FILE *pipeProg = nullptr;
static boost::iostreams::file_descriptor_sink *pfds = nullptr;
static boost::iostreams::stream_buffer<boost::iostreams::file_descriptor_sink> *psbuf = nullptr;
static std::ostream *postrProg = nullptr;

bool open_progress(const std::string& strTitle)
{
	close_progress();

	pipeProg = (FILE*)popen(("dialog --title \"" + strTitle + "\" --gauge \"\" 10 50 0").c_str(), "w");
	if(!pipeProg) return false;
	pfds = new ios::file_descriptor_sink(fileno(pipeProg), ios::close_handle);
	psbuf = new ios::stream_buffer<ios::file_descriptor_sink>(*pfds);
	postrProg = new std::ostream(psbuf);

	return true;
}

void close_progress()
{
	if(pipeProg)
	{
		pclose(pipeProg);
		pipeProg = nullptr;
	}

	if(postrProg) { delete postrProg; postrProg = nullptr; }
	if(psbuf) { delete psbuf; psbuf = nullptr; }
	if(pfds) { delete pfds; pfds = nullptr; }
}

void set_progress(int iPerc, const std::string& strTxt)
{
	if(!postrProg) return;
	(*postrProg) << "XXX\n" << iPerc << "\n"
		<< strTxt << "\nXXX"
		<< std::endl;
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

