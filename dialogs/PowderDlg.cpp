/**
 * Powder Line Dialog
 * @author Tobias Weber
 * @date 2013, 2-dec-2014
 * @license GPLv2
 */

#include "PowderDlg.h"
#include "tlibs/math/lattice.h"
#include "tlibs/math/powder.h"
#include "tlibs/math/neutrons.h"
#include "tlibs/math/linalg.h"
#include "tlibs/string/string.h"
#include "tlibs/string/spec_char.h"
#include "libs/formfactors/formfact.h"
#include "libs/qthelper.h"

#include <iostream>
#include <boost/algorithm/string.hpp>

#include <QFileDialog>
#include <QMessageBox>


using t_real = t_real_glob;
static const tl::t_length_si<t_real> angs = tl::get_one_angstrom<t_real>();

namespace ublas = boost::numeric::ublas;
namespace algo = boost::algorithm;

using t_mat = ublas::matrix<t_real>;
using t_vec = ublas::vector<t_real>;

enum : unsigned int
{
	TABLE_ANGLE	= 0,
	TABLE_Q		= 1,
	TABLE_PEAK	= 2,
	TABLE_MULT	= 3,
	TABLE_FN	= 4,
	TABLE_IN	= 5,
	TABLE_FX	= 6,
	TABLE_IX	= 7
};

PowderDlg::PowderDlg(QWidget* pParent, QSettings* pSett)
	: QDialog(pParent), m_pSettings(pSett),
	m_pmapSpaceGroups(SpaceGroups<t_real>::GetInstance()->get_space_groups())
{
	this->setupUi(this);
	if(m_pSettings)
	{
		QFont font;
		if(m_pSettings->contains("main/font_gen") && font.fromString(m_pSettings->value("main/font_gen", "").toString()))
			setFont(font);
	}

	btnAtoms->setEnabled(g_bHasScatlens);


	// -------------------------------------------------------------------------
	// plot stuff
	m_plotwrapN.reset(new QwtPlotWrapper(plotN));
	m_plotwrapN->GetCurve(0)->setTitle("Neutron Powder Pattern");
	m_plotwrapN->GetPlot()->setAxisTitle(QwtPlot::xBottom, "Scattering Angle");
	m_plotwrapN->GetPlot()->setAxisTitle(QwtPlot::yLeft, "Intensity");
	if(m_plotwrapN->HasTrackerSignal())
		connect(m_plotwrapN->GetPicker(), SIGNAL(moved(const QPointF&)),
			this, SLOT(cursorMoved(const QPointF&)));

	m_plotwrapX.reset(new QwtPlotWrapper(plotX));
	m_plotwrapX->GetCurve(0)->setTitle("X-Ray Powder Pattern");
	m_plotwrapX->GetPlot()->setAxisTitle(QwtPlot::xBottom, "Scattering Angle");
	m_plotwrapX->GetPlot()->setAxisTitle(QwtPlot::yLeft, "Intensity");
	if(m_plotwrapX->HasTrackerSignal())
		connect(m_plotwrapX->GetPicker(), SIGNAL(moved(const QPointF&)),
			this, SLOT(cursorMoved(const QPointF&)));
	// -------------------------------------------------------------------------


	btnSave->setIcon(load_icon("res/document-save.svg"));
	btnLoad->setIcon(load_icon("res/document-open.svg"));

	tablePowderLines->horizontalHeader()->setVisible(true);
	tablePowderLines->verticalHeader()->setDefaultSectionSize(tablePowderLines->verticalHeader()->defaultSectionSize()*1.4);
	tablePowderLines->setColumnWidth(TABLE_ANGLE, 75);
	tablePowderLines->setColumnWidth(TABLE_Q, 75);
	tablePowderLines->setColumnWidth(TABLE_PEAK, 75);
	tablePowderLines->setColumnWidth(TABLE_MULT, 50);
	tablePowderLines->setColumnWidth(TABLE_FN, 75);
	tablePowderLines->setColumnWidth(TABLE_FX, 75);
	tablePowderLines->setColumnWidth(TABLE_IN, 75);
	tablePowderLines->setColumnWidth(TABLE_IX, 75);

	std::vector<QLineEdit*> vecEditsUC = {editA, editB, editC, editAlpha, editBeta, editGamma};
	for(QLineEdit* pEdit : vecEditsUC)
	{
		QObject::connect(pEdit, SIGNAL(textEdited(const QString&)), this, SLOT(CheckCrystalType()));
		QObject::connect(pEdit, SIGNAL(textEdited(const QString&)), this, SLOT(CalcPeaks()));
	}
	QObject::connect(spinLam, SIGNAL(valueChanged(double)), this, SLOT(CalcPeaks()));
	QObject::connect(spinOrder, SIGNAL(valueChanged(int)), this, SLOT(CalcPeaks()));
	QObject::connect(checkUniquePeaks, SIGNAL(toggled(bool)), this, SLOT(CalcPeaks()));

	QObject::connect(editSpaceGroupsFilter, SIGNAL(textChanged(const QString&)), this, SLOT(RepopulateSpaceGroups()));
	QObject::connect(comboSpaceGroups, SIGNAL(currentIndexChanged(int)), this, SLOT(SpaceGroupChanged()));

	connect(btnSaveTable, SIGNAL(clicked()), this, SLOT(SaveTable()));
	connect(btnSave, SIGNAL(clicked()), this, SLOT(SavePowder()));
	connect(btnLoad, SIGNAL(clicked()), this, SLOT(LoadPowder()));
	connect(btnAtoms, SIGNAL(clicked()), this, SLOT(ShowAtomDlg()));

	connect(btnSyncKi, SIGNAL(clicked()), this, SLOT(SetExtKi()));
	connect(btnSyncKf, SIGNAL(clicked()), this, SLOT(SetExtKf()));

	m_bDontCalc = 0;
	RepopulateSpaceGroups();
	CalcPeaks();

	setAcceptDrops(1);

	if(m_pSettings && m_pSettings->contains("powder/geo"))
		restoreGeometry(m_pSettings->value("powder/geo").toByteArray());
}

