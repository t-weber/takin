/*
 * Connection to Sics
 * @author tweber
 * @date 26-aug-2015
 * @license GPLv2
 */

#include "sics.h"
#include "tlibs/string/string.h"
#include "tlibs/log/log.h"
#include "tlibs/helper/misc.h"
#include "tlibs/time/chrono.h"
#include "libs/globals.h"
#include <boost/algorithm/string.hpp>

using t_real = t_real_glob;


SicsCache::SicsCache(QSettings* pSettings) : m_pSettings(pSettings)
{
	m_tcp.add_connect(boost::bind(&SicsCache::slot_connected, this, _1, _2));
	m_tcp.add_disconnect(boost::bind(&SicsCache::slot_disconnected, this, _1, _2));
	m_tcp.add_receiver(boost::bind(&SicsCache::slot_receive, this, _1));
}

SicsCache::~SicsCache()
{
	disconnect();
}

void SicsCache::connect(const std::string& strHost, const std::string& strPort,
	const std::string& strUser, const std::string& strPass)
{
	if(m_pSettings && m_pSettings->contains("net/poll"))
		m_iPollRate = m_pSettings->value("net/poll").value<unsigned int>();

	m_mapCache.clear();
	emit cleared_cache();

	m_strUser = strUser;
	m_strPass = strPass;

	if(!m_tcp.connect(strHost, strPort))
		return;
}

void SicsCache::disconnect()
{
	m_bPollerActive.store(false);
	if(m_pthPoller && m_pthPoller->joinable())
		m_pthPoller->join();
	if(m_pthPoller)
	{
		delete m_pthPoller;
		m_pthPoller = nullptr;
	}

	m_tcp.disconnect();
}

void SicsCache::refresh()
{}

void SicsCache::start_poller()
{
	m_bPollerActive.store(true);

	m_pthPoller = new std::thread([this]()
	{
		if(m_strUser.size() || m_strPass.size())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			m_tcp.write(m_strUser + " " + m_strPass + "\n");
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

		while(m_bPollerActive.load())
		{
			std::string strMsg;
			strMsg += "DM\nDA\n";
			strMsg += "AS\nBS\nCS\nAA\nBB\nCC\n";
			strMsg += "AX\nAY\nAZ\nBX\nBY\nBZ\n";
			strMsg += "A2\nA3\nA4\nA6\n";

			m_tcp.write(strMsg);
			std::this_thread::sleep_for(std::chrono::milliseconds(m_iPollRate));
		}
	});
}

void SicsCache::slot_connected(const std::string& strHost, const std::string& strSrv)
{
	tl::log_info("Connected to ", strHost, " on port ", strSrv, ".");
	start_poller();

	QString qstrHost = strHost.c_str();
	QString qstrSrv = strSrv.c_str();
	emit connected(qstrHost, qstrSrv);
}

void SicsCache::slot_disconnected(const std::string& strHost, const std::string& strSrv)
{
	tl::log_info("Disconnected from ", strHost, " on port ", strSrv, ".");
	m_bPollerActive.store(false);
	emit disconnected();
}

