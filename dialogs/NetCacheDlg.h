/*
 * Cache dialog
 * @author Tobias Weber
 * @date 21-oct-2014
 * @license GPLv2
 */

#ifndef __NET_CACHE_DLG_H__
#define __NET_CACHE_DLG_H__

#include <map>
#include <QDialog>
#include <QSettings>
#include <QTimer>
#include "ui/ui_netcache.h"
#include "libs/globals.h"


struct CacheVal
{
	std::string strVal;
	t_real_glob dTimestamp;
};

typedef std::map<std::string, CacheVal> t_mapCacheVal;


class NetCacheDlg : public QDialog, Ui::NetCacheDlg
{ Q_OBJECT
protected:
	QSettings *m_pSettings = 0;

	static const int s_iTimer = 1000;
	QTimer m_timer;

protected:
	virtual void hideEvent(QHideEvent *pEvt) override;
	virtual void showEvent(QShowEvent *pEvt) override;
	virtual void accept() override;

	void UpdateAge(int iRow=-1);

public:
	NetCacheDlg(QWidget* pParent=0, QSettings* pSett=0);
	virtual ~NetCacheDlg();

protected slots:
	void UpdateTimer();

public slots:
	void ClearAll();
	void UpdateValue(const std::string& strKey, const CacheVal& val);
	void UpdateAll(const t_mapCacheVal& map);
};

#endif
