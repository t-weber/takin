/**
 * Real Space Parameters
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 2014-2016
 * @license GPLv2
 */

#include "RealParamDlg.h"
#include "tlibs/string/string.h"
#include "tlibs/math/math.h"
#include "tlibs/math/linalg.h"
#include "tlibs/phys/neutrons.h"
#include "tlibs/phys/atoms.h"
#include "tlibs/phys/nn.h"
#include "tlibs/string/spec_char.h"
#include "tlibs/string/string.h"


using t_real = t_real_glob;
using t_mat = ublas::matrix<t_real>;
using t_vec = ublas::vector<t_real>;


RealParamDlg::RealParamDlg(QWidget* pParent, QSettings* pSett)
	: QDialog(pParent), m_pSettings(pSett)
{
	this->setupUi(this);

	QObject::connect(editVec1, SIGNAL(textChanged(const QString&)), this, SLOT(CalcVecs()));
	QObject::connect(editVec2, SIGNAL(textChanged(const QString&)), this, SLOT(CalcVecs()));

	if(m_pSettings)
	{
		QFont font;
		if(m_pSettings->contains("main/font_gen") &&
			font.fromString(m_pSettings->value("main/font_gen", "").toString()))
			setFont(font);
	}

	if(m_pSettings && m_pSettings->contains("real_params/geo"))
		restoreGeometry(m_pSettings->value("real_params/geo").toByteArray());
}


RealParamDlg::~RealParamDlg() {}


void RealParamDlg::paramsChanged(const RealParams& parms)
{
	this->edit2ThetaM->setText(tl::var_to_str<t_real>(tl::r2d(parms.dMonoTT), g_iPrec).c_str());
	this->editThetaM->setText(tl::var_to_str<t_real>(tl::r2d(parms.dMonoT), g_iPrec).c_str());

	this->edit2ThetaS->setText(tl::var_to_str<t_real>(tl::r2d(parms.dSampleTT), g_iPrec).c_str());
	this->editThetaS->setText(tl::var_to_str<t_real>(tl::r2d(parms.dSampleT), g_iPrec).c_str());

	this->edit2ThetaA->setText(tl::var_to_str<t_real>(tl::r2d(parms.dAnaTT), g_iPrec).c_str());
	this->editThetaA->setText(tl::var_to_str<t_real>(tl::r2d(parms.dAnaT), g_iPrec).c_str());


	this->editLenMonoSample->setText(tl::var_to_str<t_real>(parms.dLenMonoSample, g_iPrec).c_str());
	this->editLenSampleAna->setText(tl::var_to_str<t_real>(parms.dLenSampleAna, g_iPrec).c_str());
	this->editLenAnaDet->setText(tl::var_to_str<t_real>(parms.dLenAnaDet, g_iPrec).c_str());
}


