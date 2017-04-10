/**
 * test instrument client
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 2014
 * @license GPLv2
 * clang -o tst_server -I. -I../.. ../../tools/misc/tst_server.cpp ../../tlibs/net/tcp.cpp ../../tlibs/log/log.cpp -lstdc++ -std=c++11 -lboost_system -lboost_iostreams -lpthread -lm
 */

/*
 * clang -o tst_client -I../.. tst_client.cpp ../../tlibs/net/tcp.cpp ../../tlibs/log/log.cpp -lstdc++ -std=c++11 -lboost_system -lpthread -lm
 */

#include <fstream>
#include "../../tlibs/net/tcp.h"
#include "../../tlibs/log/log.h"

using namespace tl;

struct TstOut
{
	std::ofstream ofstr;

	TstOut() : ofstr("tst_out.txt")
	{}

	void print(const std::string& str)
	{
		//ofstr << str << std::endl;
		std::cout << "out: " << str << std::endl;
	}
};

static void disconnected(const std::string& strHost, const std::string& strSrv)
{
	log_info("Disconnected from ", strHost, " on port ", strSrv, ".");
}

static void connected(const std::string& strHost, const std::string& strSrv)
{
	log_info("Connected to ", strHost, " on port ", strSrv, ".");
}

static void received(const std::string& strMsg)
{
	log_info("Received: ", strMsg, ".");
}


int main(int argc, char** argv)
{
	if(argc < 3)
	{
		std::cerr << "Usage: " << argv[0] << " <server> <port>" << std::endl;
		return -1;
	}


	TcpTxtClient<> client;
	//TstOut tstout;
	//client.add_receiver(boost::bind(&TstOut::print, &tstout, _1));
	client.add_receiver(received);
	client.add_disconnect(disconnected);
	client.add_connect(connected);


	if(!client.connect(argv[1], argv[2]))
	{
		log_err("Cannot connect.");
		return -1;
	}

	std::string strMsg;
	while(client.is_connected())
	{
		std::getline(std::cin, strMsg);
		if(strMsg == "!exit!")
			break;

		strMsg+="\n";
		client.write(strMsg);
	}

	return 0;
}
