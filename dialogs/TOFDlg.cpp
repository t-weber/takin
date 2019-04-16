/**
 * Component calculations (formerly only TOF-specific)
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date feb-2017
 * @license GPLv2
 */

#include "TOFDlg.h"
#include <boost/units/io.hpp>

#include "tlibs/string/string.h"
#include "tlibs/math/math.h"
#include "tlibs/phys/neutrons.h"

#include <sstream>


using t_real = t_real_glob;
namespace co = boost::units::si::constants::codata;
static const tl::t_length_si<t_real> angs = tl::get_one_angstrom<t_real>();
static const tl::t_length_si<t_real> meter = tl::get_one_meter<t_real>();
static const tl::t_length_si<t_real> cm = meter * t_real(1e-2);
static const tl::t_time_si<t_real> sec = tl::get_one_second<t_real>();
static const tl::t_time_si<t_real> us = sec * t_real(1e-6);
static const tl::t_angle_si<t_real> deg = tl::get_one_deg<t_real>();


TOFDlg::TOFDlg(QWidget* pParent, QSettings *pSett)
	: QDialog(pParent), m_pSettings(pSett)
{
	setupUi(this);

	// -------------------------------------------------------------------------
	// all ui elements
	m_vecRadioNames = { "tof_calc/calc_chopper_l", "tof_calc/calc_chopper_r",
		"tof_calc/calc_chopper_om", "tof_calc/calc_chopper_t",
		"tof_calc/calc_div_l", "tof_calc/calc_div_w", "tof_calc/calc_div_ang",
		"tof_calc/calc_sel_lam", "tof_calc/calc_sel_th", "tof_calc/calc_sel_l",
		"tof_calc/calc_sel_om"
	};
	m_vecEditNames = { "tof_calc/chopper_l", "tof_calc/chopper_r",
		"tof_calc/chopper_om", "tof_calc/chopper_t",
		"tof_calc/div_l", "tof_calc/div_w", "tof_calc/div_ang",
		"tof_calc/sel_lam", "tof_calc/sel_th", "tof_calc/sel_l", "tof_calc/sel_om",
		"tof_foc_l1", "tof_foc_l2", "tof_foc_tt"
	};
	m_vecCheckNames = { "tof_calc/counter_rot" };

	m_vecRadioBoxes = { radioChopperL, radioChopperR, radioChopperOm, radioChopperT,
		radioDivL, radioDivW, radioDivAng,
		radioSelLam, radioSelTh, radioSelL, radioSelOm
	};
	m_vecEditBoxes = { editChopperL, editChopperR, editChopperOm, editChopperT, 
		editDivL, editDivW, editDivAng,
		editSelLam, editSelTh, editSelL, editSelOm,
		editFocL1, editFocL2, editFocTT
	};
	m_vecCheckBoxes = { checkChopperCounterRot };


	if(m_pSettings)
	{
		QFont font;
		if(m_pSettings->contains("main/font_gen") && font.fromString(m_pSettings->value("main/font_gen", "").toString()))
			setFont(font);

		if(m_pSettings->contains("tof_calc/geo"))
			restoreGeometry(m_pSettings->value("tof_calc/geo").toByteArray());

		ReadLastConfig();
	}


	// -------------------------------------------------------------------------
	// chopper
	std::vector<QLineEdit*> editsChopper = { 
		editChopperL, editChopperR, editChopperOm, editChopperT };
	std::vector<QRadioButton*> radioChopper = {
		radioChopperL, radioChopperR, radioChopperOm, radioChopperT };

	for(QLineEdit* pEdit : editsChopper)
		QObject::connect(pEdit, SIGNAL(textEdited(const QString&)), this, SLOT(CalcChopper()));
	for(QRadioButton* pRadio : radioChopper)
	{
		QObject::connect(pRadio, SIGNAL(toggled(bool)), this, SLOT(EnableChopperEdits()));
		QObject::connect(pRadio, SIGNAL(toggled(bool)), this, SLOT(CalcChopper()));
	}
	QObject::connect(checkChopperCounterRot, SIGNAL(toggled(bool)), this, SLOT(CalcChopper()));

	EnableChopperEdits();
	CalcChopper();


	// -------------------------------------------------------------------------
	// divergence
	std::vector<QLineEdit*> editsDiv = { editDivL, editDivW, editDivAng };
	std::vector<QRadioButton*> radioDiv = { radioDivL, radioDivW, radioDivAng };

	for(QLineEdit* pEdit : editsDiv)
		QObject::connect(pEdit, SIGNAL(textEdited(const QString&)), this, SLOT(CalcDiv()));
	for(QRadioButton* pRadio : radioDiv)
	{
		QObject::connect(pRadio, SIGNAL(toggled(bool)), this, SLOT(EnableDivEdits()));
		QObject::connect(pRadio, SIGNAL(toggled(bool)), this, SLOT(CalcDiv()));
	}

	EnableDivEdits();
	CalcDiv();


	// -------------------------------------------------------------------------
	// selector
	std::vector<QLineEdit*> editsSel = { editSelLam, editSelTh, editSelL, editSelOm };
	std::vector<QRadioButton*> radioSel = { radioSelLam, radioSelTh, radioSelL, radioSelOm };

	for(QLineEdit* pEdit : editsSel)
		QObject::connect(pEdit, SIGNAL(textEdited(const QString&)), this, SLOT(CalcSel()));
	for(QRadioButton* pRadio : radioSel)
	{
		QObject::connect(pRadio, SIGNAL(toggled(bool)), this, SLOT(EnableSelEdits()));
		QObject::connect(pRadio, SIGNAL(toggled(bool)), this, SLOT(CalcSel()));
	}

	EnableSelEdits();
	CalcSel();


	// -------------------------------------------------------------------------
	// focus
	std::vector<QLineEdit*> editsFoc = { editFocL1, editFocL2, editFocTT };

	for(QLineEdit* pEdit : editsFoc)
		QObject::connect(pEdit, SIGNAL(textEdited(const QString&)), this, SLOT(CalcFoc()));

	CalcFoc();
}