void RealParamDlg::CrystalChanged(const tl::Lattice<t_real>& latt,
	const tl::Lattice<t_real>& recip, const SpaceGroup<t_real>* pSG,
	const std::vector<AtomPos<t_real>>* pAtoms)
{
	// crystal coordinates
	{
		std::ostringstream ostrRealG, ostrRecipG, ostrBasisReal, ostrBasisRecip;
		for(std::ostringstream* postr : {&ostrRealG, &ostrRecipG, &ostrBasisReal, &ostrBasisRecip} )
		{
			postr->precision(g_iPrec);
			(*postr) << "<html><body>\n";
			(*postr) << "<table border=\"0\" width=\"100%\">\n";
		}

		m_matGCov = latt.GetMetricCov();
		//t_mat matGCont = 2.*M_PI*2.*M_PI * latt.GetMetricCont();
		m_matGCont = recip.GetMetricCov();

		// also update vector calculations
		CalcVecs();

		t_mat matReal = latt.GetBaseMatrixCov();
		//t_mat matRecip = 2.*M_PI * latt.GetBaseMatrixCont();
		t_mat matRecip = recip.GetBaseMatrixCov();

		tl::set_eps_0(m_matGCov, g_dEps);
		tl::set_eps_0(m_matGCont, g_dEps);
		tl::set_eps_0(matReal, g_dEps);
		tl::set_eps_0(matRecip, g_dEps);

		auto fktPrintMatrix = [](std::ostream& ostr, const t_mat& mat) -> void
		{
			for(std::size_t i=0; i<mat.size1(); ++i)
			{
				ostr << "<tr>\n";
				for(std::size_t j=0; j<mat.size2(); ++j)
					ostr << "\t<td>" << mat(i,j) << "</td>";
				ostr << "</tr>\n";
			}			
		};

		fktPrintMatrix(ostrRealG, m_matGCov);
		fktPrintMatrix(ostrRecipG, m_matGCont);
		fktPrintMatrix(ostrBasisReal, matReal);
		fktPrintMatrix(ostrBasisRecip, matRecip);

		for(std::ostringstream* postr : {&ostrRealG, &ostrRecipG, &ostrBasisReal, &ostrBasisRecip} )
		{
			(*postr) << "</table></p>\n";
			(*postr) << "</body></html>\n";
		}

		editMetricCov->setHtml(QString::fromUtf8(ostrRealG.str().c_str()));
		editMetricCont->setHtml(QString::fromUtf8(ostrRecipG.str().c_str()));
		editBaseCov->setHtml(QString::fromUtf8(ostrBasisReal.str().c_str()));
		editBaseCont->setHtml(QString::fromUtf8(ostrBasisRecip.str().c_str()));
	}


	// unit cell
	{
		treeNN->clear();
		listAtoms->clear();
		if(!pAtoms || !pSG) return;

		const unsigned iSC = 3;
		const t_real dEpsShell = 0.01;

		const std::wstring strAA = tl::get_spec_char_utf16("AA");
		try
		{
			const t_mat matA = latt.GetBaseMatrixCov();	// frac -> A
			//const t_mat matB = recip.GetBaseMatrixCov();	// rlu -> 1/A
			const std::vector<t_mat>& vecSymTrafos = pSG->GetTrafos();

			// all primitive atoms
			std::vector<t_vec> vecAtoms, vecAtomsUC, vecAtomsUCFrac,
				vecAtomsSC, vecAtomsNN;
			for(const AtomPos<t_real>& atom : *pAtoms)
				vecAtoms.push_back(atom.vecPos);

			// all atoms in unit cell
			std::vector<std::size_t> vecIdxUC, vecIdxSC;
			std::tie(std::ignore, vecAtomsUC, vecAtomsUCFrac, vecIdxUC) =
			tl::generate_all_atoms<t_mat, t_vec, std::vector, std::string>
				(vecSymTrafos, vecAtoms, nullptr, matA,
				t_real(-0.5), t_real(0.5), g_dEps);

			for(t_vec& vecAt : vecAtomsUC) tl::set_eps_0(vecAt, g_dEps);
			for(t_vec& vecAt : vecAtomsUCFrac) tl::set_eps_0(vecAt, g_dEps);

			// fill list widget
			std::size_t iCurAtom = 0;
			for(std::size_t iIdxUC : vecIdxUC)
			{
				const std::string& strName = (*pAtoms)[iIdxUC].strAtomName;
				const t_vec& vecAtom = vecAtomsUC[iCurAtom];
				const t_vec& vecAtomFrac = vecAtomsUCFrac[iCurAtom];

				std::wostringstream ostr;
				ostr.precision(g_iPrec);
				ostr << "(" << (iCurAtom+1) << ") " << tl::str_to_wstr(strName);
				ostr << "\npos = (" << vecAtom[0] << ", " << vecAtom[1] << ", " << vecAtom[2] << ") " << strAA;
				ostr << "\npos = (" << vecAtomFrac[0] << ", " << vecAtomFrac[1] << ", " << vecAtomFrac[2] << ") frac";
				listAtoms->addItem(new QListWidgetItem(QString::fromWCharArray(ostr.str().c_str())));

				++iCurAtom;
			}


			// all atoms in super cell
			std::vector<std::complex<t_real>> vecDummy;
			std::tie(vecAtomsSC, std::ignore, vecIdxSC) =
				tl::generate_supercell<t_vec, std::vector, t_real>
					(latt, vecAtomsUC, vecDummy, iSC);
			std::vector<std::string> vecNamesSC;
			for(std::size_t iIdxSC : vecIdxSC)
				vecNamesSC.push_back((*pAtoms)[vecIdxUC[iIdxSC]].strAtomName);

			// get neighbours of all atoms
			for(const AtomPos<t_real>& atom : *pAtoms)
			{
				const std::string& strName = atom.strAtomName;
				const t_vec& vecCentreFrac = atom.vecPos;
				t_vec vecCentreAA = tl::mult<t_mat, t_vec>(matA, vecCentreFrac);
				if(tl::is_nan_or_inf(vecCentreFrac) || tl::is_nan_or_inf(vecCentreAA))
				{
					tl::log_err("Invalid centre.");
					break;
				}
				tl::set_eps_0(vecCentreAA, g_dEps);

				std::wostringstream ostrCentre;
				ostrCentre.precision(g_iPrec);
				ostrCentre << "(" << vecCentreAA[0] << ", " << vecCentreAA[1] << ", " << vecCentreAA[2] << ") " << strAA;


				QTreeWidgetItem *pWidParent = new QTreeWidgetItem(treeNN);
				pWidParent->setText(0, (strName + " (nearest)").c_str());
				pWidParent->setText(1, QString::fromWCharArray(ostrCentre.str().c_str()));
				pWidParent->setExpanded(1);

				QTreeWidgetItem *pWidParentNN = new QTreeWidgetItem(treeNN);
				pWidParentNN->setText(0, (strName + " (next-nearest)").c_str());
				pWidParentNN->setText(1, QString::fromWCharArray(ostrCentre.str().c_str()));
				pWidParentNN->setExpanded(1);


				// neighbours
				std::vector<std::vector<std::size_t>> vecIdxNN =
					tl::get_neighbours<t_vec, std::vector, t_real>
						(vecAtomsSC, vecCentreAA, dEpsShell);

				// nearest neighbour
				if(vecIdxNN.size() > 1)
				{
					for(std::size_t iIdxNN : vecIdxNN[1])
					{
						t_vec vecThisAA = vecAtomsSC[iIdxNN] - vecCentreAA;
						tl::set_eps_0(vecThisAA, g_dEps);
						const std::string& strThisAtom = vecNamesSC[iIdxNN];

						QTreeWidgetItem *pWidNN = new QTreeWidgetItem(pWidParent);
						pWidNN->setText(0, strThisAtom.c_str());

						std::wostringstream ostr;
						ostr.precision(g_iPrec);
						ostr << "(" << vecThisAA[0] << ", " << vecThisAA[1] << ", " << vecThisAA[2] << ") " << strAA;
						pWidNN->setText(1, QString::fromWCharArray(ostr.str().c_str()));
					}
				}
				// next-nearest neighbour
				if(vecIdxNN.size() > 2)
				{
					for(std::size_t iIdxNN : vecIdxNN[2])
					{
						t_vec vecThisAA = vecAtomsSC[iIdxNN] - vecCentreAA;
						tl::set_eps_0(vecThisAA, g_dEps);
						const std::string& strThisAtom = vecNamesSC[iIdxNN];

						QTreeWidgetItem *pWidNN = new QTreeWidgetItem(pWidParentNN);
						pWidNN->setText(0, strThisAtom.c_str());

						std::wostringstream ostr;
						ostr.precision(g_iPrec);
						ostr << "(" << vecThisAA[0] << ", " << vecThisAA[1] << ", " << vecThisAA[2] << ") " << strAA;
						pWidNN->setText(1, QString::fromWCharArray(ostr.str().c_str()));
					}
				}
			}
		}
		catch(const std::exception& ex)
		{
			tl::log_err(ex.what());
		}
	}
}


