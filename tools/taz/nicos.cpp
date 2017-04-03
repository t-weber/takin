/**
 * Connection to Nicos
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 27-aug-2014
 * @license GPLv2
 */

#include "nicos.h"
#include "tlibs/string/string.h"
#include "tlibs/log/log.h"
#include "tlibs/helper/py.h"
#include "libs/globals.h"

using t_real = t_real_glob;


NicosCache::NicosCache(QSettings* pSettings) : m_pSettings(pSettings)
{
	// map device names from settings
	// first: settings name, second: pointer to string receiving device name
	std::vector<std::pair<std::string, std::string*>> vecStrings =
	{
		{"sample_name", &m_strSampleName},
		{"lattice", &m_strSampleLattice},
		{"angles", &m_strSampleAngles},
		{"orient1", &m_strSampleOrient1},
		{"orient2", &m_strSampleOrient2},
		{"spacegroup", &m_strSampleSpacegroup},
		{"psi0", &m_strSamplePsi0},

		{"stheta", &m_strSampleTheta},
		{"s2theta", &m_strSample2Theta},

		{"mtheta", &m_strMonoTheta},
		{"m2theta", &m_strMono2Theta},
		{"mono_d", &m_strMonoD},

		{"atheta", &m_strAnaTheta},
		{"a2theta", &m_strAna2Theta},
		{"ana_d", &m_strAnaD},

		{"timer", &m_strTimer},
		{"preset", &m_strPreset},
		{"counter", &m_strCtr},
	};

	// fill in device names from settings
	for(const std::pair<std::string, std::string*>& pair : vecStrings)
	{
		std::string strKey = std::string("net/") + pair.first;
		if(!m_pSettings->contains(strKey.c_str()))
			continue;

		*pair.second = m_pSettings->value(strKey.c_str(), pair.second->c_str()).
			toString().toStdString();
	}

	m_bFlipOrient2 = m_pSettings->value("net/flip_orient2", true).toBool();
	m_bSthCorr = m_pSettings->value("net/sth_stt_corr", false).toBool();

	// all final device names
	m_vecKeys = std::vector<std::string>
	({
		m_strSampleName,
		m_strSampleLattice, m_strSampleAngles,
		m_strSampleOrient1, m_strSampleOrient2,
		m_strSampleSpacegroup,
		m_strSamplePsi0, m_strSampleTheta, m_strSample2Theta,

		m_strMonoD, m_strMonoTheta, m_strMono2Theta,
		m_strAnaD, m_strAnaTheta, m_strAna2Theta,

		m_strTimer, m_strPreset, m_strCtr,

		// additional info fields (not needed for calculation)
		//"logbook/remark",
		//"nicos/mira/value",
	});

	m_tcp.add_connect(boost::bind(&NicosCache::slot_connected, this, _1, _2));
	m_tcp.add_disconnect(boost::bind(&NicosCache::slot_disconnected, this, _1, _2));
	m_tcp.add_receiver(boost::bind(&NicosCache::slot_receive, this, _1));
}

NicosCache::~NicosCache()
{
	disconnect();
}

void NicosCache::connect(const std::string& strHost, const std::string& strPort,
	const std::string& strUser, const std::string& strPass)
{
	m_mapCache.clear();
	emit cleared_cache();

	if(!m_tcp.connect(strHost, strPort))
		return;
}

void NicosCache::disconnect()
{
	UnregisterKeys();
	m_tcp.disconnect();
}

void NicosCache::refresh()
{
	RefreshKeys();
}

void NicosCache::AddKey(const std::string& strKey)
{
	// connection is asynchronous -> cannot yet request data from server
	//if(!m_tcp.is_connected())
	//	return false;

	m_vecKeys.push_back(strKey);
}

void NicosCache::ClearKeys()
{
	m_vecKeys.clear();
}

void NicosCache::RefreshKeys()
{
	if(!m_tcp.is_connected()) return;

	std::string strMsg;
	for(const std::string& strKey : m_vecKeys)
		strMsg += "@"+strKey+"?\n";
	m_tcp.write(strMsg);
}

void NicosCache::RegisterKeys()
{
	if(!m_tcp.is_connected()) return;

	std::string strMsg;
	for(const std::string& strKey : m_vecKeys)
		strMsg += "@"+strKey+":\n";
	m_tcp.write(strMsg);
}

void NicosCache::UnregisterKeys()
{
	if(!m_tcp.is_connected()) return;

	std::string strMsg;
	for(const std::string& strKey : m_vecKeys)
		strMsg += strKey+"|\n";
	m_tcp.write(strMsg);
}


void NicosCache::slot_connected(const std::string& strHost, const std::string& strSrv)
{
	tl::log_info("Connected to ", strHost, " on port ", strSrv, ".");

	QString qstrHost = strHost.c_str();
	QString qstrSrv = strSrv.c_str();
	emit connected(qstrHost, qstrSrv);

	RegisterKeys();
	RefreshKeys();
}

void NicosCache::slot_disconnected(const std::string& strHost, const std::string& strSrv)
{
	tl::log_info("Disconnected from ", strHost, " on port ", strSrv, ".");

	emit disconnected();
}

