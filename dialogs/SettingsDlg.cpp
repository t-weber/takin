/**
 * Settings
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 5-dec-2014
 * @license GPLv2
 */

#include "SettingsDlg.h"
#include "tlibs/string/string.h"
#include "tlibs/log/log.h"
#include "tlibs/file/file.h"
#ifndef NO_3D
	#include "tlibs/gfx/gl.h"
#endif
#include "libs/globals.h"
#include "libs/globals_qt.h"
#include "libs/qt/qthelper.h"

#include <QFileDialog>
#include <QFontDialog>
#include <QStyleFactory>

#include <iostream>
#include <limits>
#include <thread>

using t_real = t_real_glob;

// -----------------------------------------------------------------------------


SettingsDlg::SettingsDlg(QWidget* pParent, QSettings* pSett)
	: QDialog(pParent), m_pSettings(pSett)
{
	setupUi(this);

	g_fontGen.setStyleHint(QFont::SansSerif);
	g_fontGfx.setStyleHint(QFont::SansSerif, QFont::PreferAntialias);
	setFont(g_fontGen);

	// get possible GUI styles
	for(const QString& strStyle : QStyleFactory::keys())
		comboGUI->addItem(strStyle);

#if QT_VER >= 5
	connect(buttonBox, &QDialogButtonBox::clicked, this, &SettingsDlg::ButtonBoxClicked);
	connect(btnGLFont, &QAbstractButton::clicked, this, &SettingsDlg::SelectGLFont);
	connect(btnGfxFont, &QAbstractButton::clicked, this, &SettingsDlg::SelectGfxFont);
	connect(btnGenFont, &QAbstractButton::clicked, this, &SettingsDlg::SelectGenFont);
	connect(btnCif, &QAbstractButton::clicked, this, &SettingsDlg::SelectCifTool);
	connect(btnGpl, &QAbstractButton::clicked, this, &SettingsDlg::SelectGplTool);
#else
	connect(buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(ButtonBoxClicked(QAbstractButton*)));
	connect(btnGLFont, SIGNAL(clicked()), this, SLOT(SelectGLFont()));
	connect(btnGfxFont, SIGNAL(clicked()), this, SLOT(SelectGfxFont()));
	connect(btnGenFont, SIGNAL(clicked()), this, SLOT(SelectGenFont()));
	connect(btnCif, SIGNAL(clicked()), this, SLOT(SelectCifTool()));
	connect(btnGpl, SIGNAL(clicked()), this, SLOT(SelectGplTool()));
#endif

	m_vecEdits =
	{
		// nicos devices
		t_tupEdit("net/sample_name", "nicos/sample/samplename", editSampleName),
		t_tupEdit("net/lattice", "nicos/sample/lattice", editSampleLattice),
		t_tupEdit("net/angles", "nicos/sample/angles", editSampleAngles),
		t_tupEdit("net/orient1", "nicos/sample/orient1", editSampleOrient1),
		t_tupEdit("net/orient2", "nicos/sample/orient2", editSampleOrient2),
		t_tupEdit("net/spacegroup", "nicos/sample/spacegroup", editSampleSG),
		t_tupEdit("net/psi0", "nicos/sample/psi0", editSamplePsi0),

		t_tupEdit("net/stheta", "nicos/sth/value", editSampleTheta),
		t_tupEdit("net/s2theta", "nicos/stt/value", editSample2Theta),

		t_tupEdit("net/mtheta", "nicos/mth/value", editMonoTheta),
		t_tupEdit("net/m2theta", "nicos/mtt/value", editMono2Theta),
		t_tupEdit("net/mono_d", "nicos/mono/dvalue", editMonoD),

		t_tupEdit("net/atheta", "nicos/ath/value", editAnaTheta),
		t_tupEdit("net/a2theta", "nicos/att/value", editAna2Theta),
		t_tupEdit("net/ana_d", "nicos/ana/dvalue", editAnaD),

		t_tupEdit("net/timer", "nicos/timer/value", editCurTime),
		t_tupEdit("net/preset", "nicos/timer/preselection", editPreset),
		t_tupEdit("net/counter", "nicos/ctr1/value", editCounter),

		// sics devices
		t_tupEdit("net/lattice_a_6", "AS", edit6_AS),
		t_tupEdit("net/lattice_b_6", "BS", edit6_BS),
		t_tupEdit("net/lattice_c_6", "CS", edit6_CS),

		t_tupEdit("net/angle_a_6", "AA", edit6_AA),
		t_tupEdit("net/angle_b_6", "BB", edit6_BB),
		t_tupEdit("net/angle_c_6", "CC", edit6_CC),

		t_tupEdit("net/orient1_x_6", "AX", edit6_AX),
		t_tupEdit("net/orient1_y_6", "AY", edit6_AY),
		t_tupEdit("net/orient1_z_6", "AZ", edit6_AZ),

		t_tupEdit("net/orient2_x_6", "BX", edit6_BX),
		t_tupEdit("net/orient2_y_6", "BY", edit6_BY),
		t_tupEdit("net/orient2_z_6", "BZ", edit6_BZ),

		t_tupEdit("net/stheta_6", "A3", edit6_A3),
		t_tupEdit("net/s2theta_6", "A4", edit6_A4),

		t_tupEdit("net/mtheta_6", "A1", edit6_A1),
		t_tupEdit("net/m2theta_6", "A2", edit6_A2),
		t_tupEdit("net/mono_d_6", "DM", edit6_DM),

		t_tupEdit("net/atheta_6", "A5", edit6_A5),
		t_tupEdit("net/a2theta_6", "A6", edit6_A6),
		t_tupEdit("net/ana_d_6", "DA", edit6_DA),

		t_tupEdit("net/timer_6", "counter getmonitor 1", edit6_CurTime),
		t_tupEdit("net/preset_6", "counter getpreset", edit6_Preset),
		t_tupEdit("net/counter_6", "counter getcounts", edit6_Counter),
		t_tupEdit("net/xdat_6", "iscan getvardata", edit6_xDat),
		t_tupEdit("net/ydat_6", "iscan getcounts", edit6_yDat),

		// misc
		t_tupEdit("gl/font", "", editGLFont),
		t_tupEdit("main/font_gfx", g_fontGfx.toString().toStdString().c_str(), editGfxFont),
		t_tupEdit("main/font_gen", g_fontGen.toString().toStdString().c_str(), editGenFont),
	
		// external tools
		t_tupEdit("tools/cif2xml", "cif2xml", editCif),
		t_tupEdit("tools/gpl", "gnuplot", editGpl)
	};

	m_vecChecks =
	{
		t_tupCheck("main/dlg_previews", 1, checkPreview),
		t_tupCheck("main/threaded_gl", 0, checkThreadedGL),
		t_tupCheck("main/native_dialogs", 0, checkNativeDlg),
		t_tupCheck("net/flip_orient2", 1, checkFlipOrient2),
		t_tupCheck("net/sth_stt_corr", 0, checkSthSttCorr),
		t_tupCheck("main/ignore_xtal_restrictions", 0, checkIgnoreXtalRestrictions),
	};

	m_vecSpins =
	{
		t_tupSpin("main/prec", g_iPrec, spinPrecGen),
		t_tupSpin("main/prec_gfx", g_iPrecGfx, spinPrecGfx),
		t_tupSpin("main/points_gfx", GFX_NUM_POINTS, spinPtsGfx),
		t_tupSpin("main/max_neighbours", g_iMaxNN, spinMaxNN),
		t_tupSpin("main/max_peaks", 10, spinBragg),
		t_tupSpin("main/max_threads", g_iMaxThreads, spinThreads),
		t_tupSpin("gl/font_size", 24, spinGLFont),
		t_tupSpin("net/poll", 750, spinNetPoll),
	};

	m_vecCombos =
	{
		t_tupCombo("main/sfact_sq", 0, comboSFact),
		t_tupCombo("main/calc_3d_bz", 0, comboBZ),
		t_tupCombo("main/gui_style", 0, comboGUI),
	};

	spinPrecGen->setMaximum(std::numeric_limits<t_real>::max_digits10);
	spinPrecGfx->setMaximum(std::numeric_limits<t_real>::max_digits10);
	spinThreads->setMaximum(std::thread::hardware_concurrency());

	SetDefaults(0);


	if(m_pSettings && m_pSettings->contains("settings/geo"))
		restoreGeometry(m_pSettings->value("settings/geo").toByteArray());

	LoadSettings();
}

