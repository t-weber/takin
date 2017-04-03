/**
 * Space Group List Dialog
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date oct-2015
 * @license GPLv2
 */

#ifndef __TAZ_SGLISTDLG_H__
#define __TAZ_SGLISTDLG_H__

#include <QDialog>
#include <QSettings>

#include "ui/ui_sglist.h"


class SgListDlg : public QDialog, Ui::SgListDlg
{ Q_OBJECT
protected:
	QSettings m_settings;

protected:
	void SetupSpacegroups();
	virtual void closeEvent(QCloseEvent* pEvt) override;

protected slots:
	void UpdateSG();
	void SGSelected(QListWidgetItem *pItem, QListWidgetItem *pItemPrev);
	void RecalcBragg();
	void SearchSG(const QString& qstr);

	void CalcTrafo();

public:
	SgListDlg(QWidget* pParent = nullptr);
	virtual ~SgListDlg();
};


#endif