PowderDlg::~PowderDlg()
{}


void PowderDlg::PlotPowderLines(const std::vector<const PowderLine*>& vecLines)
{
	using t_iter = typename std::vector<const PowderLine*>::const_iterator;
	std::pair<t_iter, t_iter> pairMinMax =
		boost::minmax_element(vecLines.begin(), vecLines.end(),
		[](const PowderLine* pLine1, const PowderLine* pLine2) ->bool
		{
			return pLine1->dAngle <= pLine2->dAngle;
		});

	t_real dMinTT = 0., dMaxTT = 0.;
	if(pairMinMax.first!=vecLines.end() && pairMinMax.second!=vecLines.end())
	{
		dMinTT = (*pairMinMax.first)->dAngle;
		dMaxTT = (*pairMinMax.second)->dAngle;

		dMinTT -= tl::d2r(10.);
		dMaxTT += tl::d2r(10.);
		if(dMinTT < 0.) dMinTT = 0.;
	}

	m_vecTT.clear();
	m_vecTTx.clear();
	m_vecInt.clear();
	m_vecIntx.clear();

	m_vecTT.reserve(GFX_NUM_POINTS);
	m_vecTTx.reserve(GFX_NUM_POINTS);
	m_vecInt.reserve(GFX_NUM_POINTS);
	m_vecIntx.reserve(GFX_NUM_POINTS);

	for(unsigned int iPt=0; iPt<GFX_NUM_POINTS; ++iPt)
	{
		t_real dTT = (dMinTT + (dMaxTT - dMinTT)/t_real(GFX_NUM_POINTS)*t_real(iPt));

		t_real dInt = 0., dIntX = 0.;
		for(const PowderLine *pLine : vecLines)
		{
			const t_real dPeakX = pLine->dAngle;

			t_real dPeakInt = pLine->dIn;
			t_real dPeakIntX = pLine->dIx;
			if(dPeakInt < 0.) dPeakInt = 1.;
			if(dPeakIntX < 0.) dPeakIntX = 1.;

			constexpr t_real dSig = 0.25;
			dInt += tl::gauss_model<t_real>(tl::r2d(dTT), tl::r2d(dPeakX), dSig, dPeakInt, 0.);
			dIntX += tl::gauss_model<t_real>(tl::r2d(dTT), tl::r2d(dPeakX), dSig, dPeakIntX, 0.);
		}

		m_vecTT.push_back(tl::r2d(dTT));
		m_vecTTx.push_back(tl::r2d(dTT));
		m_vecInt.push_back(dInt);
		m_vecIntx.push_back(dIntX);
	}

	set_qwt_data<t_real>()(*m_plotwrapN, m_vecTT, m_vecInt);
	set_qwt_data<t_real>()(*m_plotwrapX, m_vecTTx, m_vecIntx);
}


