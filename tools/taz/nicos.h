/**
 * Connection to Nicos
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 27-aug-2014
 * @license GPLv2
 */

#ifndef __NICOS_H__
#define __NICOS_H__

#include "net.h"
#include "tlibs/net/tcp.h"

#include <QSettings>
#include <map>
#include <vector>

class NicosCache : public NetCache
{ Q_OBJECT
	protected:
		QSettings* m_pSettings = 0;

		tl::TcpTxtClient<> m_tcp;
		std::vector<std::string> m_vecKeys;
		t_mapCacheVal m_mapCache;

		bool m_bFlipOrient2 = 1;
		bool m_bSthCorr = 0;

		// endpoints of the TcpClient signals
		void slot_connected(const std::string& strHost, const std::string& strSrv);
		void slot_disconnected(const std::string& strHost, const std::string& strSrv);
		void slot_receive(const std::string& str);

	public:
		NicosCache(QSettings* pSettings=0);
		virtual ~NicosCache();

		virtual void connect(const std::string& strHost, const std::string& strPort,
			const std::string& strUser, const std::string& strPass) override;
		virtual void disconnect() override;
		virtual void refresh() override;

		void AddKey(const std::string& strKey);
		void ClearKeys();

		void RefreshKeys();
		void RegisterKeys();
		void UnregisterKeys();

	protected:
		// device names
		std::string m_strSampleName;
		std::string m_strSampleLattice, m_strSampleAngles;
		std::string m_strSampleOrient1, m_strSampleOrient2;
		std::string m_strSampleSpacegroup;
		std::string m_strSamplePsi0, m_strSampleTheta, m_strSample2Theta;
		std::string m_strMonoTheta, m_strMono2Theta, m_strMonoD;
		std::string m_strAnaTheta, m_strAna2Theta, m_strAnaD;
		std::string m_strTimer, m_strPreset, m_strCtr;
};

#endif
