/*
 * About Dialog
 * @author Tobias Weber
 * @date nov-2015
 * @license GPLv2
 */

#ifndef __ABOUT_DLG_H__
#define __ABOUT_DLG_H__

#include <QDialog>
#include <QSettings>
#include "ui/ui_about.h"

class AboutDlg : public QDialog, Ui::AboutDlg
{ Q_OBJECT
	protected:
		QSettings *m_pSettings = 0;

		virtual void accept() override;

	public:
		AboutDlg(QWidget* pParent=0, QSettings *pSett=0);
		virtual ~AboutDlg() = default;
};

#endif
