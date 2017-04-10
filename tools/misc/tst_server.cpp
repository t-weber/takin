/**
 * test instrument server
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date apr-2016
 * @license GPLv2
 * clang -o tst_server -I. -I../.. ../../tools/misc/tst_server.cpp ../../tlibs/net/tcp.cpp ../../tlibs/log/log.cpp -lstdc++ -std=c++11 -lboost_system -lboost_iostreams -lpthread -lm
 */

#include "tlibs/net/tcp.h"
#include "tlibs/log/log.h"
#include "tlibs/string/string.h"
#include "tlibs/file/prop.h"


using namespace tl;

static void disconnected(const std::string& strHost, const std::string& strSrv)
{
	log_info("Disconnected.");
}

static void connected(unsigned short iPort)
{
	log_info("Listening on port ", iPort, ".");
}


int main(int argc, char** argv)
{
	if(argc < 2)
	{
		log_err("Usage: ", argv[0], " <port>");
		return -1;
	}


	TcpTxtServer<> server;
	server.add_disconnect(disconnected);
	server.add_server_start(connected);
	server.add_receiver([&server](const std::string& strMsg)
	{
		log_info("Received: ", strMsg);
		tl::Prop<> prop;
		if(prop.Load("replies.ini"))
		{
			std::vector<std::string> vecMsg;
			tl::get_tokens<std::string>(strMsg, std::string(" ,"), vecMsg);
			for(std::string& strTok : vecMsg)
			{
				tl::trim(strTok);
				if(strTok == "") continue;

				std::string strVal = prop.Query<std::string>("replies/" + strTok, "0");
				server.write(strTok + "=" + strVal + "\n");
			}
		}
	});


	if(!server.start_server(tl::str_to_var<unsigned short>(std::string(argv[1]))))
	{
		log_err("Cannot start server.");
		return -1;
	}

	log_info("Waiting...");
	server.wait();

	/*std::string strMsg;
	while(server.is_connected())
	{
		std::getline(std::cin, strMsg);
		if(strMsg == "!exit!")
			break;

		strMsg+="\n";
		server.write(strMsg);
	}*/
	return 0;
}
