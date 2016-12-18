/*
 * TAS tool
 * @author tweber
 * @date feb-2015
 * @license GPLv2
 */

#include "taz.h"
#include "tlibs/string/string.h"
#include "tlibs/file/recent.h"
#include "tlibs/time/chrono.h"
#include "libs/version.h"
#include "dialogs/FilePreviewDlg.h"

#include <QMessageBox>
#include <QFileDialog>
#include <boost/scope_exit.hpp>


//--------------------------------------------------------------------------------
// loading/saving

void TazDlg::New()
{
	CrystalOptions crys;
	TriangleOptions triag;

	crys.dLattice[0] = crys.dLattice[1] = crys.dLattice[2] = 5.;
	crys.dLatticeAngles[0] = crys.dLatticeAngles[1] = crys.dLatticeAngles[2] = 90.;
	crys.bChangedLattice = crys.bChangedLatticeAngles = 1;

	crys.dPlane1[0] = 1.; crys.dPlane1[1] = 0.; crys.dPlane1[2] = 0.;
	crys.dPlane2[0] = 0.; crys.dPlane2[1] = 1.; crys.dPlane2[2] = 0.;
	crys.bChangedPlane1 = crys.bChangedPlane2 = 1;

	crys.strSampleName = " ";
	crys.strSpacegroup = "";
	crys.bChangedSpacegroup = 1;

	triag.dAnaD = triag.dMonoD = 3.355;
	triag.bChangedAnaD = triag.bChangedMonoD = 1;
	triag.dAnaTwoTheta = triag.dMonoTwoTheta = tl::get_pi<t_real_glob>()/2.;
	triag.bChangedAnaTwoTheta = triag.bChangedMonoTwoTheta = 1;

	triag.dTwoTheta = tl::get_pi<t_real_glob>()/2.;
	triag.dAngleKiVec0 = tl::get_pi<t_real_glob>()/4.;
	triag.bChangedTwoTheta = triag.bChangedAngleKiVec0 = 1;

	m_vecAtoms.clear();
	m_strCurFile = "";
	setWindowTitle(s_strTitle.c_str());

	DeleteDialogs();
	Disconnect();
	VarsChanged(crys, triag);
}

