/*
 * Connection to Sics
 * @author tweber
 * @date 26-aug-2015
 * @license GPLv2
 */

#ifndef __SICS_CONN_H__
#define __SICS_CONN_H__

#include "net.h"
#include "tlibs/net/tcp.h"

#include <QSettings>
#include <map>
#include <vector>
#include <thread>
#include <atomic>


class SicsCache : public NetCache
{ Q_OBJECT
	protected:
		QSettings* m_pSettings = 0;

		tl::TcpTxtClient<> m_tcp;
		t_mapCacheVal m_mapCache;

		std::string m_strUser, m_strPass;

		std::atomic<bool> m_bPollerActive;
		std::thread *m_pthPoller = nullptr;

		unsigned int m_iPollRate = 750;

	protected:
		// endpoints of the TcpClient signals
		void slot_connected(const std::string& strHost, const std::string& strSrv);
		void slot_disconnected(const std::string& strHost, const std::string& strSrv);
		void slot_receive(const std::string& str);

		void start_poller();

	public:
		SicsCache(QSettings* pSettings=0);
		virtual ~SicsCache();

		virtual void connect(const std::string& strHost, const std::string& strPort,
			const std::string& strUser, const std::string& strPass) override;
		virtual void disconnect() override;
		virtual void refresh() override;
};

#endif
