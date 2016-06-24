/*
 * TAS tool (server stuff)
 * @author tweber
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

	m_pSrvDlg->show();
	m_pSrvDlg->activateWindow();
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

	m_pNetCacheDlg->ClearAll();
	QObject::connect(m_pNetCache, SIGNAL(updated_cache_value(const std::string&, const CacheVal&)),
		m_pNetCacheDlg, SLOT(UpdateValue(const std::string&, const CacheVal&)));

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

	m_pNetCacheDlg->show();
	m_pNetCacheDlg->activateWindow();
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

void TazDlg::VarsChanged(const CrystalOptions& crys, const TriangleOptions& triag)
{
	if(crys.strSampleName != "")
		editDescr->setText(tl::trimmed(crys.strSampleName).c_str());

	if(crys.bChangedLattice)
	{
		QString qstr0 = QString::number(crys.dLattice[0]);
		QString qstr1 = QString::number(crys.dLattice[1]);
		QString qstr2 = QString::number(crys.dLattice[2]);

		if(qstr0!=editA->text() || qstr1!=editB->text() || qstr2!=editC->text())
		{
			editA->setText(qstr0);
			editB->setText(qstr1);
			editC->setText(qstr2);

			CalcPeaks();
		}
	}

	if(crys.bChangedLatticeAngles)
	{
		QString qstr0 = QString::number(crys.dLatticeAngles[0]);
		QString qstr1 = QString::number(crys.dLatticeAngles[1]);
		QString qstr2 = QString::number(crys.dLatticeAngles[2]);

		if(qstr0!=editAlpha->text() || qstr1!=editBeta->text() || qstr2!=editGamma->text())
		{
			editAlpha->setText(qstr0);
			editBeta->setText(qstr1);
			editGamma->setText(qstr2);

			CalcPeaks();
		}
	}

	if(crys.bChangedSpacegroup)
	{
		editSpaceGroupsFilter->clear();
		int iSGIdx = comboSpaceGroups->findText(crys.strSpacegroup.c_str());
		if(iSGIdx >= 0)
			comboSpaceGroups->setCurrentIndex(iSGIdx);
		else
			comboSpaceGroups->setCurrentIndex(0);

		CalcPeaks();
	}

	if(crys.bChangedPlane1)
	{
		QString qstr0 = QString::number(crys.dPlane1[0]);
		QString qstr1 = QString::number(crys.dPlane1[1]);
		QString qstr2 = QString::number(crys.dPlane1[2]);

		if(qstr0!=editScatX0->text() || qstr1!=editScatX1->text() || qstr2!=editScatX2->text())
		{
			editScatX0->setText(qstr0);
			editScatX1->setText(qstr1);
			editScatX2->setText(qstr2);

			CalcPeaks();
		}
	}
	if(crys.bChangedPlane2)
	{
		QString qstr0 = QString::number(crys.dPlane2[0]);
		QString qstr1 = QString::number(crys.dPlane2[1]);
		QString qstr2 = QString::number(crys.dPlane2[2]);

		if(qstr0!=editScatY0->text() || qstr1!=editScatY1->text() || qstr2!=editScatY2->text())
		{
			editScatY0->setText(qstr0);
			editScatY1->setText(qstr1);
			editScatY2->setText(qstr2);

			CalcPeaks();
		}
	}

	if(triag.bChangedMonoD)
	{
		QString qstr = QString::number(triag.dMonoD);
		if(qstr != editMonoD->text())
		{
			editMonoD->setText(qstr);
			UpdateDs();
		}
	}
	if(triag.bChangedAnaD)
	{
		QString qstr = QString::number(triag.dAnaD);
		if(qstr != editAnaD->text())
		{
			editAnaD->setText(qstr);
			UpdateDs();
		}
	}


	// hack!
	if(triag.bChangedTwoTheta && !checkSenseS->isChecked())
		const_cast<TriangleOptions&>(triag).dTwoTheta = -triag.dTwoTheta;

	//if(triag.bChangedTwoTheta)
	//	tl::log_info("2theta: ", triag.dTwoTheta/M_PI*180.);

	m_sceneReal.triangleChanged(triag);
	m_sceneReal.emitUpdate(triag);
	//m_sceneReal.emitAllParams();

	UpdateMonoSense();
	UpdateAnaSense();
	UpdateSampleSense();

	if(triag.bChangedAngleKiVec0)
	{
		m_sceneRecip.tasChanged(triag);
		m_sceneRecip.emitUpdate();
		//m_sceneRecip.emitAllParams();
	}
}