void RealParamDlg::CalcVecs()
{
	if(m_matGCont.size1()!=3 || m_matGCov.size1()!=3)
		return;

	auto fktGetVec = [this](QLineEdit* pEdit) -> t_vec
	{
		std::vector<t_real> _vec;
		tl::get_tokens<t_real>(pEdit->text().toStdString(), std::string(",;"), _vec);

		t_vec vec = tl::make_vec<t_vec, std::vector>(_vec);
		vec.resize(3,1);

		return vec;
	};
	
	// get vectors 1 & 2
	t_vec vec1 = fktGetVec(editVec1);
	t_vec vec2 = fktGetVec(editVec2);

	t_real dAngleReal = tl::r2d(tl::vec_angle(m_matGCov, vec1, vec2));
	t_real dAngleRecip = tl::r2d(tl::vec_angle(m_matGCont, vec1, vec2));
	t_real dLen1Real = tl::vec_len(m_matGCov, vec1);
	t_real dLen2Real = tl::vec_len(m_matGCov, vec2);
	t_real dLen1Recip = tl::vec_len(m_matGCont, vec1);
	t_real dLen2Recip = tl::vec_len(m_matGCont, vec2);

	tl::set_eps_0(dAngleReal, g_dEps);	tl::set_eps_0(dAngleRecip, g_dEps);
	tl::set_eps_0(dLen1Real, g_dEps);	tl::set_eps_0(dLen2Real, g_dEps);
	tl::set_eps_0(dLen1Recip, g_dEps);	tl::set_eps_0(dLen2Recip, g_dEps);

	std::ostringstream ostr;
	ostr.precision(g_iPrec);
	ostr << "<html><body>\n";

	ostr << "<p><b>Reciprocal Vectors 1 and 2:</b>\n<ul>\n";
	ostr << "\t<li> Angle: " << dAngleRecip << " deg </li>\n";
	ostr << "\t<li> Length 1: " << dLen1Recip << " 1/A </li>\n";
	ostr << "\t<li> Length 2: " << dLen2Recip << " 1/A </li>\n";
	ostr << "</ul></p>";

	ostr << "<p><b>Real Vectors 1 and 2:</b>\n<ul>\n";
	ostr << "\t<li> Angle: " << dAngleReal << " deg </li>\n";
	ostr << "\t<li> Length 1: " << dLen1Real << " A </li>\n";
	ostr << "\t<li> Length 2: " << dLen2Real << " A </li>\n";
	ostr << "</ul></p>";

	ostr << "</body></html>\n";
	editCalc->setHtml(QString::fromUtf8(ostr.str().c_str()));
}


void RealParamDlg::closeEvent(QCloseEvent *pEvt)
{
	QDialog::closeEvent(pEvt);
}

void RealParamDlg::accept()
{
	if(m_pSettings)
		m_pSettings->setValue("real_params/geo", saveGeometry());

	QDialog::accept();
}

void RealParamDlg::showEvent(QShowEvent *pEvt)
{
	QDialog::showEvent(pEvt);
}


#include "RealParamDlg.moc"
