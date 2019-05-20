/**
 * monte carlo convolution tool
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date aug-2015
 * @license GPLv2
 */

#ifndef __MCONVO_GUI_H__
#define __MCONVO_GUI_H__

#include <QDialog>
#include <QSettings>
#include <QMenuBar>
#include <QMenu>
#include <QSignalMapper>

#include <thread>
#include <atomic>
#include <memory>

#include "ui/ui_monteconvo.h"

#include "libs/qt/qthelper.h"
#include "libs/qt/qwthelper.h"
#include "tlibs/file/prop.h"

#include "sqwfactory.h"

#include "tools/res/defs.h"
#include "tools/convofit/scan.h"

#include "dialogs/FavDlg.h"
#include "SqwParamDlg.h"
#include "TASReso.h"


#define CONVO_MAX_CURVES		32
#define CONVO_DISP_CURVE_START 		3


class ConvoDlg : public QDialog, Ui::ConvoDlg
{ Q_OBJECT
protected:
	std::thread *m_pth = nullptr;
	std::atomic<bool> m_atStop;

	QSettings *m_pSett = nullptr;
	QMenuBar *m_pMenuBar = nullptr;
	SqwParamDlg *m_pSqwParamDlg = nullptr;
	FavDlg *m_pFavDlg = nullptr;

	bool m_bAllowSqwReinit = 1;
	std::shared_ptr<SqwBase> m_pSqw;
	std::vector<t_real_reso> m_vecQ, m_vecS, m_vecScaledS;
	std::vector<std::vector<t_real_reso>> m_vecvecQ, m_vecvecE, m_vecvecW;
	std::unique_ptr<QwtPlotWrapper> m_plotwrap, m_plotwrap2d;

	bool m_bUseScan = 0;
	Scan m_scan;

	static const std::string s_strTitle;
	std::string m_strLastFile;

protected:
	std::vector<QDoubleSpinBox*> m_vecSpinBoxes;
	std::vector<QSpinBox*> m_vecIntSpinBoxes;
	std::vector<QLineEdit*> m_vecEditBoxes;
	std::vector<QPlainTextEdit*> m_vecTextBoxes;
	std::vector<QComboBox*> m_vecComboBoxes;
	std::vector<QCheckBox*> m_vecCheckBoxes;

	std::vector<std::string> m_vecSpinNames, m_vecIntSpinNames,
		m_vecEditNames, m_vecTextNames,
		m_vecComboNames, m_vecCheckNames;

	QAction *m_pLiveResults = nullptr, *m_pLivePlots = nullptr;

	// recent files
	QMenu *m_pMenuRecent = nullptr;
	QSignalMapper *m_pMapperRecent = nullptr;

protected:
	void LoadSettings();
	virtual void showEvent(QShowEvent *pEvt) override;

	ResoFocus GetFocus() const;

	std::tuple<bool, int, std::string, std::vector<std::vector<t_real_reso>>> GetScanAxis(bool bIncludeE=true);
	void ClearPlot1D();
	void Start1D();
	void Start2D();

public:
	void Load(tl::Prop<std::string>& xml, const std::string& strXmlRoot);
	void Save(std::map<std::string, std::string>& mapConf, const std::string& strXmlRoot);

protected slots:
	void showSqwParamDlg();

	void browseCrysFiles();
	void browseResoFiles();
	void browseSqwFiles();
	void browseScanFiles();

	void SqwModelChanged(int);
	void createSqwModel(const QString& qstrFile);
	void SqwParamsChanged(const std::vector<SqwBase::t_var>&,
		const std::vector<SqwBase::t_var_fit>*);

	void scanFileChanged(const QString& qstrFile);
	void scanCheckToggled(bool);
	void scaleChanged();

	void SaveResult();

	void Start();		// convolution
	void StartFit();	// convolution fit
	void StartDisp();	// plot dispersion
	void Stop();		// stop convo

	void ChangeHK();
	void ChangeHL();
	void ChangeKL();

	void ShowFavourites();
	void UpdateCurFavPos();
	void ChangePos(const struct FavHklPos& pos);

	virtual void accept() override;

	void New();
	void Load(const QString&);
	void Load();
	void Save();
	void SaveAs();
	void SaveConvofit();

	void ShowAboutDlg();

public:
	ConvoDlg(QWidget* pParent=0, QSettings* pSett=0);
	virtual ~ConvoDlg();

signals:
	void SqwLoaded(const std::vector<SqwBase::t_var>&, const std::vector<SqwBase::t_var_fit>*);
};

#endif
