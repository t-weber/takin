/**
 * Scan Monitor
 * @author Tobias Weber
 * @date jul-2016
 * @license GPLv2
 */

#include "ScanMonDlg.h"
#include "tlibs/string/string.h"

using t_real = t_real_glob;


ScanMonDlg::ScanMonDlg(QWidget* pParent, QSettings *pSett)
	: QDialog(pParent), m_pSettings(pSett)
{
	setupUi(this);
	if(m_pSettings)
	{
		QFont font;
		if(m_pSettings->contains("main/font_gen") && font.fromString(m_pSettings->value("main/font_gen", "").toString()))
			setFont(font);

		if(m_pSettings->contains("scanmon/geo"))
			restoreGeometry(m_pSettings->value("scanmon/geo").toByteArray());
	}

	progress->setRange(0, 1000);
}

void ScanMonDlg::accept()
{
	if(m_pSettings)
		m_pSettings->setValue("scanmon/geo", saveGeometry());

	QDialog::accept();
}


static t_real get_value(const std::string& strVal)
{
	t_real val(0);

	std::size_t iBeg = strVal.find_first_of("0123456789e.+-");
	if(iBeg == std::string::npos)
		return val;
	std::string strVal2 = strVal.substr(iBeg);

	val = tl::str_to_var<t_real>(strVal2);
	return val;
}

void ScanMonDlg::UpdateValue(const std::string& strKey, const CacheVal& val)
{
	switch(val.ty)
	{
		case CacheValType::TIMER:
			m_dTimer = get_value(val.strVal);
			break;
		case CacheValType::PRESET:
			m_dPreset = get_value(val.strVal);
			break;
		case CacheValType::COUNTER:
			m_dCounter = get_value(val.strVal);
			break;
		default:
			return;
	}


	t_real dProgress = 0.;
	t_real dExpCtr = 0.;

	if(!tl::float_equal(m_dPreset, t_real(0.)))
	{
		dProgress = m_dTimer / m_dPreset;
		if(!tl::float_equal(dProgress, t_real(0.)))
			dExpCtr = m_dCounter / dProgress;
	}

	if(dProgress < t_real(0.)) dProgress = t_real(0.);
	else if(dProgress > t_real(1.)) dProgress = t_real(1.);

	progress->setValue(dProgress * t_real(1000));


	std::ostringstream ostrCts, ostrExp, ostrProg;
	ostrCts.precision(g_iPrecGfx);
	ostrExp.precision(g_iPrecGfx);
	ostrProg.precision(g_iPrecGfx);

	ostrCts << std::fixed << unsigned(m_dCounter) << " +- " << std::sqrt(m_dCounter);
	ostrExp << std::fixed << unsigned(std::round(dExpCtr)) << " (" << dExpCtr << ")";
	ostrProg << std::fixed << m_dTimer << " of " << m_dPreset;

	editCts->setText(ostrCts.str().c_str());
	editExpected->setText(ostrExp.str().c_str());
	editTime->setText(ostrProg.str().c_str());
}


#include "ScanMonDlg.moc"
