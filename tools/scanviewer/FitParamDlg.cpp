/**
 * Fit Parameters Dialog
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date sep-2016
 * @license GPLv2
 */

#include "FitParamDlg.h"

FitParamDlg::FitParamDlg(QWidget* pParent, QSettings *pSett)
	: QDialog(pParent), m_pSettings(pSett)
{
	this->setupUi(this);

	if(m_pSettings)
	{
		QFont font;
		if(m_pSettings->contains("main/font_gen") && font.fromString(m_pSettings->value("main/font_gen", "").toString()))
			setFont(font);

		if(m_pSettings->contains("fit_params/geo"))
			restoreGeometry(m_pSettings->value("fit_params/geo").toByteArray());
	}
}

FitParamDlg::~FitParamDlg()
{}


void FitParamDlg::SetBold(QLabel* pLab, bool bBold)
{
	QFont fontLabel = pLab->font();
	fontLabel.setBold(bBold);
	pLab->setFont(fontLabel);
}

void FitParamDlg::UnsetAllBold()
{
	for(QLabel* pLab : {labelAmp, labelSig, labelHWHM, labelX0, labelOffs, labelSlope, labelFreq, labelPhase})
		SetBold(pLab, 0);
}


void FitParamDlg::accept()
{
	if(m_pSettings)
		m_pSettings->setValue("fit_params/geo", saveGeometry());

	QDialog::accept();
}


#include "FitParamDlg.moc"
