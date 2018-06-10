/**
 * TAS tool
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date feb-2015
 * @license GPLv2
 */

#include "taz.h"
#include "tlibs/string/string.h"
#include "tlibs/time/chrono.h"
#include "libs/version.h"
#include "libs/qt/recent.h"
#include "dialogs/FilePreviewDlg.h"

#include <QMessageBox>
#include <QFileDialog>
#include <boost/scope_exit.hpp>

using t_real = t_real_glob;


/**
 * gets space group index from combo box
 */
static int find_sg_from_combo(QComboBox* pCombo, const std::string& str)
{
	//return pCombo->findText(str.c_str(), Qt::MatchContains /*Qt::MatchFixedString*/);

	for(int iIdx=0; iIdx<pCombo->count(); ++iIdx)
	{
		xtl::SpaceGroup<t_real> *pSG = reinterpret_cast<xtl::SpaceGroup<t_real>*>
			(pCombo->itemData(iIdx).value<void*>());
		if(pSG && pSG->GetName() == str)
			return iIdx;
	}

	return -1;
}


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
	triag.dAnaTwoTheta = triag.dMonoTwoTheta = tl::get_pi<t_real>()/2.;
	triag.bChangedAnaTwoTheta = triag.bChangedMonoTwoTheta = 1;

	triag.dTwoTheta = tl::get_pi<t_real>()/2.;
	triag.dMonoTwoTheta = triag.dAnaTwoTheta = tl::get_pi<t_real>()/2.;
	triag.dAngleKiVec0 = tl::get_pi<t_real>()/4.;
	triag.bChangedTwoTheta = triag.bChangedAngleKiVec0 = 1;
	triag.bChangedMonoTwoTheta = triag.bChangedAnaTwoTheta = 1;

	m_vecAtoms.clear();
	m_vecDeadAngles.clear();
	if(m_sceneReal.GetTasLayout())
		m_sceneReal.GetTasLayout()->SetDeadAngles(&m_vecDeadAngles);

	m_strCurFile = "";
	setWindowTitle(s_strTitle.c_str());

	clear_global_paths();
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

	clear_global_paths();
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

		for(std::size_t iEditBox=0; iEditBox<pVec->size(); ++iEditBox)
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
		t_real dVal = xml.Query<t_real>((strXmlRoot+m_vecSpinBoxNamesSample[iSpinBox]).c_str(), 0., &bOk);
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
	if(xml.Exists((strXmlRoot + "real").c_str()))
	{
		t_real dRealScale = xml.Query<t_real>((strXmlRoot + "real/pixels_per_cm").c_str(), 0., &bOk);
		if(bOk)
			m_sceneReal.GetTasLayout()->SetScaleFactor(dRealScale);

		unsigned int iNodeReal = 0;
		for(TasLayoutNode *pNode : m_sceneReal.GetTasLayout()->GetNodes())
		{
			std::string strNode = m_sceneReal.GetTasLayout()->GetNodeNames()[iNodeReal];

			bool bOkX=0, bOkY=0;
			t_real dValX = xml.Query<t_real>((strXmlRoot + "real/" + strNode + "_x").c_str(), 0., &bOkX);
			t_real dValY = xml.Query<t_real>((strXmlRoot + "real/" + strNode + "_y").c_str(), 0., &bOkY);

			pNode->setPos(dValX, dValY);
			++iNodeReal;
		}

		int bWSEnabled = xml.Query<int>((strXmlRoot + "real/enable_ws").c_str(), 0, &bOk);
		if(bOk)
			m_pWS->setChecked(bWSEnabled!=0);

		int bRealQEnabled = xml.Query<int>((strXmlRoot + "real/enable_realQDir").c_str(), 0, &bOk);
		if(bOk)
			m_pShowRealQDir->setChecked(bRealQEnabled!=0);
	}


	// scattering triangle
	if(xml.Exists((strXmlRoot + "recip").c_str()))
	{
		t_real dRecipScale = xml.Query<t_real>((strXmlRoot + "recip/pixels_per_A-1").c_str(), 0., &bOk);
		if(bOk)
			m_sceneRecip.GetTriangle()->SetScaleFactor(dRecipScale);

		unsigned int iNodeRecip = 0;
		for(ScatteringTriangleNode *pNode : m_sceneRecip.GetTriangle()->GetNodes())
		{
			std::string strNode = m_sceneRecip.GetTriangle()->GetNodeNames()[iNodeRecip];

			bool bOkX=0, bOkY=0;
			t_real dValX = xml.Query<t_real>((strXmlRoot + "recip/" + strNode + "_x").c_str(), 0., &bOkX);
			t_real dValY = xml.Query<t_real>((strXmlRoot + "recip/" + strNode + "_y").c_str(), 0., &bOkY);

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
	}


	// sample definitions
	if(xml.Exists((strXmlRoot + "sample").c_str()))
	{
		std::string strSpaceGroup = xml.Query<std::string>((strXmlRoot + "sample/spacegroup").c_str(), "", &bOk);
		tl::trim(strSpaceGroup);
		if(bOk)
		{
			editSpaceGroupsFilter->clear();
			RepopulateSpaceGroups();

			int iSGIdx = find_sg_from_combo(comboSpaceGroups, strSpaceGroup);
			if(iSGIdx >= 0)
				comboSpaceGroups->setCurrentIndex(iSGIdx);
			else
				comboSpaceGroups->setCurrentIndex(0);
		}


		m_vecAtoms.clear();
		std::size_t iNumAtoms = xml.Query<std::size_t>((strXmlRoot + "sample/atoms/num").c_str(), 0, &bOk);
		if(bOk)
		{
			m_vecAtoms.reserve(iNumAtoms);

			for(std::size_t iAtom=0; iAtom<iNumAtoms; ++iAtom)
			{
				xtl::AtomPos<t_real> atom;
				atom.vecPos.resize(3,0);

				std::string strNr = tl::var_to_str(iAtom);
				atom.strAtomName = xml.Query<std::string>((strXmlRoot + "sample/atoms/" + strNr + "/name").c_str(), "");
				atom.vecPos[0] = xml.Query<t_real>((strXmlRoot + "sample/atoms/" + strNr + "/x").c_str(), 0.);
				atom.vecPos[1] = xml.Query<t_real>((strXmlRoot + "sample/atoms/" + strNr + "/y").c_str(), 0.);
				atom.vecPos[2] = xml.Query<t_real>((strXmlRoot + "sample/atoms/" + strNr + "/z").c_str(), 0.);

				m_vecAtoms.push_back(atom);
			}

			if(m_pAtomsDlg) m_pAtomsDlg->SetAtoms(m_vecAtoms);
		}
	}


	// dead angles
	m_vecDeadAngles.clear();
	unsigned int iNumAngles = xml.Query<unsigned int>((strXmlRoot + "deadangles/num").c_str(), 0, &bOk);
	if(bOk)
	{
		m_vecDeadAngles.reserve(iNumAngles);

		for(unsigned int iAngle=0; iAngle<iNumAngles; ++iAngle)
		{
			DeadAngle<t_real> angle;

			std::string strNr = tl::var_to_str(iAngle);
			angle.dAngleStart = xml.Query<t_real>((strXmlRoot + "deadangles/" + strNr + "/start").c_str(), 0.);
			angle.dAngleEnd = xml.Query<t_real>((strXmlRoot + "deadangles/" + strNr + "/end").c_str(), 0.);
			angle.dAngleOffs = xml.Query<t_real>((strXmlRoot + "deadangles/" + strNr + "/offs").c_str(), 0.);
			angle.iCentreOn = xml.Query<int>((strXmlRoot + "deadangles/" + strNr + "/centreon").c_str(), 1);
			angle.iRelativeTo = xml.Query<int>((strXmlRoot + "deadangles/" + strNr + "/relativeto").c_str(), 0);

			m_vecDeadAngles.push_back(angle);
		}

		if(m_pDeadAnglesDlg)
			m_pDeadAnglesDlg->SetDeadAngles(m_vecDeadAngles);
		if(m_sceneReal.GetTasLayout())
			m_sceneReal.GetTasLayout()->SetDeadAngles(&m_vecDeadAngles);
	}


	// goto dialog
	if(m_pGotoDlg)
		m_pGotoDlg->ClearList();

	if(xml.Exists((strXmlRoot + "goto_favlist").c_str()) ||
		xml.Exists((strXmlRoot + "goto_pos").c_str()))
	{
		InitGoto();
		m_pGotoDlg->Load(xml, strXmlRoot);
	}


	// reso dialog
	if(xml.Exists((strXmlRoot + "reso").c_str()))
	{
		InitReso();
		m_pReso->SetUpdateOn(0,0);
		m_pReso->Load(xml, strXmlRoot);
	}


	// convo dialog
	if(xml.Exists((strXmlRoot + "monteconvo").c_str()))
	{
		InitResoConv();
		m_pConvoDlg->Load(xml, strXmlRoot);
	}


	m_strCurFile = strFile1;
	setWindowTitle((s_strTitle + " - " + m_strCurFile).c_str());

	RecentFiles recent(&m_settings, "main/recent");
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
		= {&m_vecEdits_real, &m_vecEdits_recip, &m_vecEdits_plane, &m_vecEdits_monoana};
	std::vector<const std::vector<std::string>*> vecEditNames
		= {&m_vecEditNames_real, &m_vecEditNames_recip, &m_vecEditNames_plane, &m_vecEditNames_monoana};
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
	t_real dRealScale = m_sceneReal.GetTasLayout()->GetScaleFactor();
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
	t_real dRecipScale = m_sceneRecip.GetTriangle()->GetScaleFactor();
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


	// space group
	xtl::SpaceGroup<t_real> *pSpaceGroup = nullptr;
	std::string strSG = "-1";
	int iSpaceGroupIdx = comboSpaceGroups->currentIndex();
	if(iSpaceGroupIdx != 0)
		pSpaceGroup = (xtl::SpaceGroup<t_real>*)comboSpaceGroups->itemData(iSpaceGroupIdx).value<void*>();
	if(pSpaceGroup)
		strSG = pSpaceGroup->GetName();
	mapConf[strXmlRoot + "sample/spacegroup"] = strSG;


	// atom positions
	mapConf[strXmlRoot + "sample/atoms/num"] = tl::var_to_str(m_vecAtoms.size());
	for(unsigned int iAtom=0; iAtom<m_vecAtoms.size(); ++iAtom)
	{
		const xtl::AtomPos<t_real>& atom = m_vecAtoms[iAtom];

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


	// dead angles
	mapConf[strXmlRoot + "deadangles/num"] = tl::var_to_str(m_vecDeadAngles.size());
	for(unsigned int iAngle=0; iAngle<m_vecDeadAngles.size(); ++iAngle)
	{
		const DeadAngle<t_real>& angle = m_vecDeadAngles[iAngle];

		std::string strAtomNr = tl::var_to_str(iAngle);
		mapConf[strXmlRoot + "deadangles/" + strAtomNr + "/start"] =
			tl::var_to_str(angle.dAngleStart);
		mapConf[strXmlRoot + "deadangles/" + strAtomNr + "/end"] =
			tl::var_to_str(angle.dAngleEnd);
		mapConf[strXmlRoot + "deadangles/" + strAtomNr + "/offs"] =
			tl::var_to_str(angle.dAngleOffs);
		mapConf[strXmlRoot + "deadangles/" + strAtomNr + "/centreon"] =
			tl::var_to_str(angle.iCentreOn);
		mapConf[strXmlRoot + "deadangles/" + strAtomNr + "/relativeto"] =
			tl::var_to_str(angle.iRelativeTo);
	}


	// meta data
	mapConf[strXmlRoot + "meta/timestamp"] = tl::var_to_str<t_real>(tl::epoch<t_real>());
	mapConf[strXmlRoot + "meta/version"] = TAKIN_VER;
	mapConf[strXmlRoot + "meta/info"] = "Created with Takin.";


	// dialogs
	if(m_pReso) m_pReso->Save(mapConf, strXmlRoot);
	if(m_pConvoDlg) m_pConvoDlg->Save(mapConf, strXmlRoot);
	if(m_pGotoDlg) m_pGotoDlg->Save(mapConf, strXmlRoot);
	//if(m_pPowderDlg) m_pPowderDlg->Save(mapConf, strXmlRoot);


	tl::Prop<std::string> xml;
	xml.Add(mapConf);
	if(!xml.Save(m_strCurFile.c_str(), tl::PropType::XML))
	{
		QMessageBox::critical(this, "Error", "Could not save configuration file.");
		return false;
	}

	RecentFiles recent(&m_settings, "main/recent");
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
		if(tl::get_fileext(strFile1,1) != "taz")
			strFile1 += ".taz";

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

	std::unique_ptr<QFileDialog> pdlg;
	if(bShowPreview)
		pdlg.reset(new FilePreviewDlg(this, "Import Data File...", &m_settings));
	else
		pdlg.reset(new QFileDialog(this, "Import Data File..."));

	pdlg->setOptions(fileopt);
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
		std::unique_ptr<tl::FileInstrBase<t_real>> ptrDat(
			tl::FileInstrBase<t_real>::LoadInstr(pcFile));
		tl::FileInstrBase<t_real>* pdat = ptrDat.get();
		if(!pdat)
			return false;

		std::array<t_real, 3> arrLatt = pdat->GetSampleLattice();
		std::array<t_real, 3> arrAng = pdat->GetSampleAngles();
		std::array<bool, 3> arrSenses = pdat->GetScatterSenses();
		std::array<t_real, 2> arrD = pdat->GetMonoAnaD();
		std::array<t_real, 3> arrPeak0 = pdat->GetScatterPlane0();
		std::array<t_real, 3> arrPeak1 = pdat->GetScatterPlane1();

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
		editSpaceGroupsFilter->clear();
		RepopulateSpaceGroups();

		std::string strSpaceGroup = pdat->GetSpacegroup();
		tl::trim(strSpaceGroup);

		int iSGIdx = find_sg_from_combo(comboSpaceGroups, strSpaceGroup);
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
				std::array<t_real, 5> arrScan = pdat->GetScanHKLKiKf(iScan);
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

	RecentFiles recent(&m_settings, "main/recent_import");
	recent.AddFile(strFile1.c_str());
	recent.SaveList();
	recent.FillMenu(m_pMenuRecentImport, m_pMapperRecentImport);

	CalcPeaks();

	if(iScanNum && m_pGotoDlg)
	{
		if(m_pGotoDlg->GotoPos(0))
			focus_dlg(m_pGotoDlg);
	}

	return true;
}

void TazDlg::ShowScanViewer()
{
	if(!m_pScanViewer)
		m_pScanViewer = new ScanViewerDlg(this);

	focus_dlg(m_pScanViewer);
}

void TazDlg::ShowScanPos()
{
	if(!m_pScanPos)
		m_pScanPos = new ScanPosDlg(this, &m_settings);

	focus_dlg(m_pScanPos);
}

void TazDlg::ShowPowderFit()
{
	if(!m_pPowderFit)
		m_pPowderFit = new PowderFitDlg(this, &m_settings);

	focus_dlg(m_pPowderFit);
}
