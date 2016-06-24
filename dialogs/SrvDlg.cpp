/*
 * Server Dialog
 * @author Tobias Weber
 * @date 27-aug-2014
 * @license GPLv2
 */

#include "SrvDlg.h"
#include "tlibs/string/string.h"

SrvDlg::SrvDlg(QWidget* pParent, QSettings *pSett)
		: QDialog(pParent), m_pSettings(pSett)
{
	this->setupUi(this);

	if(m_pSettings)
	{
		QFont font;
		if(m_pSettings->contains("main/font_gen") && font.fromString(m_pSettings->value("main/font_gen", "").toString()))
			setFont(font);


		bool bRememberPwd = 0;
		if(m_pSettings->contains("server/remember_pwd"))
		{
			bRememberPwd = m_pSettings->value("server/remember_pwd").toBool();
			checkRememberPwd->setChecked(bRememberPwd);
		}
		if(m_pSettings->contains("server/login"))
			editLogin->setText(m_pSettings->value("server/login").toString());
		if(bRememberPwd && m_pSettings->contains("server/pwd"))
			editPwd->setText(m_pSettings->value("server/pwd").toString());

		if(m_pSettings->contains("server/system"))
			comboSys->setCurrentIndex(m_pSettings->value("server/system").toInt());
		if(m_pSettings->contains("server/host"))
			editHost->setText(m_pSettings->value("server/host").toString());
		if(m_pSettings->contains("server/port"))
			editPort->setText(m_pSettings->value("server/port").toString());
	}
}

SrvDlg::~SrvDlg()
{}

void SrvDlg::accept()
{
	const int iSys = comboSys->currentIndex();
	const QString strHost = editHost->text();
	const QString strPort = editPort->text();
	const QString strLogin = editLogin->text();
	const QString strPwd = editPwd->text();

	if(m_pSettings)
	{
		m_pSettings->setValue("server/remember_pwd", checkRememberPwd->isChecked());
		m_pSettings->setValue("server/login", strLogin);
		if(checkRememberPwd->isChecked())
			m_pSettings->setValue("server/pwd", strPwd);
		else
			m_pSettings->setValue("server/pwd", "");

		m_pSettings->setValue("server/system", iSys);
		m_pSettings->setValue("server/host", strHost);
		m_pSettings->setValue("server/port", strPort);
	}

	emit ConnectTo(iSys, strHost, strPort, strLogin, strPwd);
	QDialog::accept();
}

#include "SrvDlg.moc"