SettingsDlg::~SettingsDlg()
{}


void SettingsDlg::SetDefaults(bool bOverwrite)
{
	if(!m_pSettings) return;

	for(const t_tupEdit& tup : m_vecEdits)
	{
		const std::string& strKey = std::get<0>(tup);
		const std::string& strDef = std::get<1>(tup);

		bool bKeyExists = m_pSettings->contains(strKey.c_str());
		if(bKeyExists && !bOverwrite) continue;

		m_pSettings->setValue(strKey.c_str(), strDef.c_str());
	}

	for(const t_tupCheck& tup : m_vecChecks)
	{
		const std::string& strKey = std::get<0>(tup);
		const bool bDef = std::get<1>(tup);

		bool bKeyExists = m_pSettings->contains(strKey.c_str());
		if(bKeyExists && !bOverwrite) continue;

		m_pSettings->setValue(strKey.c_str(), bDef);
	}

	for(const t_tupSpin& tup : m_vecSpins)
	{
		const std::string& strKey = std::get<0>(tup);
		const int iDef = std::get<1>(tup);

		bool bKeyExists = m_pSettings->contains(strKey.c_str());
		if(bKeyExists && !bOverwrite) continue;

		m_pSettings->setValue(strKey.c_str(), iDef);
	}

	for(const t_tupCombo& tup : m_vecCombos)
	{
		const std::string& strKey = std::get<0>(tup);
		const int iDef = std::get<1>(tup);

		bool bKeyExists = m_pSettings->contains(strKey.c_str());
		if(bKeyExists && !bOverwrite) continue;

		m_pSettings->setValue(strKey.c_str(), iDef);
	}
}