void PowderDlg::CalcPeaks()
{
	try
	{
		if(m_bDontCalc) return;
		static const unsigned int iPrec = g_iPrec;
		const bool bWantUniquePeaks = checkUniquePeaks->isChecked();

		const t_real dA = editA->text().toDouble();
		const t_real dB = editB->text().toDouble();
		const t_real dC = editC->text().toDouble();
		const t_real dAlpha = tl::d2r(editAlpha->text().toDouble());
		const t_real dBeta = tl::d2r(editBeta->text().toDouble());
		const t_real dGamma = tl::d2r(editGamma->text().toDouble());

		const t_real dLam = spinLam->value();
		const int iOrder = spinOrder->value();
		//tl::log_debug("Lambda = ", dLam, ", order = ", iOrder);

		tl::Lattice<t_real> lattice(dA, dB, dC, dAlpha, dBeta, dGamma);
		tl::Lattice<t_real> recip = lattice.GetRecip();

		const t_mat matA = lattice.GetMetric();
		//const t_mat matB = recip.GetMetric();

		const SpaceGroup<t_real>* pSpaceGroup = GetCurSpaceGroup();


		// ----------------------------------------------------------------------------
		// structure factor stuff
		std::shared_ptr<const ScatlenList<t_real>> lstsl = ScatlenList<t_real>::GetInstance();
		std::shared_ptr<const FormfactList<t_real>> lstff = FormfactList<t_real>::GetInstance();

		std::vector<std::string> vecElems;
		std::vector<t_vec> vecAllAtoms, vecAllAtomsFrac;
		std::vector<std::complex<t_real>> vecScatlens;
		std::vector<std::size_t> vecAllAtomTypes;
		std::vector<t_real> vecFormfacts;

		const std::vector<t_mat>* pvecSymTrafos = nullptr;
		if(pSpaceGroup)
			pvecSymTrafos = &pSpaceGroup->GetTrafos();

		if(pvecSymTrafos && pvecSymTrafos->size() && g_bHasFormfacts &&
			g_bHasScatlens && m_vecAtoms.size())
		{
			std::vector<t_vec> vecAtoms;
			std::vector<std::string> vecNames;
			for(std::size_t iAtom=0; iAtom<m_vecAtoms.size(); ++iAtom)
			{
				vecAtoms.push_back(m_vecAtoms[iAtom].vecPos);
				vecNames.push_back(m_vecAtoms[iAtom].strAtomName);
			}

			std::tie(vecElems, vecAllAtoms, vecAllAtomsFrac, vecAllAtomTypes) =
			tl::generate_all_atoms<t_mat, t_vec, std::vector>
				(*pvecSymTrafos, vecAtoms, &vecNames, matA,
				t_real(0), t_real(1), g_dEps);

			for(const std::string& strElem : vecElems)
			{
				const ScatlenList<t_real>::elem_type* pElem = lstsl->Find(strElem);
				vecScatlens.push_back(pElem ? pElem->GetCoherent() : std::complex<t_real>(0.,0.));
				if(!pElem)
					tl::log_err("Element \"", strElem, "\" not found in scattering length table.",
						" Using b=0.");
			}
		}
		// ----------------------------------------------------------------------------


		std::map<std::string, PowderLine> mapPeaks;
		tl::Powder<int, t_real> powder;
		powder.SetRecipLattice(&recip);

		for(int ih=iOrder; ih>=-iOrder; --ih)
			for(int ik=iOrder; ik>=-iOrder; --ik)
				for(int il=iOrder; il>=-iOrder; --il)
				{
					if(ih==0 && ik==0 && il==0) continue;
					if(pSpaceGroup && !pSpaceGroup->HasReflection(ih, ik, il))
						continue;


					t_vec vecBragg = recip.GetPos(ih, ik, il);
					t_real dQ = ublas::norm_2(vecBragg);
					if(tl::is_nan_or_inf<t_real>(dQ)) continue;

					t_real dAngle = 0;
					try
					{
						dAngle = tl::bragg_recip_twotheta(dQ/angs, dLam*angs, t_real(1.)) / tl::get_one_radian<t_real>();
						if(tl::is_nan_or_inf<t_real>(dAngle)) continue;
					}
					catch(const std::exception&)
					{
						continue;
					}

					//std::cout << "Q = " << dQ << ", angle = " << (dAngle/M_PI*180.) << std::endl;


					t_vec vechkl = tl::make_vec({t_real(ih), t_real(ik), t_real(il)});
					t_real dF = -1., dI = -1.;
					t_real dFx = -1., dIx = -1.;

					// ----------------------------------------------------------------------------
					// structure factor stuff
					if(vecScatlens.size())
					{
						std::complex<t_real> cF =
							tl::structfact<t_real, std::complex<t_real>, t_vec, std::vector>
								(vecAllAtoms, vecBragg, vecScatlens);
						t_real dFsq = (std::conj(cF)*cF).real();
						dF = std::sqrt(dFsq);
						tl::set_eps_0(dF, g_dEps);

						t_real dLor = tl::lorentz_factor(dAngle);
						dI = dFsq*dLor;
					}



					vecFormfacts.clear();
					if(g_bHasFormfacts)
					{
						for(std::size_t iAtom=0; iAtom<vecAllAtoms.size(); ++iAtom)
						{
							//const t_vec& vecAtom = vecAllAtoms[iAtom];
							const FormfactList<t_real>::elem_type* pElemff = lstff->Find(vecElems[iAtom]);

							if(pElemff == nullptr)
							{
								tl::log_err("Cannot get form factor for \"", vecElems[iAtom], "\".");
								vecFormfacts.clear();
								break;
							}

							t_real dFF = pElemff->GetFormfact(dQ);
							vecFormfacts.push_back(dFF);
						}
					}

					if(vecFormfacts.size())
					{
						std::complex<t_real> cFx =
							tl::structfact<t_real, t_real, t_vec, std::vector>
								(vecAllAtoms, vecBragg, vecFormfacts);

						t_real dFxsq = (std::conj(cFx)*cFx).real();
						dFx = std::sqrt(dFxsq);
						tl::set_eps_0(dFx, g_dEps);

						t_real dLor = tl::lorentz_factor(dAngle)*tl::lorentz_pol_factor(dAngle);
						dIx = dFxsq*dLor;
					}
					// ----------------------------------------------------------------------------


					bool bHasPeak = 0;
					if(bWantUniquePeaks)
						bHasPeak = powder.HasUniquePeak(ih, ik, il, dF);
					powder.AddPeak(ih, ik, il, dF);
					if(bHasPeak)
						continue;


					// using angle and F as hash for the set
					std::ostringstream ostrAngle;
					ostrAngle.precision(iPrec);
					ostrAngle << tl::r2d(dAngle) << " " << dF;
					std::string strAngle = ostrAngle.str();
					//std::cout << strAngle << std::endl;

					std::ostringstream ostrPeak;
					ostrPeak << "(" << /*std::abs*/(ih) << /*std::abs*/(ik) << /*std::abs*/(il) << ")";

					if(mapPeaks[ostrAngle.str()].strPeaks.length()!=0)
						mapPeaks[strAngle].strPeaks += ", ";

					mapPeaks[strAngle].strPeaks += ostrPeak.str();
					mapPeaks[strAngle].dAngle = dAngle;
					mapPeaks[strAngle].dQ = dQ;

					mapPeaks[strAngle].h = /*std::abs*/(ih);
					mapPeaks[strAngle].k = /*std::abs*/(ik);
					mapPeaks[strAngle].l = /*std::abs*/(il);

					mapPeaks[strAngle].dFn = dF;
					mapPeaks[strAngle].dIn = dI;
					mapPeaks[strAngle].dFx = dFx;
					mapPeaks[strAngle].dIx = dIx;
				}

		//std::cout << powder << std::endl;
		std::vector<const PowderLine*> vecPowderLines;
		//std::cout << "number of peaks: " << mapPeaks.size() << std::endl;
		vecPowderLines.reserve(mapPeaks.size());


		for(auto& pair : mapPeaks)
		{
			pair.second.strAngle = tl::var_to_str<t_real>(tl::r2d(pair.second.dAngle), iPrec);
			pair.second.strQ = tl::var_to_str<t_real>(pair.second.dQ, iPrec);
			pair.second.iMult = powder.GetMultiplicity(pair.second.h, pair.second.k, pair.second.l, pair.second.dFn);

			pair.second.dIn *= t_real(pair.second.iMult);
			pair.second.dIx *= t_real(pair.second.iMult);

			vecPowderLines.push_back(&pair.second);
		}


		std::sort(vecPowderLines.begin(), vecPowderLines.end(),
			[](const PowderLine* pLine1, const PowderLine* pLine2) -> bool
				{ return pLine1->dAngle < pLine2->dAngle; });


		const bool bSortTable = tablePowderLines->isSortingEnabled();
		tablePowderLines->setSortingEnabled(0);

		const int iNumRows = vecPowderLines.size();
		tablePowderLines->setRowCount(iNumRows);

		for(int iRow=0; iRow<iNumRows; ++iRow)
		{
			for(int iCol=0; iCol<8; ++iCol)
			{
				if(!tablePowderLines->item(iRow, iCol))
				{
					if(iCol == TABLE_PEAK)
						tablePowderLines->setItem(iRow, iCol, new QTableWidgetItem());
					else if(iCol == TABLE_MULT)
						tablePowderLines->setItem(iRow, iCol, new QTableWidgetItemWrapper<unsigned int>());
					else
						tablePowderLines->setItem(iRow, iCol, new QTableWidgetItemWrapper<t_real>());
				}
			}

			std::string strMult = tl::var_to_str(vecPowderLines[iRow]->iMult, iPrec);
			std::string strFn, strIn, strFx, strIx;
			if(vecPowderLines[iRow]->dFn >= 0.)
				strFn = tl::var_to_str(vecPowderLines[iRow]->dFn, iPrec);
			if(vecPowderLines[iRow]->dIn >= 0.)
				strIn = tl::var_to_str(vecPowderLines[iRow]->dIn, iPrec);
			if(vecPowderLines[iRow]->dFx >= 0.)
				strFx = tl::var_to_str(vecPowderLines[iRow]->dFx, iPrec);
			if(vecPowderLines[iRow]->dIx >= 0.)
				strIx = tl::var_to_str(vecPowderLines[iRow]->dIx, iPrec);

			tablePowderLines->item(iRow, TABLE_PEAK)->setText(vecPowderLines[iRow]->strPeaks.c_str());
			dynamic_cast<QTableWidgetItemWrapper<t_real>*>(tablePowderLines->item(iRow, TABLE_ANGLE))->
				SetValue(vecPowderLines[iRow]->dAngle, vecPowderLines[iRow]->strAngle);
			dynamic_cast<QTableWidgetItemWrapper<t_real>*>(tablePowderLines->item(iRow, TABLE_Q))->
				SetValue(vecPowderLines[iRow]->dQ, vecPowderLines[iRow]->strQ);
			dynamic_cast<QTableWidgetItemWrapper<unsigned int>*>(tablePowderLines->item(iRow, TABLE_MULT))->
				SetValue(vecPowderLines[iRow]->iMult, strMult);
			dynamic_cast<QTableWidgetItemWrapper<t_real>*>(tablePowderLines->item(iRow, TABLE_FN))->
				SetValue(vecPowderLines[iRow]->dFn, strFn);
			dynamic_cast<QTableWidgetItemWrapper<t_real>*>(tablePowderLines->item(iRow, TABLE_IN))->
				SetValue(vecPowderLines[iRow]->dIn, strIn);
			dynamic_cast<QTableWidgetItemWrapper<t_real>*>(tablePowderLines->item(iRow, TABLE_FX))->
				SetValue(vecPowderLines[iRow]->dFx, strFx);
			dynamic_cast<QTableWidgetItemWrapper<t_real>*>(tablePowderLines->item(iRow, TABLE_IX))->
				SetValue(vecPowderLines[iRow]->dIx, strIx);
		}

		tablePowderLines->setSortingEnabled(bSortTable);
		PlotPowderLines(vecPowderLines);
		labelStatus->setText("OK.");
	}
	catch(const std::exception& ex)
	{
		//labelStatus->setText(QString("Error: ") + ex.what());
		labelStatus->setText("Error.");
		tl::log_err("Cannot calculate powder peaks: ", ex.what());
	}
}