bool TazDlg::Load()
{
	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(!m_settings.value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = m_settings.value("main/last_dir", ".").toString();
	QString strFile = QFileDialog::getOpenFileName(this,
		"Open TAS Configuration...", strDirLast, "Takin files (*.taz *.TAZ)",
		nullptr, fileopt);
	if(strFile == "")
		return false;

	return Load(strFile.toStdString().c_str());
}

bool TazDlg::LoadFile(const QString& strFile)
{
	return Load(strFile.toStdString().c_str());
}

bool TazDlg::Load(const char* pcFile)
{
	m_bReady = 0;
	BOOST_SCOPE_EXIT(&m_bReady, &m_sceneReal, &m_sceneRecip)
	{
		m_bReady = 1;
		m_sceneReal.GetTasLayout()->SetReady(true);
		m_sceneReal.SetEmitChanges(true);

		m_sceneRecip.GetTriangle()->SetReady(true);
		m_sceneRecip.SetEmitChanges(true);
	} BOOST_SCOPE_EXIT_END

	Disconnect();

	const std::string strXmlRoot("taz/");

	std::string strFile1 = pcFile;
	std::string strDir = tl::get_dir(strFile1);


	tl::Prop<std::string> xml;
	if(!xml.Load(strFile1.c_str(), tl::PropType::XML))
	{
		std::string strErr = "Could not load file \""
			+ std::string(pcFile) + "\".";
		QMessageBox::critical(this, "Error", strErr.c_str());
		return false;
	}

	m_settings.setValue("main/last_dir", QString(strDir.c_str()));


	bool bOk = 0;
	m_sceneReal.SetEmitChanges(false);
	m_sceneReal.GetTasLayout()->SetReady(false);
	m_sceneRecip.SetEmitChanges(false);
	m_sceneRecip.GetTriangle()->SetReady(false);


	// edit boxes
	std::vector<std::vector<QLineEdit*>*> vecEdits
		= {&m_vecEdits_real, &m_vecEdits_recip,
			&m_vecEdits_plane, &m_vecEdits_monoana};
	std::vector<std::vector<std::string>*> vecEditNames
		= {&m_vecEditNames_real, &m_vecEditNames_recip,
			&m_vecEditNames_plane, &m_vecEditNames_monoana};
	unsigned int iIdxEdit = 0;
	for(const std::vector<QLineEdit*>* pVec : vecEdits)
	{
		const std::vector<std::string>* pvecName = vecEditNames[iIdxEdit];

		for(unsigned int iEditBox=0; iEditBox<pVec->size(); ++iEditBox)
		{
			std::string str = xml.Query<std::string>((strXmlRoot+(*pvecName)[iEditBox]).c_str(), "0", &bOk);
			tl::trim(str);
			if(bOk)
				(*pVec)[iEditBox]->setText(str.c_str());
		}

		++iIdxEdit;
	}

	std::string strDescr = xml.Query<std::string>((strXmlRoot+"sample/descr").c_str(), "", &bOk);
	if(bOk)
		editDescr->setText(strDescr.c_str());


	/*// spin boxes
	for(unsigned int iSpinBox=0; iSpinBox<m_vecSpinBoxesSample.size(); ++iSpinBox)
	{
		double dVal = xml.Query<double>((strXmlRoot+m_vecSpinBoxNamesSample[iSpinBox]).c_str(), 0., &bOk);
		if(bOk)
			m_vecSpinBoxesSample[iSpinBox]->setValue(dVal);
	}*/


	// check boxes
	for(unsigned int iCheckBox=0; iCheckBox<m_vecCheckBoxesSenses.size(); ++iCheckBox)
	{
		int iVal = xml.Query<int>((strXmlRoot+m_vecCheckBoxNamesSenses[iCheckBox]).c_str(), 0, &bOk);
		if(bOk)
			m_vecCheckBoxesSenses[iCheckBox]->setChecked(iVal != 0);
	}


	// TAS Layout
	double dRealScale = xml.Query<double>((strXmlRoot + "real/pixels_per_cm").c_str(), 0., &bOk);
	if(bOk)
		m_sceneReal.GetTasLayout()->SetScaleFactor(dRealScale);

	unsigned int iNodeReal = 0;
	for(TasLayoutNode *pNode : m_sceneReal.GetTasLayout()->GetNodes())
	{
		std::string strNode = m_sceneReal.GetTasLayout()->GetNodeNames()[iNodeReal];

		bool bOkX=0, bOkY=0;
		double dValX = xml.Query<double>((strXmlRoot + "real/" + strNode + "_x").c_str(), 0., &bOkX);
		double dValY = xml.Query<double>((strXmlRoot + "real/" + strNode + "_y").c_str(), 0., &bOkY);

		pNode->setPos(dValX, dValY);
		++iNodeReal;
	}



	// scattering triangle
	double dRecipScale = xml.Query<double>((strXmlRoot + "recip/pixels_per_A-1").c_str(), 0., &bOk);
	if(bOk)
		m_sceneRecip.GetTriangle()->SetScaleFactor(dRecipScale);

	unsigned int iNodeRecip = 0;
	for(ScatteringTriangleNode *pNode : m_sceneRecip.GetTriangle()->GetNodes())
	{
		std::string strNode = m_sceneRecip.GetTriangle()->GetNodeNames()[iNodeRecip];

		bool bOkX=0, bOkY=0;
		double dValX = xml.Query<double>((strXmlRoot + "recip/" + strNode + "_x").c_str(), 0., &bOkX);
		double dValY = xml.Query<double>((strXmlRoot + "recip/" + strNode + "_y").c_str(), 0., &bOkY);

		pNode->setPos(dValX, dValY);
		++iNodeRecip;
	}



	int bSmallqEnabled = xml.Query<int>((strXmlRoot + "recip/enable_q").c_str(), 0, &bOk);
	if(bOk)
		m_pSmallq->setChecked(bSmallqEnabled!=0);

	int bSmallqSnapped = xml.Query<int>((strXmlRoot + "recip/snap_q").c_str(), 1, &bOk);
	if(bOk)
		m_pSnapSmallq->setChecked(bSmallqSnapped!=0);

	int bBZEnabled = xml.Query<int>((strXmlRoot + "recip/enable_bz").c_str(), 0, &bOk);
	if(bOk)
		m_pBZ->setChecked(bBZEnabled!=0);

	int iEwald = xml.Query<int>((strXmlRoot + "recip/ewald_sphere").c_str(), 0, &bOk);
	if(bOk)
	{
		if(iEwald == EWALD_NONE) m_pEwaldSphereNone->setChecked(1);
		else if(iEwald == EWALD_KI) m_pEwaldSphereKi->setChecked(1);
		else if(iEwald == EWALD_KF) m_pEwaldSphereKf->setChecked(1);
	}

	int bWSEnabled = xml.Query<int>((strXmlRoot + "real/enable_ws").c_str(), 0, &bOk);
	if(bOk)
		m_pWS->setChecked(bWSEnabled!=0);

	int bRealQEnabled = xml.Query<int>((strXmlRoot + "real/enable_realQDir").c_str(), 0, &bOk);
	if(bOk)
		m_pShowRealQDir->setChecked(bRealQEnabled!=0);

	std::string strSpaceGroup = xml.Query<std::string>((strXmlRoot + "sample/spacegroup").c_str(), "", &bOk);
	tl::trim(strSpaceGroup);
	if(bOk)
	{
		editSpaceGroupsFilter->clear();
		int iSGIdx = comboSpaceGroups->findText(strSpaceGroup.c_str(), Qt::MatchFixedString);
		if(iSGIdx >= 0)
			comboSpaceGroups->setCurrentIndex(iSGIdx);
		else
			comboSpaceGroups->setCurrentIndex(0);
	}


	m_vecAtoms.clear();
	unsigned int iNumAtoms = xml.Query<unsigned int>((strXmlRoot + "sample/atoms/num").c_str(), 0, &bOk);
	if(bOk)
	{
		m_vecAtoms.reserve(iNumAtoms);

		for(unsigned int iAtom=0; iAtom<iNumAtoms; ++iAtom)
		{
			AtomPos<t_real_glob> atom;
			atom.vecPos.resize(3,0);

			std::string strNr = tl::var_to_str(iAtom);
			atom.strAtomName = xml.Query<std::string>((strXmlRoot + "sample/atoms/" + strNr + "/name").c_str(), "");
			atom.vecPos[0] = xml.Query<double>((strXmlRoot + "sample/atoms/" + strNr + "/x").c_str(), 0.);
			atom.vecPos[1] = xml.Query<double>((strXmlRoot + "sample/atoms/" + strNr + "/y").c_str(), 0.);
			atom.vecPos[2] = xml.Query<double>((strXmlRoot + "sample/atoms/" + strNr + "/z").c_str(), 0.);

			m_vecAtoms.push_back(atom);
		}
	}


	if(m_pGotoDlg)
		m_pGotoDlg->ClearList();

	if(xml.Exists((strXmlRoot + "goto_favlist").c_str()) ||
		xml.Exists((strXmlRoot + "goto_pos").c_str()))
	{
		InitGoto();
		m_pGotoDlg->Load(xml, strXmlRoot);
	}


	if(xml.Exists((strXmlRoot + "reso").c_str()))
	{
		InitReso();
		m_pReso->SetUpdateOn(0,0);
		m_pReso->Load(xml, strXmlRoot);
	}


	m_strCurFile = strFile1;
	setWindowTitle((s_strTitle + " - " + m_strCurFile).c_str());

	tl::RecentFiles recent(&m_settings, "main/recent");
	recent.AddFile(strFile1.c_str());
	recent.SaveList();
	recent.FillMenu(m_pMenuRecent, m_pMapperRecent);



	m_bReady = 1;

	m_sceneReal.GetTasLayout()->SetReady(true);
	m_sceneReal.SetEmitChanges(true);
	//m_sceneReal.emitUpdate();

	m_sceneRecip.GetTriangle()->SetReady(true);
	m_sceneRecip.SetEmitChanges(true);

	CalcPeaks();
	m_sceneRecip.emitUpdate();

	if(m_pReso)
	{
		m_pReso->SetUpdateOn(1,1);
		//emitSampleParams();
		m_sceneRecip.emitAllParams();
		//m_sceneReal.emitAllParams();
	}

	return true;
}

bool TazDlg::Save()
{
	if(m_strCurFile == "")
		return SaveAs();

	const std::string strXmlRoot("taz/");
	typedef std::map<std::string, std::string> tmap;
	tmap mapConf;


	// edit boxes
	std::vector<const std::vector<QLineEdit*>*> vecEdits
			= {&m_vecEdits_real, &m_vecEdits_recip,
				&m_vecEdits_plane, &m_vecEdits_monoana};
	std::vector<const std::vector<std::string>*> vecEditNames
			= {&m_vecEditNames_real, &m_vecEditNames_recip,
				&m_vecEditNames_plane, &m_vecEditNames_monoana};
	unsigned int iIdxEdit = 0;
	for(const std::vector<QLineEdit*>* pVec : vecEdits)
	{
		const std::vector<std::string>* pvecName = vecEditNames[iIdxEdit];

		for(unsigned int iEditBox=0; iEditBox<pVec->size(); ++iEditBox)
			mapConf[strXmlRoot+(*pvecName)[iEditBox]]
			        = (*pVec)[iEditBox]->text().toStdString();

		++iIdxEdit;
	}

	mapConf[strXmlRoot + "sample/descr"] = editDescr->text().toStdString();

	/*// spin boxes
	for(unsigned int iSpinBox=0; iSpinBox<m_vecSpinBoxesSample.size(); ++iSpinBox)
	{
		std::ostringstream ostrVal;
		ostrVal << std::scientific;
		ostrVal << m_vecSpinBoxesSample[iSpinBox]->value();

		mapConf[strXmlRoot + m_vecSpinBoxNamesSample[iSpinBox]] = ostrVal.str();
	}*/


	// check boxes
	for(unsigned int iCheckBox=0; iCheckBox<m_vecCheckBoxesSenses.size(); ++iCheckBox)
		mapConf[strXmlRoot+m_vecCheckBoxNamesSenses[iCheckBox]]
		        		= (m_vecCheckBoxesSenses[iCheckBox]->isChecked() ? "1" : "0");


	// TAS layout
	unsigned int iNodeReal = 0;
	for(const TasLayoutNode *pNode : m_sceneReal.GetTasLayout()->GetNodes())
	{
		std::string strNode = m_sceneReal.GetTasLayout()->GetNodeNames()[iNodeReal];
		std::string strValX = tl::var_to_str(pNode->pos().x());
		std::string strValY = tl::var_to_str(pNode->pos().y());

		mapConf[strXmlRoot + "real/" + strNode + "_x"] = strValX;
		mapConf[strXmlRoot + "real/" + strNode + "_y"] = strValY;

		++iNodeReal;
	}
	double dRealScale = m_sceneReal.GetTasLayout()->GetScaleFactor();
	mapConf[strXmlRoot + "real/pixels_per_cm"] = tl::var_to_str(dRealScale);


	// scattering triangle
	unsigned int iNodeRecip = 0;
	for(const ScatteringTriangleNode *pNode : m_sceneRecip.GetTriangle()->GetNodes())
	{
		std::string strNode = m_sceneRecip.GetTriangle()->GetNodeNames()[iNodeRecip];
		std::string strValX = tl::var_to_str(pNode->pos().x());
		std::string strValY = tl::var_to_str(pNode->pos().y());

		mapConf[strXmlRoot + "recip/" + strNode + "_x"] = strValX;
		mapConf[strXmlRoot + "recip/" + strNode + "_y"] = strValY;

		++iNodeRecip;
	}
	double dRecipScale = m_sceneRecip.GetTriangle()->GetScaleFactor();
	mapConf[strXmlRoot + "recip/pixels_per_A-1"] = tl::var_to_str(dRecipScale);


	bool bSmallqEnabled = m_pSmallq->isChecked();
	mapConf[strXmlRoot + "recip/enable_q"] = (bSmallqEnabled ? "1" : "0");

	bool bSmallqSnapped = m_sceneRecip.getSnapq();
	mapConf[strXmlRoot + "recip/snap_q"] = (bSmallqSnapped ? "1" : "0");

	bool bBZEnabled = m_pBZ->isChecked();
	mapConf[strXmlRoot + "recip/enable_bz"] = (bBZEnabled ? "1" : "0");

	int iEw = EWALD_NONE;
	if(m_pEwaldSphereKi->isChecked()) iEw = EWALD_KI;
	else if(m_pEwaldSphereKf->isChecked()) iEw = EWALD_KF;
	mapConf[strXmlRoot + "recip/ewald_sphere"] = tl::var_to_str(iEw);

	bool bWSEnabled = m_pWS->isChecked();
	mapConf[strXmlRoot + "real/enable_ws"] = (bWSEnabled ? "1" : "0");

	bool bRealQDir = m_pShowRealQDir->isChecked();
	mapConf[strXmlRoot + "real/enable_realQDir"] = (bRealQDir ? "1" : "0");


	std::string strSG = comboSpaceGroups->currentText().toStdString();
	if(strSG == "<not set>")
		strSG = "-1";
	mapConf[strXmlRoot + "sample/spacegroup"] = strSG;


	mapConf[strXmlRoot + "sample/atoms/num"] = tl::var_to_str(m_vecAtoms.size());
	for(unsigned int iAtom=0; iAtom<m_vecAtoms.size(); ++iAtom)
	{
		const AtomPos<t_real_glob>& atom = m_vecAtoms[iAtom];

		std::string strAtomNr = tl::var_to_str(iAtom);
		mapConf[strXmlRoot + "sample/atoms/" + strAtomNr + "/name"] =
			atom.strAtomName;
		mapConf[strXmlRoot + "sample/atoms/" + strAtomNr + "/x"] =
			tl::var_to_str(atom.vecPos[0]);
		mapConf[strXmlRoot + "sample/atoms/" + strAtomNr + "/y"] =
			tl::var_to_str(atom.vecPos[1]);
		mapConf[strXmlRoot + "sample/atoms/" + strAtomNr + "/z"] =
			tl::var_to_str(atom.vecPos[2]);
	}

	mapConf[strXmlRoot + "meta/timestamp"] = tl::var_to_str<t_real_glob>(tl::epoch<t_real_glob>());
	mapConf[strXmlRoot + "meta/version"] = TAKIN_VER;
	mapConf[strXmlRoot + "meta/info"] = "Created with Takin.";


	if(m_pReso) m_pReso->Save(mapConf, strXmlRoot);
	if(m_pGotoDlg) m_pGotoDlg->Save(mapConf, strXmlRoot);


	tl::Prop<std::string> xml;
	xml.Add(mapConf);
	if(!xml.Save(m_strCurFile.c_str(), tl::PropType::XML))
	{
		QMessageBox::critical(this, "Error", "Could not save configuration file.");
		return false;
	}

	tl::RecentFiles recent(&m_settings, "main/recent");
	recent.AddFile(m_strCurFile.c_str());
	recent.SaveList();
	recent.FillMenu(m_pMenuRecent, m_pMapperRecent);

	return true;
}

bool TazDlg::SaveAs()
{
	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(!m_settings.value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = m_settings.value("main/last_dir", ".").toString();
	QString strFile = QFileDialog::getSaveFileName(this,
		"Save TAS Configuration", strDirLast, "Takin files (*.taz *.TAZ)",
		nullptr, fileopt);

	if(strFile != "")
	{
		std::string strFile1 = strFile.toStdString();
		std::string strDir = tl::get_dir(strFile1);

		m_strCurFile = strFile1;
		setWindowTitle((s_strTitle + " - " + m_strCurFile).c_str());
		bool bOk = Save();

		if(bOk)
			m_settings.setValue("main/last_dir", QString(strDir.c_str()));

		return bOk;
	}

	return false;
}




//--------------------------------------------------------------------------------
// importing

#include "tlibs/file/loadinstr.h"

bool TazDlg::Import()
{
	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(!m_settings.value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	const bool bShowPreview = m_settings.value("main/dlg_previews", true).toBool();
	QString strDirLast = m_settings.value("main/last_import_dir", ".").toString();

	QFileDialog *pdlg = nullptr;
	if(bShowPreview)
	{
		pdlg = new FilePreviewDlg(this, "Import Data File...", &m_settings);
		pdlg->setOptions(QFileDialog::DontUseNativeDialog);
	}
	else
	{
		pdlg = new QFileDialog(this, "Import Data File...");
		pdlg->setOptions(fileopt);
	}
	std::unique_ptr<QFileDialog> ptrdlg(pdlg);

	pdlg->setDirectory(strDirLast);
	pdlg->setFileMode(QFileDialog::ExistingFile);
	pdlg->setViewMode(QFileDialog::Detail);
#if !defined NO_IOSTR
	QString strFilter = "Data files (*.dat *.scn *.DAT *.SCN *.ng0 *.NG0 *.log *.LOG *.scn.gz *.SCN.GZ *.dat.gz *.DAT.GZ *.ng0.gz *.NG0.GZ *.log.gz *.LOG.GZ *.scn.bz2 *.SCN.BZ2 *.dat.bz2 *.DAT.BZ2 *.ng0.bz2 *.NG0.BZ2 *.log.bz2 *.LOG.BZ2);;All files (*.*)";
#else
	QString strFilter = "Data files (*.dat *.scn *.DAT *.SCN *.NG0 *.ng0 *.log *.LOG);;All files (*.*)";
#endif
	pdlg->setNameFilter(strFilter);
	if(!pdlg->exec())
		return false;
	if(!pdlg->selectedFiles().size())
		return false;

	QString strFile = pdlg->selectedFiles()[0];
	if(strFile == "")
		return false;

	return Import(strFile.toStdString().c_str());
}

bool TazDlg::ImportFile(const QString& strFile)
{
	return Import(strFile.toStdString().c_str());
}

bool TazDlg::Import(const char* pcFile)
{
	Disconnect();

	std::string strFile1 = pcFile;
	std::string strDir = tl::get_dir(strFile1);

	std::size_t iScanNum = 0;
	try
	{
		std::unique_ptr<tl::FileInstrBase<t_real_glob>> ptrDat(
			tl::FileInstrBase<t_real_glob>::LoadInstr(pcFile));
		tl::FileInstrBase<t_real_glob>* pdat = ptrDat.get();
		if(!pdat)
			return false;

		std::array<t_real_glob,3> arrLatt = pdat->GetSampleLattice();
		std::array<t_real_glob,3> arrAng = pdat->GetSampleAngles();
		std::array<bool, 3> arrSenses = pdat->GetScatterSenses();
		std::array<t_real_glob, 2> arrD = pdat->GetMonoAnaD();
		std::array<t_real_glob, 3> arrPeak0 = pdat->GetScatterPlane0();
		std::array<t_real_glob, 3> arrPeak1 = pdat->GetScatterPlane1();

		editA->setText(tl::var_to_str(arrLatt[0]).c_str());
		editB->setText(tl::var_to_str(arrLatt[1]).c_str());
		editC->setText(tl::var_to_str(arrLatt[2]).c_str());

		editAlpha ->setText(tl::var_to_str(tl::r2d(arrAng[0])).c_str());
		editBeta->setText(tl::var_to_str(tl::r2d(arrAng[1])).c_str());
		editGamma->setText(tl::var_to_str(tl::r2d(arrAng[2])).c_str());

		editMonoD->setText(tl::var_to_str(arrD[0]).c_str());
		editAnaD->setText(tl::var_to_str(arrD[1]).c_str());

		checkSenseM->setChecked(arrSenses[0]);
		checkSenseS->setChecked(arrSenses[1]);
		checkSenseA->setChecked(arrSenses[2]);

		editScatX0->setText(tl::var_to_str(arrPeak0[0]).c_str());
		editScatX1->setText(tl::var_to_str(arrPeak0[1]).c_str());
		editScatX2->setText(tl::var_to_str(arrPeak0[2]).c_str());

		editScatY0->setText(tl::var_to_str(arrPeak1[0]).c_str());
		editScatY1->setText(tl::var_to_str(arrPeak1[1]).c_str());
		editScatY2->setText(tl::var_to_str(arrPeak1[2]).c_str());

		// spacegroup
		std::string strSpaceGroup = pdat->GetSpacegroup();
		tl::trim(strSpaceGroup);
		editSpaceGroupsFilter->clear();
		int iSGIdx = comboSpaceGroups->findText(strSpaceGroup.c_str(), Qt::MatchFixedString);
		if(iSGIdx >= 0)
			comboSpaceGroups->setCurrentIndex(iSGIdx);
		else
			comboSpaceGroups->setCurrentIndex(0);

		// descr
		std::string strExp = pdat->GetTitle();
		std::string strSample = pdat->GetSampleName();
		if(strSample != "")
			strExp += std::string(" - ") + strSample;
		editDescr->setText(strExp.c_str());

		iScanNum = pdat->GetScanCount();
		if(iScanNum)
		{
			InitGoto();
			m_pGotoDlg->ClearList();

			for(std::size_t iScan=0; iScan<iScanNum; ++iScan)
			{
				std::array<t_real_glob,5> arrScan = pdat->GetScanHKLKiKf(iScan);
				m_pGotoDlg->AddPosToList(arrScan[0],arrScan[1],arrScan[2],arrScan[3],arrScan[4]);
			}
		}
	}
	catch(const std::exception& ex)
	{
		tl::log_err(ex.what());
		return false;
	}


	m_settings.setValue("main/last_import_dir", QString(strDir.c_str()));
	m_strCurFile = /*strFile1*/ "";		// prevents overwriting imported file on saving
	setWindowTitle((s_strTitle + " - " + strFile1).c_str());

	tl::RecentFiles recent(&m_settings, "main/recent_import");
	recent.AddFile(strFile1.c_str());
	recent.SaveList();
	recent.FillMenu(m_pMenuRecentImport, m_pMapperRecentImport);

	CalcPeaks();

	if(iScanNum && m_pGotoDlg)
	{
		if(m_pGotoDlg->GotoPos(0))
		{
			m_pGotoDlg->show();
			m_pGotoDlg->activateWindow();
		}
	}

	return true;
}

void TazDlg::ShowScanViewer()
{
	if(!m_pScanViewer)
		m_pScanViewer = new ScanViewerDlg(this);

	m_pScanViewer->show();
	m_pScanViewer->activateWindow();
}