void SicsCache::slot_receive(const std::string& str)
{
#ifndef NDEBUG
	tl::log_debug("Received: ", str);
#endif
	if(str=="OK" || str=="Login OK")
		return;

	if(str.substr(0, 5) == "ERROR")
	{
		tl::log_err("Sics replied with: ", str);
		tl::log_err("Stopping poller...");
		m_bPollerActive.store(false);
		//this->disconnect();
		return;
	}

	std::pair<std::string, std::string> pairKeyVal = tl::split_first<std::string>(str, "=", 1);
	if(pairKeyVal.second == "")
	{
		tl::log_err("Invalid net reply: \"", str, "\"");
		return;
	}


	const std::string& strKey = pairKeyVal.first;
	const std::string& strVal = pairKeyVal.second;

	if(strVal.length() == 0)
		return;

	CacheVal cacheval;
	cacheval.strVal = strVal;
	boost::to_upper(cacheval.strVal);
	cacheval.dTimestamp = tl::epoch<t_real>();

	/*// mark special entries
	if(strKey == m_strTimer)
		cacheval.ty = CacheValType::TIMER;
	else if(strKey == m_strPreset)
		cacheval.ty = CacheValType::PRESET;
	else if(strKey == m_strCtr)
		cacheval.ty = CacheValType::COUNTER;*/

	m_mapCache[strKey] = cacheval;

	//std::cout << strKey << " = " << strVal << std::endl;
	emit updated_cache_value(strKey, cacheval);


	CrystalOptions crys;
	TriangleOptions triag;

	// monochromator
	if(tl::str_is_equal<std::string>(strKey, "A2", false))
	{
		triag.bChangedMonoTwoTheta = 1;
		triag.dMonoTwoTheta = tl::d2r(tl::str_to_var<t_real>(strVal));
	}
	// analyser
	else if(tl::str_is_equal<std::string>(strKey, "A6", false))
	{
		triag.bChangedAnaTwoTheta = 1;
		triag.dAnaTwoTheta = tl::d2r(tl::str_to_var<t_real>(strVal));
	}
	// sample
	else if(tl::str_is_equal<std::string>(strKey, "A4", false))
	{
		triag.bChangedTwoTheta = 1;
		triag.dTwoTheta = tl::d2r(tl::str_to_var<t_real>(strVal));
	}
	else if(tl::str_is_equal<std::string>(strKey, "A3", false))
	{
		triag.bChangedAngleKiVec0 = 1;
		triag.dAngleKiVec0 = -tl::d2r(tl::str_to_var<t_real>(strVal));
	}
	// lattice constants and angles
	else if(tl::str_is_equal_to_either<std::string>(strKey, {"AS", "BS", "CS", "AA", "BB", "CC"}, false))
	{
		decltype(m_mapCache)::const_iterator iterAS = m_mapCache.find("AS");
		decltype(m_mapCache)::const_iterator iterBS = m_mapCache.find("BS");
		decltype(m_mapCache)::const_iterator iterCS = m_mapCache.find("CS");
		decltype(m_mapCache)::const_iterator iterAA = m_mapCache.find("AA");
		decltype(m_mapCache)::const_iterator iterBB = m_mapCache.find("BB");
		decltype(m_mapCache)::const_iterator iterCC = m_mapCache.find("CC");

		if(iterAS==m_mapCache.end() || iterBS==m_mapCache.end() || iterCS==m_mapCache.end()
			|| iterAA==m_mapCache.end() || iterBB==m_mapCache.end() || iterCC==m_mapCache.end())
				return;

		crys.bChangedLattice = crys.bChangedLatticeAngles = 1;
		crys.dLattice[0] = tl::str_to_var<t_real>(iterAS->second.strVal);
		crys.dLattice[1] = tl::str_to_var<t_real>(iterBS->second.strVal);
		crys.dLattice[2] = tl::str_to_var<t_real>(iterCS->second.strVal);
		crys.dLatticeAngles[0] = tl::str_to_var<t_real>(iterAA->second.strVal);
		crys.dLatticeAngles[1] = tl::str_to_var<t_real>(iterBB->second.strVal);
		crys.dLatticeAngles[2] = tl::str_to_var<t_real>(iterCC->second.strVal);
	}
	// orientation reflexes
	else if(tl::str_is_equal_to_either<std::string>(strKey, {"AX", "AY", "AZ", "BX", "BY", "BZ"}, false))
	{
		decltype(m_mapCache)::const_iterator iterAX = m_mapCache.find("AX");
		decltype(m_mapCache)::const_iterator iterAY = m_mapCache.find("AY");
		decltype(m_mapCache)::const_iterator iterAZ = m_mapCache.find("AZ");
		decltype(m_mapCache)::const_iterator iterBX = m_mapCache.find("BX");
		decltype(m_mapCache)::const_iterator iterBY = m_mapCache.find("BY");
		decltype(m_mapCache)::const_iterator iterBZ = m_mapCache.find("BZ");

		if(iterAX==m_mapCache.end() || iterAY==m_mapCache.end() || iterAZ==m_mapCache.end()
			|| iterBX==m_mapCache.end() || iterBY==m_mapCache.end() || iterBZ==m_mapCache.end())
				return;

		crys.bChangedPlane1 = crys.bChangedPlane2 = 1;
		crys.dPlane1[0] = tl::str_to_var<t_real>(iterAX->second.strVal);
		crys.dPlane1[1] = tl::str_to_var<t_real>(iterAY->second.strVal);
		crys.dPlane1[2] = tl::str_to_var<t_real>(iterAZ->second.strVal);
		crys.dPlane2[0] = tl::str_to_var<t_real>(iterBX->second.strVal);
		crys.dPlane2[1] = tl::str_to_var<t_real>(iterBY->second.strVal);
		crys.dPlane2[2] = tl::str_to_var<t_real>(iterBZ->second.strVal);
	}
	else if(tl::str_is_equal<std::string>(strKey, "DM", false))
	{
		triag.dMonoD = tl::str_to_var<t_real>(strVal);
		triag.bChangedMonoD = 1;
	}
	else if(tl::str_is_equal<std::string>(strKey, "DA", false))
	{
		triag.dAnaD = tl::str_to_var<t_real>(strVal);
		triag.bChangedAnaD = 1;
	}

	// need to make crys & triag member variables and protected by a mutex
	// for the following:
	//if(tl::has_map_all_keys<decltype(m_mapCache)>(m_mapCache,
	//	{"A2", "A6", "A4", "A3", "AS", "BS", "CS", "AA", "BB", "CC",
	//	"AX", "AY", "AZ", "BX", "BY", "BZ", "DM", "DA"}))
		emit vars_changed(crys, triag);
}

#include "sics.moc"
