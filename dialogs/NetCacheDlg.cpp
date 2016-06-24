/*
 * Cache dialog
 * @author Tobias Weber
 * @date 21-oct-2014
 * @license GPLv2
 */

#include "NetCacheDlg.h"
#include "tlibs/string/string.h"
#include "tlibs/helper/flags.h"
#include "tlibs/log/log.h"
#include "libs/qthelper.h"
#include <chrono>
#include <iostream>

using t_real = t_real_glob;

enum
{
	ITEM_KEY = 0,
	ITEM_VALUE = 1,
	ITEM_TIMESTAMP = 2,
	ITEM_AGE = 3
};

NetCacheDlg::NetCacheDlg(QWidget* pParent, QSettings* pSett)
	: QDialog(pParent), m_pSettings(pSett)
{
	this->setupUi(this);
	if(m_pSettings)
	{
		QFont font;
		if(m_pSettings->contains("main/font_gen") && font.fromString(m_pSettings->value("main/font_gen", "").toString()))
			setFont(font);
	}

	tableCache->setColumnCount(4);
	tableCache->setRowCount(0);
	tableCache->setColumnWidth(ITEM_KEY, 200);
	tableCache->setColumnWidth(ITEM_VALUE, 200);
	tableCache->setColumnWidth(ITEM_TIMESTAMP, 140);
	tableCache->setColumnWidth(ITEM_AGE, 140);
	tableCache->verticalHeader()->setDefaultSectionSize(tableCache->verticalHeader()->minimumSectionSize()+2);

	tableCache->setHorizontalHeaderItem(ITEM_KEY, new QTableWidgetItem("Name"));
	tableCache->setHorizontalHeaderItem(ITEM_VALUE, new QTableWidgetItem("Value"));
	tableCache->setHorizontalHeaderItem(ITEM_TIMESTAMP, new QTableWidgetItem("Time Stamp"));
	tableCache->setHorizontalHeaderItem(ITEM_AGE, new QTableWidgetItem("Age"));

#if QT_VER >= 5
	QObject::connect(&m_timer, &QTimer::timeout, this, &NetCacheDlg::UpdateTimer);
#else
	QObject::connect(&m_timer, SIGNAL(timeout()), this, SLOT(UpdateTimer()));
#endif
	m_timer.start(s_iTimer);


	if(m_pSettings && m_pSettings->contains("net_cache/geo"))
		restoreGeometry(m_pSettings->value("net_cache/geo").toByteArray());
}

NetCacheDlg::~NetCacheDlg()
{}

void NetCacheDlg::UpdateTimer()
{
	UpdateAge(-1);
}

void NetCacheDlg::hideEvent(QHideEvent *pEvt)
{
	m_timer.stop();
}

void NetCacheDlg::showEvent(QShowEvent *pEvt)
{
	m_timer.start(s_iTimer);
}

void NetCacheDlg::accept()
{
	if(m_pSettings)
		m_pSettings->setValue("net_cache/geo", saveGeometry());

	QDialog::accept();
}

