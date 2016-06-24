/*
 * Server Dialog
 * @author Tobias Weber
 * @date 27-aug-2014
 * @license GPLv2
 */

#ifndef __SRV_DLG_H__
#define __SRV_DLG_H__

#include <QDialog>
#include <QSettings>
#include "ui/ui_connection.h"

class SrvDlg : public QDialog, Ui::SrvDlg
{ Q_OBJECT
	protected:
		QSettings *m_pSettings = 0;

	public:
		SrvDlg(QWidget* pParent=0, QSettings *pSett=0);
		virtual ~SrvDlg();

	signals:
		void ConnectTo(int iSys, const QString& strHost, const QString& strPort,
			const QString& strUser, const QString& strPass);

	protected:
		virtual void accept() override;
};

#endif
