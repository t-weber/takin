/**
 * TOF Calculations
 * @author Tobias Weber
 * @date feb-2017
 * @license GPLv2
 */

#include "TOFDlg.h"
#include <boost/units/io.hpp>

#include "tlibs/string/string.h"
#include "tlibs/math/math.h"
#include "tlibs/math/neutrons.h"

#include <sstream>


using t_real = t_real_glob;
namespace co = boost::units::si::constants::codata;
static const tl::t_length_si<t_real> angs = tl::get_one_angstrom<t_real>();
static const tl::t_length_si<t_real> meter = tl::get_one_meter<t_real>();
static const tl::t_length_si<t_real> cm = meter * t_real(1e-2);
static const tl::t_time_si<t_real> sec = tl::get_one_second<t_real>();
static const tl::t_time_si<t_real> us = sec * t_real(1e-6);


TOFDlg::TOFDlg(QWidget* pParent, QSettings *pSett)
	: QDialog(pParent), m_pSettings(pSett)
{
	setupUi(this);

	if(m_pSettings)
	{
		QFont font;
		if(m_pSettings->contains("main/font_gen") && font.fromString(m_pSettings->value("main/font_gen", "").toString()))
			setFont(font);

		if(m_pSettings->contains("tof_calc/geo"))
			restoreGeometry(m_pSettings->value("tof_calc/geo").toByteArray());
	}


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
}


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
		L = tl::burst_time_L(r, dt, om, bCounterRot);
		strChL = tl::var_to_str(t_real(L/cm), g_iPrec);
		editChopperL->setText(strChL.c_str());
	}
	else if(radioChopperR->isChecked())
	{
		r = tl::burst_time_r(dt, L, om, bCounterRot);
		strChR = tl::var_to_str(t_real(r/cm), g_iPrec);
		editChopperR->setText(strChR.c_str());
	}
	else if(radioChopperOm->isChecked())
	{
		om = tl::burst_time_om(r, L, dt, bCounterRot);
		strChOm = tl::var_to_str(t_real(om*sec), g_iPrec);
		editChopperOm->setText(strChOm.c_str());
	}
	else if(radioChopperT->isChecked())
	{
		dt = tl::burst_time(r, L, om, bCounterRot);
		strChT = tl::var_to_str(t_real(dt/us), g_iPrec);
		editChopperT->setText(strChT.c_str());
	}
}


void TOFDlg::accept()
{
	if(m_pSettings)
		m_pSettings->setValue("tof_calc/geo", saveGeometry());

	QDialog::accept();
}

void TOFDlg::showEvent(QShowEvent *pEvt)
{
	QDialog::showEvent(pEvt);
}


#include "TOFDlg.moc"
