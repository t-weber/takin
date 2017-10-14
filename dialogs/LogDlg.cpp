/**
 * Log Dialog
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 14-oct-2017
 * @license GPLv2
 */

#include <QScrollBar>
#include "LogDlg.h"
#include "tlibs/file/file.h"
#include <fstream>


LogDlg::LogDlg(QWidget* pParent, QSettings *pSett, const std::string& strLogFile)
	: QDialog(pParent), m_pSettings(pSett)
{
	this->setupUi(this);

	if(m_pSettings)
	{
		QFont font;
		if(m_pSettings->contains("main/font_gen") && font.fromString(m_pSettings->value("main/font_gen", "").toString()))
			setFont(font);

		if(m_pSettings->contains("log/geo"))
			restoreGeometry(m_pSettings->value("log/geo").toByteArray());
	}

	if(tl::file_exists(strLogFile.c_str()))
	{
		m_pFileWatcher.reset(new QFileSystemWatcher(this));
		m_pFileWatcher->addPath(strLogFile.c_str());

		QObject::connect(m_pFileWatcher.get(), SIGNAL(fileChanged(const QString&)),
			this, SLOT(LogFileChanged(const QString&)));

		LogFileChanged(strLogFile.c_str());
	}
	else
	{
		editOut->setPlainText("Error: No log data found!");
	}
}


/**
 * log file changed
 */
void LogDlg::LogFileChanged(const QString& _strLogFile)
{
	std::string strLogFile = _strLogFile.toStdString();
	std::ifstream ifstr(strLogFile);
	if(!ifstr)
		return;

	std::size_t iSize = std::size_t(tl::get_file_size(ifstr));
	std::unique_ptr<char[]> pcLog(new char[iSize+1]);

	ifstr.read((char*)pcLog.get(), iSize);
	pcLog[iSize] = 0;
	editOut->setPlainText(pcLog.get());

	// scroll to end
	QScrollBar *pSB = editOut->verticalScrollBar();
	pSB->setValue(pSB->maximum());
}


void LogDlg::accept()
{
	if(m_pSettings)
		m_pSettings->setValue("log/geo", saveGeometry());

	QDialog::accept();
}

#include "LogDlg.moc"
