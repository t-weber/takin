/**
 * TAS tool (server stuff)
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date feb-2014
 * @license GPLv2
 */

#include "taz.h"

#include <QStatusBar>
#include <QMessageBox>

#define DEFAULT_MSG_TIMEOUT 4000

void TazDlg::ShowConnectDlg()
{
	if(!m_pSrvDlg)
	{
		m_pSrvDlg = new SrvDlg(this, &m_settings);
		QObject::connect(m_pSrvDlg, SIGNAL(ConnectTo(int, const QString&, const QString&, const QString&, const QString&)),
			this, SLOT(ConnectTo(int, const QString&, const QString&, const QString&, const QString&)));
	}

	focus_dlg(m_pSrvDlg);
}

void TazDlg::ConnectTo(int iSys, const QString& _strHost, const QString& _strPort,
	const QString& _strUser, const QString& _strPass)
{
	Disconnect();

	std::string strHost = _strHost.toStdString();
	std::string strPort = _strPort.toStdString();
	std::string strUser = _strUser.toStdString();
	std::string strPass = _strPass.toStdString();

	if(iSys == 0)
		m_pNetCache = new NicosCache(&m_settings);
	else if(iSys == 1)
		m_pNetCache = new SicsCache(&m_settings);
	else
	{
		tl::log_err("Unknown instrument control system selected.");
		return;
	}


	QObject::connect(m_pNetCache, SIGNAL(vars_changed(const CrystalOptions&, const TriangleOptions&)),
		this, SLOT(VarsChanged(const CrystalOptions&, const TriangleOptions&)));
	QObject::connect(m_pNetCache, SIGNAL(connected(const QString&, const QString&)),
		this, SLOT(Connected(const QString&, const QString&)));
	QObject::connect(m_pNetCache, SIGNAL(disconnected()),
		this, SLOT(Disconnected()));

	if(!m_pNetCacheDlg)
		m_pNetCacheDlg = new NetCacheDlg(this, &m_settings);
	if(!m_pScanMonDlg)
		m_pScanMonDlg = new ScanMonDlg(this, &m_settings);

	m_pNetCacheDlg->ClearAll();
	m_pScanMonDlg->ClearPlot();

	QObject::connect(m_pNetCache, SIGNAL(updated_cache_value(const std::string&, const CacheVal&)),
		m_pNetCacheDlg, SLOT(UpdateValue(const std::string&, const CacheVal&)));
	QObject::connect(m_pNetCache, SIGNAL(updated_cache_value(const std::string&, const CacheVal&)),
		m_pScanMonDlg, SLOT(UpdateValue(const std::string&, const CacheVal&)));


	// no manual node movement
	if(m_sceneReal.GetTasLayout()) m_sceneReal.GetTasLayout()->AllowMouseMove(0);
	if(m_sceneTof.GetTofLayout()) m_sceneTof.GetTofLayout()->AllowMouseMove(0);
	if(m_sceneRecip.GetTriangle()) m_sceneRecip.GetTriangle()->AllowMouseMove(0);

	m_pNetCache->connect(strHost, strPort, strUser, strPass);
}

void TazDlg::Disconnect()
{
	if(m_pNetCache)
	{
		m_pNetCache->disconnect();

		QObject::disconnect(m_pNetCache, SIGNAL(vars_changed(const CrystalOptions&, const TriangleOptions&)),
			this, SLOT(VarsChanged(const CrystalOptions&, const TriangleOptions&)));
		QObject::disconnect(m_pNetCache, SIGNAL(connected(const QString&, const QString&)),
			this, SLOT(Connected(const QString&, const QString&)));
		QObject::disconnect(m_pNetCache, SIGNAL(disconnected()),
			this, SLOT(Disconnected()));

		QObject::disconnect(m_pNetCache, SIGNAL(updated_cache_value(const std::string&, const CacheVal&)),
			m_pNetCacheDlg, SLOT(UpdateValue(const std::string&, const CacheVal&)));

		delete m_pNetCache;
		m_pNetCache = nullptr;
	}

	// re-enable manual node movement
	if(m_sceneReal.GetTasLayout()) m_sceneReal.GetTasLayout()->AllowMouseMove(1);
	if(m_sceneTof.GetTofLayout()) m_sceneTof.GetTofLayout()->AllowMouseMove(1);
	if(m_sceneRecip.GetTriangle()) m_sceneRecip.GetTriangle()->AllowMouseMove(1);

	statusBar()->showMessage("Disconnected.", DEFAULT_MSG_TIMEOUT);
}

void TazDlg::ShowNetCache()
{
	if(!m_pNetCacheDlg)
		m_pNetCacheDlg = new NetCacheDlg(this, &m_settings);

	focus_dlg(m_pNetCacheDlg);
}

void TazDlg::ShowNetScanMonitor()
{
	if(!m_pScanMonDlg)
		m_pScanMonDlg = new ScanMonDlg(this, &m_settings);

	focus_dlg(m_pScanMonDlg);
}

void TazDlg::NetRefresh()
{
	if(m_pNetCache)
		m_pNetCache->refresh();
	else
		QMessageBox::warning(this, "Warning", "Not connected to an instrument server.");
}

void TazDlg::Connected(const QString& strHost, const QString& strSrv)
{
	m_strCurFile = "";
	m_vecAtoms.clear();

	setWindowTitle((s_strTitle + " - ").c_str() + strHost + ":" + strSrv);
	statusBar()->showMessage("Connected to " + strHost + " on port " + strSrv + ".", DEFAULT_MSG_TIMEOUT);
}

void TazDlg::Disconnected()
{
	setWindowTitle((s_strTitle).c_str());
}
