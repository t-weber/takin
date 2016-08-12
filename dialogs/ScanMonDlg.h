/**
 * Scan Monitor
 * @author Tobias Weber
 * @date Jul-2016
 * @license GPLv2
 */

#ifndef __SCANMON_DLG_H__
#define __SCANMON_DLG_H__

#include <QDialog>
#include <QSettings>
#include "ui/ui_scanmon.h"

#include "libs/globals.h"
#include "NetCacheDlg.h"


class ScanMonDlg : public QDialog, Ui::ScanMonDlg
{ Q_OBJECT
	protected:
		t_real_glob m_dTimer = 0,
			m_dPreset = 0,
			m_dCounter = 0;

	protected:
		QSettings *m_pSettings = 0;

		virtual void accept() override;

	public:
		ScanMonDlg(QWidget* pParent=0, QSettings *pSett=0);
		virtual ~ScanMonDlg() = default;

	public slots:
		void UpdateValue(const std::string& strKey, const CacheVal& val);
};

#endif
