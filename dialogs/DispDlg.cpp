/**
 * Dispersion Dialog
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date may-2016
 * @license GPLv2
 */

#include "DispDlg.h"

#include "tlibs/phys/lattice.h"
#include "tlibs/phys/atoms.h"
#include "tlibs/phys/nn.h"
#include "tlibs/phys/mag.h"
#include "tlibs/phys/neutrons.h"
#include "tlibs/math/linalg.h"
#include "tlibs/string/string.h"
#include "tlibs/string/spec_char.h"
#include "libs/formfactors/formfact.h"

#include <iostream>
#include <boost/algorithm/string.hpp>

#include <QFileDialog>
#include <QMessageBox>
#include <QMimeData>

using t_real = t_real_glob;
static const auto one_meV = tl::get_one_meV<t_real>();
static const auto kelvin = tl::get_one_kelvin<t_real>();
static const auto k_B = tl::get_kB<t_real>();

namespace ublas = boost::numeric::ublas;
namespace algo = boost::algorithm;

using t_mat = ublas::matrix<t_real>;
using t_vec = ublas::vector<t_real>;
using t_cplx = std::complex<t_real>;

enum : unsigned
{
	TABLE_NN_ORDER	= 0,
	TABLE_NN_ATOM	= 1,
	TABLE_NN_X		= 2,
	TABLE_NN_Y		= 3,
	TABLE_NN_Z		= 4,
	TABLE_NN_DIST	= 5
};


DispDlg::DispDlg(QWidget* pParent, QSettings* pSett)
	: QDialog(pParent), m_pSettings(pSett),
	m_pmapSpaceGroups(xtl::SpaceGroups<t_real>::GetInstance()->get_space_groups())
{
	this->setupUi(this);
	if(m_pSettings)
	{
		QFont font;
		if(m_pSettings->contains("main/font_gen") && font.fromString(m_pSettings->value("main/font_gen", "").toString()))
			setFont(font);
	}

	btnSave->setIcon(load_icon("res/icons/document-save.svg"));
	btnLoad->setIcon(load_icon("res/icons/document-open.svg"));

	//tableNN->sortByColumn(TABLE_NN_ORDER);
	tableNN->sortItems(TABLE_NN_ORDER);
	tableNN->verticalHeader()->setDefaultSectionSize(tableNN->verticalHeader()->defaultSectionSize()*0.8);
	tableNN->horizontalHeader()->setVisible(true);
	tableNN->setColumnWidth(TABLE_NN_ORDER, 75);
	tableNN->setColumnWidth(TABLE_NN_ATOM, 75);
	tableNN->setColumnWidth(TABLE_NN_X, 75);
	tableNN->setColumnWidth(TABLE_NN_Y, 75);
	tableNN->setColumnWidth(TABLE_NN_Z, 75);
	tableNN->setColumnWidth(TABLE_NN_DIST, 75);

	m_plotwrapFerro.reset(new QwtPlotWrapper(plotFerro));
	m_plotwrapFerro->GetCurve(0)->setTitle("Ferromagnetic Dispersion");
	m_plotwrapFerro->GetPlot()->setAxisTitle(QwtPlot::xBottom, "q (rlu)");
	m_plotwrapFerro->GetPlot()->setAxisTitle(QwtPlot::yLeft, "E (meV)");
	if(m_plotwrapFerro->HasTrackerSignal())
		connect(m_plotwrapFerro->GetPicker(), SIGNAL(moved(const QPointF&)),
			this, SLOT(cursorMoved(const QPointF&)));

	spinCentreIdx->setMinimum(0);

	std::vector<QLineEdit*> vecEditsUC = {editA, editB, editC, editAlpha, editBeta, editGamma};
	for(QLineEdit* pEdit : vecEditsUC)
	{
		QObject::connect(pEdit, SIGNAL(textEdited(const QString&)), this, SLOT(CheckCrystalType()));
		QObject::connect(pEdit, SIGNAL(textEdited(const QString&)), this, SLOT(Calc()));
	}

	QObject::connect(editEpsShell, SIGNAL(textEdited(const QString&)), this, SLOT(Calc()));

	QObject::connect(editSpaceGroupsFilter, SIGNAL(textChanged(const QString&)), this, SLOT(RepopulateSpaceGroups()));
	QObject::connect(comboSpaceGroups, SIGNAL(currentIndexChanged(int)), this, SLOT(SpaceGroupChanged()));

	QObject::connect(editS, SIGNAL(textChanged(const QString&)), this, SLOT(Calc()));
	QObject::connect(editJ, SIGNAL(textChanged(const QString&)), this, SLOT(Calc()));
	QObject::connect(editqDir, SIGNAL(textChanged(const QString&)), this, SLOT(Calc()));

	std::vector<QSpinBox*> vecSpins = {spinNN, spinSC, spinCentreIdx};
	for(QSpinBox* pSpin : vecSpins)
		QObject::connect(pSpin, SIGNAL(valueChanged(int)), this, SLOT(Calc()));

	QObject::connect(btnSave, SIGNAL(clicked()), this, SLOT(Save()));
	QObject::connect(btnLoad, SIGNAL(clicked()), this, SLOT(Load()));
	QObject::connect(btnAtoms, SIGNAL(clicked()), this, SLOT(ShowAtomDlg()));

	m_bDontCalc = 0;
	RepopulateSpaceGroups();
	Calc();

	setAcceptDrops(1);

	if(m_pSettings && m_pSettings->contains("disp/geo"))
		restoreGeometry(m_pSettings->value("disp/geo").toByteArray());
}