const SpaceGroup<t_real>* PowderDlg::GetCurSpaceGroup() const
{
	SpaceGroup<t_real>* pSpaceGroup = 0;
	int iSpaceGroupIdx = comboSpaceGroups->currentIndex();
	if(iSpaceGroupIdx != 0)
		pSpaceGroup = (SpaceGroup<t_real>*)comboSpaceGroups->itemData(iSpaceGroupIdx).value<void*>();
	return pSpaceGroup;
}

void PowderDlg::SpaceGroupChanged()
{
	m_crystalsys = CrystalSystem::CRYS_NOT_SET;
	std::string strCryTy = "<not set>";

	const SpaceGroup<t_real>* pSpaceGroup = GetCurSpaceGroup();
	if(pSpaceGroup)
	{
		m_crystalsys = pSpaceGroup->GetCrystalSystem();
		strCryTy = pSpaceGroup->GetCrystalSystemName();
	}
	editCrystalSystem->setText(strCryTy.c_str());

	CheckCrystalType();
	CalcPeaks();
}

void PowderDlg::RepopulateSpaceGroups()
{
	if(!m_pmapSpaceGroups)
		return;

	for(int iCnt=comboSpaceGroups->count()-1; iCnt>0; --iCnt)
		comboSpaceGroups->removeItem(iCnt);

	std::string strFilter = editSpaceGroupsFilter->text().toStdString();

	for(const SpaceGroups<t_real>::t_mapSpaceGroups::value_type& pair : *m_pmapSpaceGroups)
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

void PowderDlg::CheckCrystalType()
{
	set_crystal_system_edits(m_crystalsys, editA, editB, editC,
		editAlpha, editBeta, editGamma);
}

void PowderDlg::SavePowder()
{
	const std::string strXmlRoot("taz/");

	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(m_pSettings && !m_pSettings->value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = ".";
	if(m_pSettings)
		m_pSettings->value("powder/last_dir", ".").toString();
	QString qstrFile = QFileDialog::getSaveFileName(this,
		"Save Powder Configuration", strDirLast,
		"TAZ files (*.taz *.TAZ)", nullptr,
		fileopt);

	if(qstrFile == "")
		return;

	std::string strFile = qstrFile.toStdString();
	std::string strDir = tl::get_dir(strFile);

	std::map<std::string, std::string> mapConf;
	Save(mapConf, strXmlRoot);

	tl::Prop<std::string> xml;
	xml.Add(mapConf);

	bool bOk = xml.Save(strFile.c_str(), tl::PropType::XML);
	if(!bOk)
		QMessageBox::critical(this, "Error", "Could not save powder file.");

	if(bOk && m_pSettings)
		m_pSettings->setValue("powder/last_dir", QString(strDir.c_str()));
}

void PowderDlg::dragEnterEvent(QDragEnterEvent *pEvt)
{
	if(pEvt) pEvt->accept();
}

void PowderDlg::dropEvent(QDropEvent *pEvt)
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
			QMessageBox::critical(this, "Error", "Could not load powder file.");
			return;
		}

		const std::string strXmlRoot("taz/");
		Load(xml, strXmlRoot);
	}
}

