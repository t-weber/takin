/**
 * monte carlo convolution tool
 * @author tweber
 * @date aug-2015
 * @license GPLv2
 */

#ifndef __MCONVO_GUI_H__
#define __MCONVO_GUI_H__

#include <QDialog>
#include <QSettings>
#include <thread>
#include <atomic>
#include <memory>

#include "libs/qthelper.h"
#include "sqwfactory.h"
#include "tools/res/defs.h"
#include "tools/convofit/scan.h"

#include "SqwParamDlg.h"
#include "ui/ui_monteconvo.h"
#include "dialogs/FavDlg.h"


class ConvoDlg : public QDialog, Ui::ConvoDlg
{ Q_OBJECT
protected:
	std::thread *m_pth = nullptr;
	std::atomic<bool> m_atStop;

	QSettings *m_pSett = nullptr;
	SqwParamDlg *m_pSqwParamDlg = nullptr;
	FavDlg *m_pFavDlg = nullptr;

	std::shared_ptr<SqwBase> m_pSqw;
	std::vector<t_real_reso> m_vecQ, m_vecS, m_vecScaledS;
	std::unique_ptr<QwtPlotWrapper> m_plotwrap;

	bool m_bUseScan = 0;
	Scan m_scan;

protected:
	void LoadSettings();
	virtual void showEvent(QShowEvent *pEvt) override;

protected slots:
	void showSqwParamDlg();

	void browseCrysFiles();
	void browseResoFiles();
	void browseSqwFiles();
	void browseScanFiles();

	void SqwModelChanged(int);
	void createSqwModel(const QString& qstrFile);
	void SqwParamsChanged(const std::vector<SqwBase::t_var>&);

	void scanFileChanged(const QString& qstrFile);
	void scaleChanged();

	void SaveResult();

	void Start();
	void Stop();

	void ChangeHK();
	void ChangeHL();
	void ChangeKL();

	void ShowFavourites();
	void UpdateCurFavPos();
	void ChangePos(const struct FavHklPos& pos);

	void ButtonBoxClicked(QAbstractButton *pBtn);

public:
	ConvoDlg(QWidget* pParent=0, QSettings* pSett=0);
	virtual ~ConvoDlg();

signals:
	void SqwLoaded(const std::vector<SqwBase::t_var>&);
};

#endif