DispDlg::~DispDlg()
{
	ClearPlots();
	setAcceptDrops(0);
	m_bDontCalc = 1;
	m_pmapSpaceGroups = nullptr;

	// ----------------------------------------------
	// hack to circumvent bizarre object deletion bug
	delete btnAtoms; btnAtoms = 0;
	delete btnLoad; btnLoad = 0;
	delete btnSave; btnSave = 0;
	m_plotwrapFerro.reset(nullptr);
	delete plotFerro; plotFerro = 0;
	delete tabWidget; tabWidget = 0;
	// ----------------------------------------------
}


void DispDlg::ClearPlots()
{
	static const std::vector<t_real> vecZero;

	if(m_plotwrapFerro)
		set_qwt_data<t_real>()(*m_plotwrapFerro, vecZero, vecZero);
}


void DispDlg::Calc()
{
	if(m_bDontCalc) return;
	try
	{
		if(m_vecAtoms.size() == 0)
		{
			tl::log_err("No atoms defined.");
			return;
		}

		const t_real dEpsShell = editEpsShell->text().toDouble();

		const t_real dA = editA->text().toDouble();
		const t_real dB = editB->text().toDouble();
		const t_real dC = editC->text().toDouble();
		const t_real dAlpha = tl::d2r(editAlpha->text().toDouble());
		const t_real dBeta = tl::d2r(editBeta->text().toDouble());
		const t_real dGamma = tl::d2r(editGamma->text().toDouble());

		const t_real dS = editS->text().toDouble();
		std::string strJ = editJ->text().toStdString();
		std::string strqDir = editqDir->text().toStdString();
		std::vector<t_real> vecJbyOrder, vecqDir;
		tl::get_tokens<t_real, std::string, std::vector<t_real>>(strJ, ",;", vecJbyOrder);
		tl::get_tokens<t_real, std::string, std::vector<t_real>>(strqDir, ",;", vecqDir);
		while(vecqDir.size() < 3)
			vecqDir.push_back(t_real(0));

		const int iNN = spinNN->value();
		const int iSC = spinSC->value();
		int iIdxCentre = spinCentreIdx->value();
		if(iIdxCentre >= int(m_vecAtoms.size())) iIdxCentre = int(m_vecAtoms.size())-1;
		if(iIdxCentre < 0) iIdxCentre = 0;

		// lattice
		tl::Lattice<t_real> lattice(dA, dB, dC, dAlpha, dBeta, dGamma);
		tl::Lattice<t_real> recip = lattice.GetRecip();

		const t_mat matA = lattice.GetBaseMatrixCov();
		const t_mat matB = recip.GetBaseMatrixCov();

		t_vec vecCentre = tl::mult<t_mat,t_vec>(matA, m_vecAtoms[iIdxCentre].vecPos);
		if(tl::is_nan_or_inf(vecCentre))
		{
			tl::log_err("Invalid centre.");
			return;
		}
		//tl::log_debug("Centre: ", vecCentre);


		// spacegroup
		const xtl::SpaceGroup<t_real> *pSpaceGroup = GetCurSpaceGroup();
		if(!pSpaceGroup)
		{
			tl::log_err("No spacegroup defined.");
			return;
		}
		const std::vector<t_mat>& vecSymTrafos = pSpaceGroup->GetTrafos();


		// all primitive atoms
		std::vector<t_vec> vecAtoms, vecAtomsUC, vecAtomsSC, vecAtomsNN;
		std::vector<t_cplx> vecJUC, vecJNN;
		for(const xtl::AtomPos<t_real>& atom : m_vecAtoms)
			vecAtoms.push_back(atom.vecPos);

		// all atoms in unit cell
		std::vector<std::size_t> vecIdxUC, vecIdxSC;
		std::tie(std::ignore, vecAtomsUC, std::ignore, vecIdxUC) =
		tl::generate_all_atoms<t_mat, t_vec, std::vector, std::string>
			(vecSymTrafos, vecAtoms, nullptr, matA, g_dEps);


		// all atoms in super cell
		std::tie(vecAtomsSC, std::ignore, vecIdxSC) =
			tl::generate_supercell<t_vec, std::vector, t_real>
				(lattice, vecAtomsUC, vecJUC, iSC);
		std::vector<std::string> vecNamesSC;
		for(std::size_t iIdxSC : vecIdxSC)
			vecNamesSC.push_back(m_vecAtoms[vecIdxUC[iIdxSC]].strAtomName);


		// neighbours
		std::vector<std::vector<std::size_t>> vecIdxNN =
			tl::get_neighbours<t_vec, std::vector, t_real>
				(vecAtomsSC, vecCentre, dEpsShell);
		std::size_t iNumRows = 0;
		for(const auto& vecIdxNNInner : vecIdxNN) iNumRows += vecIdxNNInner.size();


		//----------------------------------------------------------------------
		// neighbours & table
		const bool bSortTable = tableNN->isSortingEnabled();
		tableNN->setSortingEnabled(0);
		tableNN->setRowCount(iNumRows);

		int iRow = 0;
		std::size_t iOuterIdx = 0;
		for(const auto& vecIdxNNInner : vecIdxNN)
		{
			for(std::size_t iIdx : vecIdxNNInner)
			{
				const t_vec& vecThisAtom = vecAtomsSC[iIdx];
				const std::string& strThisAtom = vecNamesSC[iIdx];

				if(iOuterIdx > 0 && int(iOuterIdx) <= iNN)
				{
					vecAtomsNN.push_back(vecThisAtom - vecCentre);

					t_real dJ = iOuterIdx > vecJbyOrder.size()
						? t_real(0) : vecJbyOrder[iOuterIdx-1];
					t_cplx J = dJ * t_real(k_B / one_meV * kelvin);
					vecJNN.push_back(J);
				}

				if(!tableNN->item(iRow, TABLE_NN_ATOM))
					tableNN->setItem(iRow, TABLE_NN_ATOM, new QTableWidgetItem());
				if(!tableNN->item(iRow, TABLE_NN_ORDER))
					tableNN->setItem(iRow, TABLE_NN_ORDER, new QTableWidgetItemWrapper<std::size_t>());
				if(!tableNN->item(iRow, TABLE_NN_X))
					tableNN->setItem(iRow, TABLE_NN_X, new QTableWidgetItemWrapper<t_real>());
				if(!tableNN->item(iRow, TABLE_NN_Y))
					tableNN->setItem(iRow, TABLE_NN_Y, new QTableWidgetItemWrapper<t_real>());
				if(!tableNN->item(iRow, TABLE_NN_Z))
					tableNN->setItem(iRow, TABLE_NN_Z, new QTableWidgetItemWrapper<t_real>());
				if(!tableNN->item(iRow, TABLE_NN_DIST))
					tableNN->setItem(iRow, TABLE_NN_DIST, new QTableWidgetItemWrapper<t_real>());

				tableNN->item(iRow, TABLE_NN_ATOM)->setText(strThisAtom.c_str());
				dynamic_cast<QTableWidgetItemWrapper<std::size_t>*>(
					tableNN->item(iRow, TABLE_NN_ORDER))->SetValue(iOuterIdx);
				dynamic_cast<QTableWidgetItemWrapper<t_real>*>(
					tableNN->item(iRow, TABLE_NN_X))->SetValue(vecThisAtom[0]);
				dynamic_cast<QTableWidgetItemWrapper<t_real>*>(
					tableNN->item(iRow, TABLE_NN_Y))->SetValue(vecThisAtom[1]);
				dynamic_cast<QTableWidgetItemWrapper<t_real>*>(
					tableNN->item(iRow, TABLE_NN_Z))->SetValue(vecThisAtom[2]);
				dynamic_cast<QTableWidgetItemWrapper<t_real>*>(
					tableNN->item(iRow, TABLE_NN_DIST))->SetValue(ublas::norm_2(vecThisAtom - vecCentre));

				++iRow;
			}
			++iOuterIdx;
		}

		tableNN->setSortingEnabled(bSortTable);
		labStatus->setText("OK.");


		// ---------------------------------------------------------------------
		// ferro plot
		ClearPlots();

		m_vecFerroQ.clear();
		m_vecFerroE.clear();
		m_vecFerroQ.reserve(GFX_NUM_POINTS);
		m_vecFerroE.reserve(GFX_NUM_POINTS);

		for(std::size_t iPt=0; iPt<GFX_NUM_POINTS; ++iPt)
		{
			t_real tPos = t_real(iPt)/t_real(GFX_NUM_POINTS-1) * 1.5;
			t_vec vecq = tl::make_vec<t_vec>({vecqDir[0], vecqDir[1], vecqDir[2]}) * tPos
				/ std::sqrt(vecqDir[0]*vecqDir[0] + vecqDir[1]*vecqDir[1] + vecqDir[2]*vecqDir[2]);
			t_vec vecq_AA = tl::mult<t_mat, t_vec>(matB, vecq);
			t_real dE = tl::ferromag<t_vec, t_real, std::vector>
				(vecAtomsNN, vecJNN, vecq_AA, dS);

			m_vecFerroQ.push_back(ublas::norm_2(vecq));
			m_vecFerroE.push_back(dE);
		}

		if(m_plotwrapFerro)
			set_qwt_data<t_real>()(*m_plotwrapFerro, m_vecFerroQ, m_vecFerroE);
	}
	catch(const std::exception& ex)
	{
		tl::log_err(ex.what());
	}
}