// -----------------------------------------------------------------------------
// chopper

void TOFDlg::EnableChopperEdits()
{
	void (QLineEdit::*pFunc)(bool) = &QLineEdit::setReadOnly;

	(editChopperL->*pFunc)(0);
	(editChopperR->*pFunc)(0);
	(editChopperOm->*pFunc)(0);
	(editChopperT->*pFunc)(0);


	if(radioChopperL->isChecked())
		(editChopperL->*pFunc)(1);
	else if(radioChopperR->isChecked())
		(editChopperR->*pFunc)(1);
	else if(radioChopperOm->isChecked())
		(editChopperOm->*pFunc)(1);
	else if(radioChopperT->isChecked())
		(editChopperT->*pFunc)(1);
}

void TOFDlg::CalcChopper()
{
	std::string strChL = editChopperL->text().toStdString();
	std::string strChR = editChopperR->text().toStdString();
	std::string strChOm = editChopperOm->text().toStdString();
	std::string strChT = editChopperT->text().toStdString();

	tl::t_length_si<t_real> L = tl::str_to_var_parse<t_real>(strChL) * cm;
	tl::t_length_si<t_real> r = tl::str_to_var_parse<t_real>(strChR) * cm;
	tl::t_frequency_si<t_real> om = tl::str_to_var_parse<t_real>(strChOm) / sec;
	tl::t_time_si<t_real> dt = tl::str_to_var_parse<t_real>(strChT) * us;
	bool bCounterRot = checkChopperCounterRot->isChecked();

	if(radioChopperL->isChecked())
	{
		L = tl::burst_time_L(r, dt, om, bCounterRot, true);
		strChL = tl::var_to_str(t_real(L/cm), g_iPrec);
		editChopperL->setText(strChL.c_str());
	}
	else if(radioChopperR->isChecked())
	{
		r = tl::burst_time_r(dt, L, om, bCounterRot, true);
		strChR = tl::var_to_str(t_real(r/cm), g_iPrec);
		editChopperR->setText(strChR.c_str());
	}
	else if(radioChopperOm->isChecked())
	{
		om = tl::burst_time_om(r, L, dt, bCounterRot, true);
		strChOm = tl::var_to_str(t_real(om*sec), g_iPrec);
		editChopperOm->setText(strChOm.c_str());
	}
	else if(radioChopperT->isChecked())
	{
		dt = tl::burst_time(r, L, om, bCounterRot, true);
		strChT = tl::var_to_str(t_real(dt/us), g_iPrec);
		editChopperT->setText(strChT.c_str());
	}
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// div

void TOFDlg::EnableDivEdits()
{
	void (QLineEdit::*pFunc)(bool) = &QLineEdit::setReadOnly;

	(editDivL->*pFunc)(0);
	(editDivW->*pFunc)(0);
	(editDivAng->*pFunc)(0);

	if(radioDivL->isChecked())
		(editDivL->*pFunc)(1);
	else if(radioDivW->isChecked())
		(editDivW->*pFunc)(1);
	else if(radioDivAng->isChecked())
		(editDivAng->*pFunc)(1);
}

void TOFDlg::CalcDiv()
{
	std::string strDivL = editDivL->text().toStdString();
	std::string strDivW = editDivW->text().toStdString();
	std::string strDivAng = editDivAng->text().toStdString();

	tl::t_length_si<t_real> L = tl::str_to_var_parse<t_real>(strDivL) * cm;
	tl::t_length_si<t_real> w = tl::str_to_var_parse<t_real>(strDivW) * cm;
	tl::t_angle_si<t_real> ang = tl::str_to_var_parse<t_real>(strDivAng) * deg;

	if(radioDivL->isChecked())
	{
		L = tl::colli_div_L(ang, w, true);
		strDivL = tl::var_to_str(t_real(L/cm), g_iPrec);
		editDivL->setText(strDivL.c_str());
	}
	else if(radioDivW->isChecked())
	{
		w = tl::colli_div_w(L, ang, true);
		strDivW = tl::var_to_str(t_real(w/cm), g_iPrec);
		editDivW->setText(strDivW.c_str());
	}
	else if(radioDivAng->isChecked())
	{
		ang = tl::colli_div(L, w, true);
		strDivAng = tl::var_to_str(t_real(ang/deg), g_iPrec);
		editDivAng->setText(strDivAng.c_str());
	}
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// sel

void TOFDlg::EnableSelEdits()
{
	void (QLineEdit::*pFunc)(bool) = &QLineEdit::setReadOnly;

	(editSelLam->*pFunc)(0);
	(editSelTh->*pFunc)(0);
	(editSelL->*pFunc)(0);
	(editSelOm->*pFunc)(0);

	if(radioSelLam->isChecked())
		(editSelLam->*pFunc)(1);
	else if(radioSelTh->isChecked())
		(editSelTh->*pFunc)(1);
	else if(radioSelL->isChecked())
		(editSelL->*pFunc)(1);
	else if(radioSelOm->isChecked())
		(editSelOm->*pFunc)(1);
}

void TOFDlg::CalcSel()
{
	std::string strSelLam = editSelLam->text().toStdString();
	std::string strSelTh = editSelTh->text().toStdString();
	std::string strSelL = editSelL->text().toStdString();
	std::string strSelOm = editSelOm->text().toStdString();

	tl::t_length_si<t_real> lam = tl::str_to_var_parse<t_real>(strSelLam) * angs;
	tl::t_angle_si<t_real> th = tl::str_to_var_parse<t_real>(strSelTh) * deg;
	tl::t_length_si<t_real> L = tl::str_to_var_parse<t_real>(strSelL) * cm;
	tl::t_frequency_si<t_real> om = tl::str_to_var_parse<t_real>(strSelOm) / sec;

	if(radioSelLam->isChecked())
	{
		lam = tl::vsel_lam(th, om, L);
		strSelLam = tl::var_to_str(t_real(lam/angs), g_iPrec);
		editSelLam->setText(strSelLam.c_str());
	}
	else if(radioSelTh->isChecked())
	{
		th = tl::vsel_twist(lam, om, L);
		strSelTh = tl::var_to_str(t_real(th/deg), g_iPrec);
		editSelTh->setText(strSelTh.c_str());
	}
	else if(radioSelL->isChecked())
	{
		L = tl::vsel_len(lam, om, th);
		strSelL = tl::var_to_str(t_real(L/cm), g_iPrec);
		editSelL->setText(strSelL.c_str());
	}
	else if(radioSelOm->isChecked())
	{
		om = tl::vsel_freq(lam, L, th);
		strSelOm = tl::var_to_str(t_real(om*sec), g_iPrec);
		editSelOm->setText(strSelOm.c_str());
	}
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// foc

void TOFDlg::CalcFoc()
{
	std::string strL1 = editFocL1->text().toStdString();
	std::string strL2 = editFocL2->text().toStdString();
	std::string strTT = editFocTT->text().toStdString();

	tl::t_length_si<t_real> L1 = tl::str_to_var_parse<t_real>(strL1) * meter;
	tl::t_length_si<t_real> L2 = tl::str_to_var_parse<t_real>(strL2) * meter;
	tl::t_angle_si<t_real> tt = tl::str_to_var_parse<t_real>(strTT) * deg;

	tl::t_length_si<t_real> curvv = tl::foc_curv(L1, L2, tt, true);
	tl::t_length_si<t_real> curvh = tl::foc_curv(L1, L2, tt, false);

	std::string strCurvV = tl::var_to_str(t_real(curvv/meter), g_iPrec);
	std::string strCurvH = tl::var_to_str(t_real(curvh/meter), g_iPrec);

	editFocCurvV->setText(strCurvV.c_str());
	editFocCurvH->setText(strCurvH.c_str());
}
// -----------------------------------------------------------------------------



void TOFDlg::ReadLastConfig()
{
	if(!m_pSettings)
		return;

	for(std::size_t iEditBox=0; iEditBox<m_vecEditBoxes.size(); ++iEditBox)
	{
		if(!m_pSettings->contains(m_vecEditNames[iEditBox].c_str()))
			continue;
		QString strEditVal = m_pSettings->value(m_vecEditNames[iEditBox].c_str()).value<QString>();
		m_vecEditBoxes[iEditBox]->setText(strEditVal);
	}

	for(std::size_t iCheckBox=0; iCheckBox<m_vecCheckBoxes.size(); ++iCheckBox)
	{
		if(!m_pSettings->contains(m_vecCheckNames[iCheckBox].c_str()))
			continue;
		m_vecCheckBoxes[iCheckBox]->setChecked(m_pSettings->value(m_vecCheckNames[iCheckBox].c_str()).value<bool>());
	}

	for(std::size_t iRadio=0; iRadio<m_vecRadioBoxes.size(); ++iRadio)
	{
		if(!m_pSettings->contains(m_vecRadioNames[iRadio].c_str()))
			continue;

		bool bChecked = m_pSettings->value(m_vecRadioNames[iRadio].c_str()).value<bool>();
		m_vecRadioBoxes[iRadio]->setChecked(bChecked);
	}
}


void TOFDlg::WriteLastConfig()
{
	if(!m_pSettings)
		return;

	for(std::size_t iEditBox=0; iEditBox<m_vecEditBoxes.size(); ++iEditBox)
		m_pSettings->setValue(m_vecEditNames[iEditBox].c_str(), m_vecEditBoxes[iEditBox]->text()/*.toDouble()*/);
	for(std::size_t iRadio=0; iRadio<m_vecRadioBoxes.size(); ++iRadio)
		m_pSettings->setValue(m_vecRadioNames[iRadio].c_str(), m_vecRadioBoxes[iRadio]->isChecked());
	for(std::size_t iCheck=0; iCheck<m_vecCheckBoxes.size(); ++iCheck)
		m_pSettings->setValue(m_vecCheckNames[iCheck].c_str(), m_vecCheckBoxes[iCheck]->isChecked());
}


// -----------------------------------------------------------------------------


void TOFDlg::accept()
{
	if(m_pSettings)
	{
		WriteLastConfig();
		m_pSettings->setValue("tof_calc/geo", saveGeometry());
	}

	QDialog::accept();
}

void TOFDlg::showEvent(QShowEvent *pEvt)
{
	QDialog::showEvent(pEvt);
}


#include "TOFDlg.moc"