void PowderDlg::LoadPowder()
{
	const std::string strXmlRoot("taz/");

	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(m_pSettings && !m_pSettings->value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = ".";
	if(m_pSettings)
		strDirLast = m_pSettings->value("powder/last_dir", ".").toString();
	QString qstrFile = QFileDialog::getOpenFileName(this,
		"Open Powder Configuration", strDirLast,
		"TAZ files (*.taz *.TAZ)", nullptr,
		fileopt);
	if(qstrFile == "")
		return;


	std::string strFile = qstrFile.toStdString();
	std::string strDir = tl::get_dir(strFile);

	tl::Prop<std::string> xml;
	if(!xml.Load(strFile.c_str(), tl::PropType::XML))
	{
		QMessageBox::critical(this, "Error", "Could not load powder file.");
		return;
	}

	Load(xml, strXmlRoot);
	if(m_pSettings)
		m_pSettings->setValue("powder/last_dir", QString(strDir.c_str()));
}

void PowderDlg::Save(std::map<std::string, std::string>& mapConf, const std::string& strXmlRoot)
{
	mapConf[strXmlRoot + "sample/a"] = editA->text().toStdString();
	mapConf[strXmlRoot + "sample/b"] = editB->text().toStdString();
	mapConf[strXmlRoot + "sample/c"] = editC->text().toStdString();

	mapConf[strXmlRoot + "sample/alpha"] = editAlpha->text().toStdString();
	mapConf[strXmlRoot + "sample/beta"] = editBeta->text().toStdString();
	mapConf[strXmlRoot + "sample/gamma"] = editGamma->text().toStdString();

	mapConf[strXmlRoot + "powder/maxhkl"] = tl::var_to_str<int>(spinOrder->value(), g_iPrec);
	mapConf[strXmlRoot + "powder/lambda"] = tl::var_to_str<t_real>(spinLam->value(), g_iPrec);

	mapConf[strXmlRoot + "sample/spacegroup"] = comboSpaceGroups->currentText().toStdString();

	// atom positions
	mapConf[strXmlRoot + "sample/atoms/num"] = tl::var_to_str(m_vecAtoms.size());
	for(std::size_t iAtom=0; iAtom<m_vecAtoms.size(); ++iAtom)
	{
		const AtomPos<t_real>& atom = m_vecAtoms[iAtom];

		std::string strAtomNr = tl::var_to_str(iAtom);
		mapConf[strXmlRoot + "sample/atoms/" + strAtomNr + "/name"] =
			atom.strAtomName;
		mapConf[strXmlRoot + "sample/atoms/" + strAtomNr + "/x"] =
			tl::var_to_str(atom.vecPos[0], g_iPrec);
		mapConf[strXmlRoot + "sample/atoms/" + strAtomNr + "/y"] =
			tl::var_to_str(atom.vecPos[1], g_iPrec);
		mapConf[strXmlRoot + "sample/atoms/" + strAtomNr + "/z"] =
			tl::var_to_str(atom.vecPos[2], g_iPrec);
	}
}

void PowderDlg::Load(tl::Prop<std::string>& xml, const std::string& strXmlRoot)
{
	m_bDontCalc = 1;
	bool bOk=0;

	editA->setText(tl::var_to_str(xml.Query<t_real>((strXmlRoot + "sample/a").c_str(), 5., &bOk), g_iPrec).c_str());
	editB->setText(tl::var_to_str(xml.Query<t_real>((strXmlRoot + "sample/b").c_str(), 5., &bOk), g_iPrec).c_str());
	editC->setText(tl::var_to_str(xml.Query<t_real>((strXmlRoot + "sample/c").c_str(), 5., &bOk), g_iPrec).c_str());

	editAlpha->setText(tl::var_to_str(xml.Query<t_real>((strXmlRoot + "sample/alpha").c_str(), 90., &bOk), g_iPrec).c_str());
	editBeta->setText(tl::var_to_str(xml.Query<t_real>((strXmlRoot + "sample/beta").c_str(), 90., &bOk), g_iPrec).c_str());
	editGamma->setText(tl::var_to_str(xml.Query<t_real>((strXmlRoot + "sample/gamma").c_str(), 90., &bOk), g_iPrec).c_str());

	spinOrder->setValue(xml.Query<int>((strXmlRoot + "powder/maxhkl").c_str(), 5, &bOk));
	spinLam->setValue(xml.Query<t_real>((strXmlRoot + "powder/lambda").c_str(), 5., &bOk));

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
			AtomPos<t_real> atom;
			atom.vecPos.resize(3,0);

			std::string strNr = tl::var_to_str(iAtom);
			atom.strAtomName = xml.Query<std::string>((strXmlRoot + "sample/atoms/" + strNr + "/name").c_str(), "");
			atom.vecPos[0] = xml.Query<t_real>((strXmlRoot + "sample/atoms/" + strNr + "/x").c_str(), 0.);
			atom.vecPos[1] = xml.Query<t_real>((strXmlRoot + "sample/atoms/" + strNr + "/y").c_str(), 0.);
			atom.vecPos[2] = xml.Query<t_real>((strXmlRoot + "sample/atoms/" + strNr + "/z").c_str(), 0.);

			m_vecAtoms.push_back(atom);
		}
	}


	m_bDontCalc = 0;
	CalcPeaks();
}