void SettingsDlg::LoadSettings()
{
	if(!m_pSettings) return;

	for(const t_tupEdit& tup : m_vecEdits)
	{
		const std::string& strKey = std::get<0>(tup);
		const std::string& strDef = std::get<1>(tup);
		QLineEdit* pEdit = std::get<2>(tup);

		QString strVal = m_pSettings->value(strKey.c_str(), strDef.c_str()).toString();
		pEdit->setText(strVal);
	}

	for(const t_tupCheck& tup : m_vecChecks)
	{
		const std::string& strKey = std::get<0>(tup);
		bool bDef = std::get<1>(tup);
		QCheckBox* pCheck = std::get<2>(tup);

		bool bVal = m_pSettings->value(strKey.c_str(), bDef).toBool();
		pCheck->setChecked(bVal);
	}

	for(const t_tupSpin& tup : m_vecSpins)
	{
		const std::string& strKey = std::get<0>(tup);
		int iDef = std::get<1>(tup);
		QSpinBox* pSpin = std::get<2>(tup);

		int iVal = m_pSettings->value(strKey.c_str(), iDef).toInt();
		pSpin->setValue(iVal);
	}

	for(const t_tupCombo& tup : m_vecCombos)
	{
		const std::string& strKey = std::get<0>(tup);
		int iDef = std::get<1>(tup);
		QComboBox* pCombo = std::get<2>(tup);

		int iVal = m_pSettings->value(strKey.c_str(), iDef).toInt();
		pCombo->setCurrentIndex(iVal);
	}

	SetGlobals();
}


void SettingsDlg::SaveSettings()
{
	if(!m_pSettings) return;

	for(const t_tupEdit& tup : m_vecEdits)
	{
		const std::string& strKey = std::get<0>(tup);
		QLineEdit* pEdit = std::get<2>(tup);

		m_pSettings->setValue(strKey.c_str(), pEdit->text());
	}

	for(const t_tupCheck& tup : m_vecChecks)
	{
		const std::string& strKey = std::get<0>(tup);
		QCheckBox* pCheck = std::get<2>(tup);

		m_pSettings->setValue(strKey.c_str(), pCheck->isChecked());
	}

	for(const t_tupSpin& tup : m_vecSpins)
	{
		const std::string& strKey = std::get<0>(tup);
		QSpinBox* pSpin = std::get<2>(tup);

		m_pSettings->setValue(strKey.c_str(), pSpin->value());
	}

	for(const t_tupCombo& tup : m_vecCombos)
	{
		const std::string& strKey = std::get<0>(tup);
		QComboBox* pCombo = std::get<2>(tup);

		// save both combo-box index and value
		m_pSettings->setValue(strKey.c_str(), pCombo->currentIndex());
		m_pSettings->setValue((strKey+"_value").c_str(), pCombo->currentText());
	}

	SetGlobals();
}


static inline std::string find_program_binary(const std::string& strExe)
{
	// if the given binary file exists, directly use it
	if(tl::file_exists(strExe.c_str()))
	{
		tl::log_info("Found external tool: \"", strExe, "\".");
		return strExe;
	}

	// try prefixing it with the application path
	std::string strPath = g_strApp + "/" + strExe;
	if(tl::file_exists(strPath.c_str()))
	{
		tl::log_info("Found external tool in program path: \"", strPath, "\".");
		return strPath;
	}

	return strExe;
}


