/**
 * Log Dialog
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 14-oct-2017
 * @license GPLv2
 */

#ifndef __LOG_DLG_H__
#define __LOG_DLG_H__

#include <QDialog>
#include <QSettings>
#include <QFileSystemWatcher>
#include "ui/ui_log.h"
#include <memory>


class LogDlg : public QDialog, Ui::LogDlg
{ Q_OBJECT
	protected:
		QSettings *m_pSettings = 0;
		std::unique_ptr<QFileSystemWatcher> m_pFileWatcher;

	public:
		LogDlg(QWidget* pParent=0, QSettings *pSett=0,
			const std::string& strLogFile="");
		virtual ~LogDlg() = default;

	protected:
		virtual void accept() override;

	protected slots:
		void LogFileChanged(const QString&);
};

#endif