void NetCacheDlg::UpdateValue(const std::string& strKey, const CacheVal& val)
{
	const bool bSortTable = tableCache->isSortingEnabled();
	tableCache->setSortingEnabled(0);

	int iRow = 0;
	QTableWidgetItem *pItem = 0;
	QString qstrKey = strKey.c_str();
	QString qstrVal = val.strVal.c_str();
	//tl::log_debug("updating net cache: ", strKey, ", ", val.strVal);

	for(iRow=0; iRow < tableCache->rowCount(); ++iRow)
	{
		if(tableCache->item(iRow, 0)->text() == qstrKey)
		{
			pItem = tableCache->item(iRow, 0);
			break;
		}
	}

	if(pItem) 	// update items
	{
		tableCache->item(iRow, 1)->setText(qstrVal);
		dynamic_cast<QTableWidgetItemWrapper<t_real>*>(tableCache->item(iRow, ITEM_TIMESTAMP))->
			SetValue(val.dTimestamp);
		dynamic_cast<QTableWidgetItemWrapper<t_real>*>(tableCache->item(iRow, ITEM_AGE))->
			SetValueKeepText(val.dTimestamp);
	}
	else		// insert new items
	{
		iRow = tableCache->rowCount();
		tableCache->setRowCount(iRow+1);
		tableCache->setItem(iRow, ITEM_KEY, new QTableWidgetItem(qstrKey));
		tableCache->setItem(iRow, ITEM_VALUE, new QTableWidgetItem(qstrVal));
		tableCache->setItem(iRow, ITEM_TIMESTAMP, new QTableWidgetItemWrapper<t_real>(val.dTimestamp));
		tableCache->setItem(iRow, ITEM_AGE, new QTableWidgetItemWrapper<t_real>(val.dTimestamp, ""));
	}

	UpdateAge(iRow);
	tableCache->setSortingEnabled(bSortTable);
}

void NetCacheDlg::UpdateAll(const t_mapCacheVal& map)
{
	for(const t_mapCacheVal::value_type& pair : map)
	{
		const std::string& strKey = pair.first;
		const CacheVal& val = pair.second;

		UpdateValue(strKey, val);
	}
}

void NetCacheDlg::UpdateAge(int iRow)
{
	const bool bSortTable = tableCache->isSortingEnabled();
	tableCache->setSortingEnabled(0);

	// update all
	if(iRow<0)
	{
		for(int iCurRow=0; iCurRow<tableCache->rowCount(); ++iCurRow)
			UpdateAge(iCurRow);

		return;
	}

	QTableWidgetItemWrapper<t_real>* pItemTimestamp =
		dynamic_cast<QTableWidgetItemWrapper<t_real>*>(tableCache->item(iRow, ITEM_TIMESTAMP));
	QTableWidgetItem *pItem = tableCache->item(iRow, ITEM_AGE);
	if(!pItemTimestamp || !pItem) return;

	t_real dTimestamp = pItemTimestamp->GetValue();
	t_real dNow = std::chrono::system_clock::now().time_since_epoch().count();
	dNow *= t_real(std::chrono::system_clock::period::num) / t_real(std::chrono::system_clock::period::den);
	t_real dAgeS = dNow - dTimestamp;
	//std::cout << strKey << " = " << val.strVal << " (age: " << dAge << "s)" << std::endl;

	int iAgeS = int(dAgeS);
	int iAgeM = 0;
	int iAgeH = 0;
	int iAgeD = 0;

	if(iAgeS > 60)
	{
		iAgeM = iAgeS / 60;
		iAgeS = iAgeS % 60;
	}
	if(iAgeM > 60)
	{
		iAgeH = iAgeM / 60;
		iAgeM = iAgeM  % 60;
	}
	if(iAgeH > 24)
	{
		iAgeD = iAgeH / 24;
		iAgeH = iAgeH  % 24;
	}

	bool bHadPrev = 0;
	std::string strAge;
	if(iAgeD || bHadPrev) { strAge += tl::var_to_str(iAgeD) + "d "; bHadPrev = 1; }
	if(iAgeH || bHadPrev) { strAge += tl::var_to_str(iAgeH) + "h "; bHadPrev = 1; }
	if(iAgeM || bHadPrev) { strAge += tl::var_to_str(iAgeM) + "m "; bHadPrev = 1; }
	/*if(iAgeS || bHadPrev)*/ { strAge += tl::var_to_str(iAgeS) + "s "; bHadPrev = 1; }
	pItem->setText(strAge.c_str());

	tableCache->setSortingEnabled(bSortTable);
}

void NetCacheDlg::ClearAll()
{
	tableCache->clearContents();
	tableCache->setRowCount(0);
}

#include "NetCacheDlg.moc"
