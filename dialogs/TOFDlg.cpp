/**
 * TOF Calculations
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
		"tof_calc/calc_div_l", "tof_calc/calc_div_w", "tof_calc/calc_div_ang" };
	m_vecEditNames = { "tof_calc/chopper_l", "tof_calc/chopper_r",
		"tof_calc/chopper_om", "tof_calc/chopper_t",
		"tof_calc/div_l", "tof_calc/div_w", "tof_calc/div_ang" };
	m_vecCheckNames = { "tof_calc/counter_rot" };

	m_vecRadioBoxes = { radioChopperL, radioChopperR, radioChopperOm, radioChopperT,
		radioDivL, radioDivW, radioDivAng };
	m_vecEditBoxes = { editChopperL, editChopperR, editChopperOm, editChopperT, 
		editDivL, editDivW, editDivAng };
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
