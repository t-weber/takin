/**
 * Settings
 * @author tweber
 * @date 5-dec-2014
 * @license GPLv2
 */

#ifndef __TAZ_SETTINGS_H__
#define __TAZ_SETTINGS_H__

#include <QDialog>
#include <QSettings>

#include <tuple>
#include <vector>
#include <string>

#include "ui/ui_settings.h"


class SettingsDlg : public QDialog, Ui::SettingsDlg
{ Q_OBJECT
	protected:
		QSettings *m_pSettings = 0;

		// key, default, lineedit
		typedef std::tuple<std::string, std::string, QLineEdit*> t_tupEdit;
		std::vector<t_tupEdit> m_vecEdits;

		// checkboxes
		typedef std::tuple<std::string, bool, QCheckBox*> t_tupCheck;
		std::vector<t_tupCheck> m_vecChecks;

		// spins
		typedef std::tuple<std::string, int, QSpinBox*> t_tupSpin;
		std::vector<t_tupSpin> m_vecSpins;

		// combos
		typedef std::tuple<std::string, int, QComboBox*> t_tupCombo;
		std::vector<t_tupCombo> m_vecCombos;

	public:
		SettingsDlg(QWidget* pParent=0, QSettings* pSett=0);
		virtual ~SettingsDlg();

	signals:
		void SettingsChanged() const;

	protected:
		void LoadSettings();
		void SaveSettings();

		void SetDefaults(bool bOverwrite=0);
		void SetGlobals() const;

	protected:
		virtual void showEvent(QShowEvent *pEvt) override;

	protected slots:
		void ButtonBoxClicked(QAbstractButton* pBtn);
		void SelectGLFont();
		void SelectGfxFont();
		void SelectGenFont();
};

#endif
