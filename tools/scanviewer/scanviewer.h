/*
 * Scan viewer
 * @author tweber
 * @date mar-2015
 * @license GPLv2
 */

#ifndef __TAZ_SCANVIEWER_H__
#define __TAZ_SCANVIEWER_H__

#include <QDialog>
#include <QSettings>
#include <string>
#include <vector>
#include <memory>

#include "tlibs/file/loadinstr.h"
#include "libs/qthelper.h"
#include "libs/globals.h"
#include "ui/ui_scanviewer.h"


class ScanViewerDlg : public QDialog, Ui::ScanViewerDlg
{ Q_OBJECT
protected:
	QSettings m_settings;
	std::string m_strCurDir, m_strCurFile;
	std::string m_strSelectedKey;
	std::vector<std::string> m_vecExts;

	bool m_bDoUpdate = 0;
	tl::FileInstrBase<t_real_glob> *m_pInstr = nullptr;
	std::vector<t_real_glob> m_vecX, m_vecY;
	std::unique_ptr<QwtPlotWrapper> m_plotwrap;
	std::string m_strX, m_strY, m_strCmd;

protected:
	void ClearPlot();
	void PlotScan();
	void ShowProps();

	void GenerateForRoot();
	void GenerateForGnuplot();
	void GenerateForPython();
	void GenerateForHermelin();

	virtual void closeEvent(QCloseEvent* pEvt) override;

protected slots:
	void GenerateExternal(int iLang=0);

	void UpdateFileList();
	void FileSelected(QListWidgetItem *pItem, QListWidgetItem *pItemPrev);
	void PropSelected(QTableWidgetItem *pItem, QTableWidgetItem *pItemPrev);
	void SelectDir();
	void ChangedPath();

	void XAxisSelected(const QString&);
	void YAxisSelected(const QString&);

	//void openExternally();

public:
	ScanViewerDlg(QWidget* pParent = nullptr);
	virtual ~ScanViewerDlg();
};


#endif