void SettingsDlg::SetGlobals() const
{
	// precisions
	g_iPrec = spinPrecGen->value();
	g_iPrecGfx = spinPrecGfx->value();

	g_iMaxThreads = spinThreads->value();
	g_bThreadedGL = checkThreadedGL->isChecked();

	g_dEps = std::pow(10., -t_real(g_iPrec));
	g_dEpsGfx = std::pow(10., -t_real(g_iPrecGfx));

	GFX_NUM_POINTS = spinPtsGfx->value();
	g_iMaxNN = spinMaxNN->value();


	g_bShowFsq = (comboSFact->currentIndex() == 1);
	g_b3dBZ = (comboBZ->currentIndex() == 0);


	// fonts
	QString strGfxFont = editGfxFont->text();
	if(strGfxFont.length() != 0)
	{
		QFont font;
		if(font.fromString(strGfxFont))
		{
			g_fontGfx = font;

			g_dFontSize = g_fontGfx.pointSizeF();
			if(g_dFontSize <= 0.) g_dFontSize = 10.;
		}
	}

	if(editGLFont->text().length() != 0)
		g_strFontGL = editGLFont->text().toStdString();

	if(spinGLFont->value() > 0)
		g_iFontGLSize = spinGLFont->value();

	QString strGenFont = editGenFont->text();
	if(strGenFont.length() != 0)
	{
		QFont font;
		if(font.fromString(strGenFont))
			g_fontGen = font;
	}


	// external tools
	if(editCif->text().length() != 0)
		g_strCifTool = find_program_binary(editCif->text().toStdString());

	if(editGpl->text().length() != 0)
		g_strGplTool = find_program_binary(editGpl->text().toStdString());


	// GUI style
	QString strStyle = comboGUI->currentText();
	QStyle *pStyle = QStyleFactory::create(strStyle);
	if(pStyle)
		QApplication::setStyle(pStyle);
	else
		tl::log_err("Style \"", strStyle.toStdString(), "\" was not found.");

	emit SettingsChanged();
}


void SettingsDlg::SelectGLFont()
{
	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(m_pSettings && !m_pSettings->value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	// find a default font directory
	std::string strFontDir;
	std::vector<std::string> vecFontDir = get_qt_std_path(QtStdPath::FONTS);
	if(vecFontDir.size() == 0)
		tl::log_warn("Could not determine font directory.");
	else
		strFontDir = vecFontDir[0];

	// get font dir either from font file or from default
	std::string strPath;
	if(g_strFontGL != "")
		strPath = tl::get_dir(g_strFontGL);
	else
		strPath = strFontDir;

	QString strFile = QFileDialog::getOpenFileName(this,
		"Open Font File...", strPath.c_str(),
		"Font Files (*.ttf *.TTF)", nullptr, fileopt);
	if(strFile == "")
		return;

	editGLFont->setText(strFile);
}


void SettingsDlg::SelectGfxFont()
{
	bool bOk = 0;
	QFont fontNew = QFontDialog::getFont(&bOk, g_fontGfx, this);
	if(bOk)
	{
		g_fontGfx = fontNew;
		g_dFontSize = g_fontGfx.pointSizeF();
		if(g_dFontSize <= 0.) g_dFontSize = 10.;

		editGfxFont->setText(fontNew.toString());
	}
}


void SettingsDlg::SelectGenFont()
{
	bool bOk = 0;
	QFont fontNew = QFontDialog::getFont(&bOk, g_fontGen, this);
	if(bOk)
	{
		g_fontGen = fontNew;
		editGenFont->setText(fontNew.toString());
	}
}


void SettingsDlg::SelectCifTool()
{
	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(m_pSettings && !m_pSettings->value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strFile = QFileDialog::getOpenFileName(this,
		"Select Cif2Xml Tool...", "",
		"Executable Files (* *.exe *.EXE)", nullptr, fileopt);
	if(strFile == "")
		return;

	editCif->setText(strFile);
}


void SettingsDlg::SelectGplTool()
{
	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(m_pSettings && !m_pSettings->value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strFile = QFileDialog::getOpenFileName(this,
		"Select Gnuplot Tool...", "",
		"Executable Files (* *.exe *.EXE)", nullptr, fileopt);
	if(strFile == "")
		return;

	editGpl->setText(strFile);
}


void SettingsDlg::showEvent(QShowEvent *pEvt)
{
	QDialog::showEvent(pEvt);
}


void SettingsDlg::ButtonBoxClicked(QAbstractButton *pBtn)
{
	if(buttonBox->buttonRole(pBtn) == QDialogButtonBox::ApplyRole ||
	   buttonBox->buttonRole(pBtn) == QDialogButtonBox::AcceptRole)
	{
		SaveSettings();
	}
	else if(buttonBox->buttonRole(pBtn) == QDialogButtonBox::ResetRole)
	{
		SetDefaults(1);
		LoadSettings();
	}
	else if(buttonBox->buttonRole(pBtn) == QDialogButtonBox::RejectRole)
	{
		QDialog::reject();
	}

	if(buttonBox->buttonRole(pBtn) == QDialogButtonBox::AcceptRole)
	{
		if(m_pSettings)
			m_pSettings->setValue("settings/geo", saveGeometry());

		QDialog::accept();
	}
}


#include "SettingsDlg.moc"