const xtl::SpaceGroup<t_real>* DispDlg::GetCurSpaceGroup() const
{
	xtl::SpaceGroup<t_real> *pSpaceGroup = 0;
	int iSpaceGroupIdx = comboSpaceGroups->currentIndex();
	if(iSpaceGroupIdx != 0)
		pSpaceGroup = (xtl::SpaceGroup<t_real>*)comboSpaceGroups->itemData(iSpaceGroupIdx).value<void*>();
	return pSpaceGroup;
}


void DispDlg::SpaceGroupChanged()
{
	m_crystalsys = xtl::CrystalSystem::CRYS_NOT_SET;
	std::string strCryTy = "<not set>";

	const xtl::SpaceGroup<t_real>* pSpaceGroup = GetCurSpaceGroup();
	if(pSpaceGroup)
	{
		m_crystalsys = pSpaceGroup->GetCrystalSystem();
		strCryTy = pSpaceGroup->GetCrystalSystemName();
	}
	editCrystalSystem->setText(strCryTy.c_str());

	CheckCrystalType();
	Calc();
}


void DispDlg::RepopulateSpaceGroups()
{
	if(!m_pmapSpaceGroups)
		return;

	for(int iCnt=comboSpaceGroups->count()-1; iCnt>0; --iCnt)
		comboSpaceGroups->removeItem(iCnt);

	std::string strFilter = editSpaceGroupsFilter->text().toStdString();

	for(const xtl::SpaceGroups<t_real>::t_mapSpaceGroups::value_type& pair : *m_pmapSpaceGroups)
	{
		const std::string& strName = pair.second.GetName();

		typedef const boost::iterator_range<std::string::const_iterator> t_striterrange;
		if(strFilter!="" &&
				!boost::ifind_first(t_striterrange(strName.begin(), strName.end()),
					t_striterrange(strFilter.begin(), strFilter.end())))
			continue;

		comboSpaceGroups->insertItem(comboSpaceGroups->count(),
			strName.c_str(), QVariant::fromValue((void*)&pair.second));
	}
}