void PowderDlg::SaveTable()
{
	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(m_pSettings && !m_pSettings->value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = m_pSettings ? m_pSettings->value("powder/last_dir_table", ".").toString() : ".";
	QString _strFile = QFileDialog::getSaveFileName(this,
		"Save Table", strDirLast, "Data files (*.dat *.DAT)", nullptr, fileopt);

	std::string strFile = _strFile.toStdString();

	if(strFile != "")
	{
		if(save_table(strFile.c_str(), tablePowderLines))
		{
			std::string strDir = tl::get_dir(strFile);
			m_pSettings->setValue("powder/last_dir_table", QString(strDir.c_str()));
		}
		else
		{
			QMessageBox::critical(this, "Error", "Could not save table data.");
		}
	}
}


void PowderDlg::paramsChanged(const RecipParams& parms)
{
	m_dExtKi = parms.dki;
	m_dExtKf = parms.dkf;
}

void PowderDlg::SetExtKi()
{
	spinLam->setValue(tl::k2lam(m_dExtKi/angs)/angs);
}

void PowderDlg::SetExtKf()
{
	spinLam->setValue(tl::k2lam(m_dExtKf/angs)/angs);
}


void PowderDlg::ApplyAtoms(const std::vector<AtomPos<t_real>>& vecAtoms)
{
	m_vecAtoms = vecAtoms;
	CalcPeaks();
}

void PowderDlg::ShowAtomDlg()
{
	if(!m_pAtomsDlg)
	{
		m_pAtomsDlg = new AtomsDlg(this, m_pSettings);
		m_pAtomsDlg->setWindowTitle(m_pAtomsDlg->windowTitle() + QString(" (Powder)"));

		QObject::connect(m_pAtomsDlg, SIGNAL(ApplyAtoms(const std::vector<AtomPos<t_real_glob>>&)),
			this, SLOT(ApplyAtoms(const std::vector<AtomPos<t_real_glob>>&)));
	}

	m_pAtomsDlg->SetAtoms(m_vecAtoms);
	m_pAtomsDlg->show();
	m_pAtomsDlg->activateWindow();
}

void PowderDlg::cursorMoved(const QPointF& pt)
{
	const t_real dX = pt.x();
	const t_real dY = pt.y();

	const std::wstring strTh = tl::get_spec_char_utf16("theta");
	std::wostringstream ostr;
	ostr.precision(g_iPrecGfx);
	ostr << L"2" << strTh << L" = " << dX << L", ";
	ostr << L"I = " << dY;

	labelStatus->setText(QString::fromWCharArray(ostr.str().c_str()));
}

void PowderDlg::accept()
{
	if(m_pSettings)
		m_pSettings->setValue("powder/geo", saveGeometry());

	QDialog::accept();
}

void PowderDlg::showEvent(QShowEvent *pEvt)
{
	QDialog::showEvent(pEvt);
}


#include "PowderDlg.moc"