void NicosCache::slot_receive(const std::string& str)
{
#ifndef NDEBUG
	tl::log_debug("Received: ", str);
#endif
	std::pair<std::string, std::string> pairTimeVal = tl::split_first<std::string>(str, "@", 1);
	std::pair<std::string, std::string> pairKeyVal = tl::split_first<std::string>(pairTimeVal.second, "=", 1);
	if(pairKeyVal.second == "")
	{
		pairKeyVal = tl::split_first<std::string>(pairTimeVal.second, "!", 1);
		if(pairKeyVal.second != "")
			tl::log_warn("Value \"", pairKeyVal.second, "\" for \"", pairKeyVal.first, "\" is marked as invalid.");
		else
			tl::log_err("Invalid net reply: \"", str, "\"");
	}


	const std::string& strKey = pairKeyVal.first;
	const std::string& strVal = pairKeyVal.second;


	const std::string& strTime = pairTimeVal.first;
	t_real dTimestamp = tl::str_to_var<t_real>(strTime);

	if(strVal.length() == 0)
		return;

	CacheVal cacheval;
	cacheval.strVal = strVal;
	cacheval.dTimestamp = dTimestamp;

	// mark special entries
	if(strKey == m_strTimer)
		cacheval.ty = CacheValType::TIMER;
	else if(strKey == m_strPreset)
		cacheval.ty = CacheValType::PRESET;
	else if(strKey == m_strCtr)
		cacheval.ty = CacheValType::COUNTER;

	m_mapCache[strKey] = cacheval;

	//std::cout << strKey << " = " << strVal << std::endl;
	emit updated_cache_value(strKey, cacheval);



	CrystalOptions crys;
	TriangleOptions triag;
	bool bUpdatedVals = 1;

	if(strKey == m_strSampleLattice)
	{
		std::vector<t_real> vecLattice =
			tl::get_py_array<std::string, std::vector<t_real>>(strVal);
		if(vecLattice.size() != 3)
			return;

		crys.bChangedLattice = 1;
		for(int i=0; i<3; ++i)
			crys.dLattice[i] = vecLattice[i];
	}
	else if(strKey == m_strSampleAngles)
	{
		std::vector<t_real> vecAngles =
			tl::get_py_array<std::string, std::vector<t_real>>(strVal);
		if(vecAngles.size() != 3)
			return;

		crys.bChangedLatticeAngles = 1;
		for(int i=0; i<3; ++i)
			crys.dLatticeAngles[i] = vecAngles[i];
	}
	else if(strKey == m_strSampleOrient1)
	{
		std::vector<t_real> vecOrient =
			tl::get_py_array<std::string, std::vector<t_real>>(strVal);
		if(vecOrient.size() != 3)
			return;

		crys.bChangedPlane1 = 1;
		for(int i=0; i<3; ++i)
			crys.dPlane1[i] = vecOrient[i];
	}
	else if(strKey == m_strSampleOrient2)
	{
		std::vector<t_real> vecOrient =
			tl::get_py_array<std::string, std::vector<t_real>>(strVal);
		if(vecOrient.size() != 3)
			return;

		crys.bChangedPlane2 = 1;

		for(int i=0; i<3; ++i)
			crys.dPlane2[i] = m_bFlipOrient2 ? -vecOrient[i] : vecOrient[i];
	}
	else if(strKey == m_strSampleSpacegroup)
	{
		crys.strSpacegroup = tl::get_py_string(strVal);
		crys.bChangedSpacegroup = 1;
	}
	else if(strKey == m_strMono2Theta)
	{
		triag.dMonoTwoTheta = tl::d2r(tl::str_to_var<t_real>(strVal));
		triag.bChangedMonoTwoTheta = 1;
	}
	else if(strKey == m_strAna2Theta)
	{
		triag.dAnaTwoTheta = tl::d2r(tl::str_to_var<t_real>(strVal));
		triag.bChangedAnaTwoTheta = 1;
	}
	else if(strKey == m_strSample2Theta)
	{
		triag.dTwoTheta = tl::d2r(tl::str_to_var<t_real>(strVal));
		triag.bChangedTwoTheta = 1;
	}
	else if(strKey == m_strMonoD)
	{
		triag.dMonoD = tl::str_to_var<t_real>(strVal);
		triag.bChangedMonoD = 1;
	}
	else if(strKey == m_strAnaD)
	{
		triag.dAnaD = tl::str_to_var<t_real>(strVal);
		triag.bChangedAnaD = 1;
	}
	else if(strKey == m_strSampleName)
	{
		crys.strSampleName = tl::get_py_string(strVal);
		crys.bChangedSampleName = 1;
	}
	else if(strKey == m_strSampleTheta || strKey == m_strSamplePsi0)
	{}
	else
	{
		bUpdatedVals = 0;
	}


	if(bUpdatedVals && m_mapCache.find(m_strSampleTheta) != m_mapCache.end()
		&& m_mapCache.find(m_strSamplePsi0) != m_mapCache.end())
	{
		// rotation of crystal -> rotation of plane (or triangle) -> sample theta

		t_real dSth = tl::d2r(tl::str_to_var<t_real>(m_mapCache[m_strSampleTheta].strVal));
		t_real dPsi = tl::d2r(tl::str_to_var<t_real>(m_mapCache[m_strSamplePsi0].strVal));

		// sth and psi0 are arbitrary, but together they form the
		// angle from ki to the bragg peak at orient1
		triag.dAngleKiVec0 = -dSth-dPsi;

		if(m_bSthCorr && m_mapCache.find(m_strSample2Theta) != m_mapCache.end())
		{
			t_real dStt = tl::d2r(tl::str_to_var<t_real>(m_mapCache[m_strSample2Theta].strVal));
			triag.dAngleKiVec0 -= dStt;
		}

		triag.bChangedAngleKiVec0 = 1;
		//std::cout << "rotation: " << triag.dAngleKiVec0 << std::endl;
	}

	if(bUpdatedVals)
		emit vars_changed(crys, triag);
}

#include "net.moc"
#include "nicos.moc"