void DispDlg::CheckCrystalType()
{
	if(m_pSettings && m_pSettings->value("main/ignore_xtal_restrictions", 0).toBool())
	{
		for(QLineEdit* pEdit : {editA, editB, editC, editAlpha, editBeta, editGamma})
			pEdit->setEnabled(true);
		return;
	}

	set_crystal_system_edits(m_crystalsys, editA, editB, editC,
		editAlpha, editBeta, editGamma);
}


void DispDlg::Save()
{
	const std::string strXmlRoot("taz/");

	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(m_pSettings && !m_pSettings->value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = ".";
	if(m_pSettings)
		m_pSettings->value("disp/last_dir", ".").toString();
	QString qstrFile = QFileDialog::getSaveFileName(this,
		"Save Dispersion Configuration", strDirLast,
		"TAZ files (*.taz *.TAZ)", nullptr,
		fileopt);

	if(qstrFile == "")
		return;

	std::string strFile = qstrFile.toStdString();
	std::string strDir = tl::get_dir(strFile);
	if(tl::get_fileext(strFile,1) != "taz")
		strFile += ".taz";

	std::map<std::string, std::string> mapConf;
	Save(mapConf, strXmlRoot);

	tl::Prop<std::string> xml;
	xml.Add(mapConf);

	bool bOk = xml.Save(strFile.c_str(), tl::PropType::XML);
	if(!bOk)
		QMessageBox::critical(this, "Error", "Could not save dispersion file.");

	if(bOk && m_pSettings)
		m_pSettings->setValue("disp/last_dir", QString(strDir.c_str()));
}


void DispDlg::dragEnterEvent(QDragEnterEvent *pEvt)
{
	if(pEvt) pEvt->accept();
}


void DispDlg::dropEvent(QDropEvent *pEvt)
{
	if(!pEvt) return;
	const QMimeData* pMime = pEvt->mimeData();
	if(!pMime) return;

	std::string strFiles = pMime->text().toStdString();
	std::vector<std::string> vecFiles;
	tl::get_tokens<std::string, std::string>(strFiles, "\n", vecFiles);
	if(vecFiles.size() > 1)
		tl::log_warn("More than one file dropped, using first one.");

	if(vecFiles.size() >= 1)
	{
		std::string& strFile = vecFiles[0];
		tl::trim(strFile);

		const std::string strHead = "file://";
		if(algo::starts_with(strFile, strHead))
			algo::replace_head(strFile, strHead.length(), "");

		tl::Prop<std::string> xml;
		if(!xml.Load(strFile.c_str(), tl::PropType::XML))
		{
			QMessageBox::critical(this, "Error", "Could not load dispersion file.");
			return;
		}

		const std::string strXmlRoot("taz/");
		Load(xml, strXmlRoot);
	}
}


void DispDlg::Load()
{
	const std::string strXmlRoot("taz/");

	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(m_pSettings && !m_pSettings->value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = ".";
	if(m_pSettings)
		strDirLast = m_pSettings->value("disp/last_dir", ".").toString();
	QString qstrFile = QFileDialog::getOpenFileName(this,
		"Open Dispersion Configuration", strDirLast,
		"TAZ files (*.taz *.TAZ)", nullptr,
		fileopt);
	if(qstrFile == "")
		return;


	std::string strFile = qstrFile.toStdString();
	std::string strDir = tl::get_dir(strFile);

	tl::Prop<std::string> xml;
	if(!xml.Load(strFile.c_str(), tl::PropType::XML))
	{
		QMessageBox::critical(this, "Error", "Could not load dispersion file.");
		return;
	}

	Load(xml, strXmlRoot);
	if(m_pSettings)
		m_pSettings->setValue("disp/last_dir", QString(strDir.c_str()));
}


void DispDlg::Save(std::map<std::string, std::string>& mapConf, const std::string& strXmlRoot)
{
	mapConf[strXmlRoot + "sample/a"] = editA->text().toStdString();
	mapConf[strXmlRoot + "sample/b"] = editB->text().toStdString();
	mapConf[strXmlRoot + "sample/c"] = editC->text().toStdString();

	mapConf[strXmlRoot + "sample/alpha"] = editAlpha->text().toStdString();
	mapConf[strXmlRoot + "sample/beta"] = editBeta->text().toStdString();
	mapConf[strXmlRoot + "sample/gamma"] = editGamma->text().toStdString();

	mapConf[strXmlRoot + "disp/neighbours"] = tl::var_to_str<int>(spinNN->value(), g_iPrec);
	mapConf[strXmlRoot + "disp/supercell"] = tl::var_to_str<int>(spinSC->value(), g_iPrec);
	mapConf[strXmlRoot + "disp/centre_idx"] = tl::var_to_str<int>(spinCentreIdx->value(), g_iPrec);
	mapConf[strXmlRoot + "disp/S"] = editS->text().toStdString();
	mapConf[strXmlRoot + "disp/J"] = editJ->text().toStdString();
	mapConf[strXmlRoot + "disp/q_dir"] = editqDir->text().toStdString();
	mapConf[strXmlRoot + "disp/eps_shell"] = editEpsShell->text().toStdString();

	mapConf[strXmlRoot + "sample/spacegroup"] = comboSpaceGroups->currentText().toStdString();

	// atom positions
	mapConf[strXmlRoot + "sample/atoms/num"] = tl::var_to_str(m_vecAtoms.size());
	for(std::size_t iAtom=0; iAtom<m_vecAtoms.size(); ++iAtom)
	{
		const xtl::AtomPos<t_real>& atom = m_vecAtoms[iAtom];

		std::string strAtomNr = tl::var_to_str(iAtom);
		mapConf[strXmlRoot + "sample/atoms/" + strAtomNr + "/name"] =
			atom.strAtomName;
		mapConf[strXmlRoot + "sample/atoms/" + strAtomNr + "/x"] =
			tl::var_to_str(atom.vecPos[0], g_iPrec);
		mapConf[strXmlRoot + "sample/atoms/" + strAtomNr + "/y"] =
			tl::var_to_str(atom.vecPos[1], g_iPrec);
		mapConf[strXmlRoot + "sample/atoms/" + strAtomNr + "/z"] =
			tl::var_to_str(atom.vecPos[2], g_iPrec);
		//mapConf[strXmlRoot + "sample/atoms/" + strAtomNr + "/J"] =
		//	tl::var_to_str(atom.J, g_iPrec);
	}
}


void DispDlg::Load(tl::Prop<std::string>& xml, const std::string& strXmlRoot)
{
	m_bDontCalc = 1;
	bool bOk=0;

	editA->setText(tl::var_to_str(xml.Query<t_real>((strXmlRoot + "sample/a").c_str(), 5., &bOk), g_iPrec).c_str());
	editB->setText(tl::var_to_str(xml.Query<t_real>((strXmlRoot + "sample/b").c_str(), 5., &bOk), g_iPrec).c_str());
	editC->setText(tl::var_to_str(xml.Query<t_real>((strXmlRoot + "sample/c").c_str(), 5., &bOk), g_iPrec).c_str());

	editAlpha->setText(tl::var_to_str(xml.Query<t_real>((strXmlRoot + "sample/alpha").c_str(), 90., &bOk), g_iPrec).c_str());
	editBeta->setText(tl::var_to_str(xml.Query<t_real>((strXmlRoot + "sample/beta").c_str(), 90., &bOk), g_iPrec).c_str());
	editGamma->setText(tl::var_to_str(xml.Query<t_real>((strXmlRoot + "sample/gamma").c_str(), 90., &bOk), g_iPrec).c_str());

	spinNN->setValue(xml.Query<int>((strXmlRoot + "disp/neighbours").c_str(), 2, &bOk));
	spinSC->setValue(xml.Query<int>((strXmlRoot + "disp/supercell").c_str(), 2, &bOk));
	spinCentreIdx->setValue(xml.Query<int>((strXmlRoot + "disp/centre_idx").c_str(), 0, &bOk));

	editS->setText(tl::var_to_str(xml.Query<t_real>((strXmlRoot + "disp/S").c_str(), 0.5, &bOk), g_iPrec).c_str());
	editJ->setText(xml.Query<std::string>((strXmlRoot + "disp/J").c_str(), "1", &bOk).c_str());
	editqDir->setText(xml.Query<std::string>((strXmlRoot + "disp/q_dir").c_str(), "1, 0, 0", &bOk).c_str());
	editEpsShell->setText(tl::var_to_str(xml.Query<t_real>((strXmlRoot + "disp/eps_shell").c_str(), 0.01, &bOk), g_iPrec).c_str());

	std::string strSpaceGroup = xml.Query<std::string>((strXmlRoot + "sample/spacegroup").c_str(), "", &bOk);
	tl::trim(strSpaceGroup);
	if(bOk)
	{
		editSpaceGroupsFilter->clear();
		int iSGIdx = comboSpaceGroups->findText(strSpaceGroup.c_str());
		if(iSGIdx >= 0)
			comboSpaceGroups->setCurrentIndex(iSGIdx);
	}


	// atom positions
	m_vecAtoms.clear();
	unsigned int iNumAtoms = xml.Query<unsigned int>((strXmlRoot + "sample/atoms/num").c_str(), 0, &bOk);
	if(bOk)
	{
		m_vecAtoms.reserve(iNumAtoms);

		for(std::size_t iAtom=0; iAtom<std::size_t(iNumAtoms); ++iAtom)
		{
			xtl::AtomPos<t_real> atom;
			atom.vecPos.resize(3,0);

			std::string strNr = tl::var_to_str(iAtom);
			atom.strAtomName = xml.Query<std::string>((strXmlRoot + "sample/atoms/" + strNr + "/name").c_str(), "");
			atom.vecPos[0] = xml.Query<t_real>((strXmlRoot + "sample/atoms/" + strNr + "/x").c_str(), 0.);
			atom.vecPos[1] = xml.Query<t_real>((strXmlRoot + "sample/atoms/" + strNr + "/y").c_str(), 0.);
			atom.vecPos[2] = xml.Query<t_real>((strXmlRoot + "sample/atoms/" + strNr + "/z").c_str(), 0.);
			//atom.J = xml.Query<t_real>((strXmlRoot + "sample/atoms/" + strNr + "/J").c_str(), 0.);

			m_vecAtoms.push_back(atom);
		}

		spinCentreIdx->setMaximum(int(m_vecAtoms.size())-1);
	}

	m_bDontCalc = 0;
	Calc();
}


void DispDlg::ApplyAtoms(const std::vector<xtl::AtomPos<t_real>>& vecAtoms)
{
	m_vecAtoms = vecAtoms;
	spinCentreIdx->setMaximum(int(m_vecAtoms.size())-1);

	Calc();
}

void DispDlg::ShowAtomDlg()
{
	if(!m_pAtomsDlg)
	{
		m_pAtomsDlg = new AtomsDlg(this, m_pSettings, false);
		m_pAtomsDlg->setWindowTitle(m_pAtomsDlg->windowTitle() + QString(" (Dispersion)"));

		QObject::connect(m_pAtomsDlg, SIGNAL(ApplyAtoms(const std::vector<xtl::AtomPos<t_real_glob>>&)),
			this, SLOT(ApplyAtoms(const std::vector<xtl::AtomPos<t_real_glob>>&)));
	}

	m_pAtomsDlg->SetAtoms(m_vecAtoms);

	focus_dlg(m_pAtomsDlg);
}

void DispDlg::cursorMoved(const QPointF& pt)
{
	const t_real dPos[] = { t_real(pt.x()), t_real(pt.y()) };

	//const std::wstring strAA = tl::get_spec_char_utf16("AA") +
	//	tl::get_spec_char_utf16("sup-") + tl::get_spec_char_utf16("sup1");

	std::wostringstream ostr;
	ostr.precision(g_iPrecGfx);
	ostr << L"q = " << dPos[0] << L" " << L"rlu" /*strAA*/ << L", ";
	ostr << L"E = " << dPos[1] << L" meV";

	labStatus->setText(QString::fromWCharArray(ostr.str().c_str()));
}


void DispDlg::accept()
{
	if(m_pSettings)
		m_pSettings->setValue("disp/geo", saveGeometry());

	QDialog::accept();
}


void DispDlg::showEvent(QShowEvent *pEvt)
{
	QDialog::showEvent(pEvt);
}


#include "DispDlg.moc"
