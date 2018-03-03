/**
 * monte carlo convolution tool
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 2015, 2016
 * @license GPLv2
 */

#include "ConvoDlg.h"
#include "tlibs/string/string.h"
#include "tlibs/math/math.h"
#include "tlibs/file/file.h"

#include "libs/globals.h"
#include "libs/globals_qt.h"
#include "libs/qt/recent.h"
#include "tools/convofit/convofit_import.h"

#include <iostream>
#include <fstream>

#include <QFileDialog>
#include <QMessageBox>
#include <QMenuBar>
#include <QMenu>

using t_real = t_real_reso;


// -----------------------------------------------------------------------------
// file operations

void ConvoDlg::New()
{
	clear_global_paths();
	m_strLastFile = "";
	setWindowTitle(s_strTitle.c_str());
}

void ConvoDlg::Load()
{
	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(m_pSett && !m_pSett->value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = "";
	if(m_pSett)
		strDirLast = m_pSett->value("monteconvo/last_dir", ".").toString();
	QString _strFile = QFileDialog::getOpenFileName(this,
		"Open Convolution Configuration...", strDirLast, "Takin files (*.taz *.TAZ)",
		nullptr, fileopt);

	Load(_strFile);
}

void ConvoDlg::Load(const QString& _strFile)
{
	const std::string strXmlRoot("taz/");

	if(_strFile == "")
		return;

	std::string strFile = _strFile.toStdString();
	if(!tl::file_exists(strFile.c_str()))
		return;

	// add the location of the convo file as a possible global path
	std::string strGlobDir = tl::get_dir(strFile);
	clear_global_paths();
	if(strGlobDir != "")
		add_global_path(strGlobDir);


	tl::Prop<std::string> xml;
	if(!xml.Load(strFile, tl::PropType::XML))
	{
		QMessageBox::critical(this, "Error", "Could not load convolution file.");
		return;
	}

	Load(xml, strXmlRoot);

	m_strLastFile = strFile;
	std::string strDir = tl::get_dir(m_strLastFile);
	setWindowTitle((s_strTitle + " - ").c_str() + _strFile);

	if(m_pSett)
	{
		m_pSett->setValue("monteconvo/last_dir", QString(strDir.c_str()));

		RecentFiles recent(m_pSett, "monteconvo/recent");
		recent.AddFile(m_strLastFile.c_str());
		recent.SaveList();
		recent.FillMenu(m_pMenuRecent, m_pMapperRecent);
	}
}


void ConvoDlg::SaveAs()
{
	const std::string strXmlRoot("taz/");

	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(m_pSett && !m_pSett->value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = "";
	if(m_pSett)
		strDirLast = m_pSett->value("monteconvo/last_dir", ".").toString();
	QString _strFile = QFileDialog::getSaveFileName(this,
		"Save Convolution Configuration", strDirLast, "Takin files (*.taz *.TAZ)",
		nullptr, fileopt);

	if(_strFile == "")
		return;

	m_strLastFile = _strFile.toStdString();
	std::string strDir = tl::get_dir(m_strLastFile);
	if(tl::get_fileext(m_strLastFile,1) != "taz")
		m_strLastFile += ".taz";

	Save();

	if(m_pSett)
	{
		m_pSett->setValue("monteconvo/last_dir", QString(strDir.c_str()));

		RecentFiles recent(m_pSett, "monteconvo/recent");
		recent.AddFile(m_strLastFile.c_str());
		recent.SaveList();
		recent.FillMenu(m_pMenuRecent, m_pMapperRecent);
	}
}


void ConvoDlg::Save()
{
	const std::string strXmlRoot("taz/");

	if(m_strLastFile == "")
	{
		SaveAs();
		return;
	}

	setWindowTitle((s_strTitle + " - " + m_strLastFile).c_str());

	std::map<std::string, std::string> mapConf;
	Save(mapConf, strXmlRoot);

	tl::Prop<std::string> xml;
	xml.Add(mapConf);
	if(!xml.Save(m_strLastFile, tl::PropType::XML))
	{
		QMessageBox::critical(this, "Error", "Could not save convolution file.");
		return;
	}
}


void ConvoDlg::Load(tl::Prop<std::string>& xml, const std::string& strXmlRoot)
{
	m_bAllowSqwReinit = 0;

	for(std::size_t iCheck=0; iCheck<m_vecCheckBoxes.size(); ++iCheck)
	{
		boost::optional<int> obChecked = xml.QueryOpt<int>(strXmlRoot+m_vecCheckNames[iCheck]);
		if(obChecked) m_vecCheckBoxes[iCheck]->setChecked(*obChecked);
	}
	for(std::size_t iSpinBox=0; iSpinBox<m_vecSpinBoxes.size(); ++iSpinBox)
	{
		boost::optional<t_real> odSpinVal = xml.QueryOpt<t_real>(strXmlRoot+m_vecSpinNames[iSpinBox]);
		if(odSpinVal) m_vecSpinBoxes[iSpinBox]->setValue(*odSpinVal);
	}
	for(std::size_t iSpinBox=0; iSpinBox<m_vecIntSpinBoxes.size(); ++iSpinBox)
	{
		boost::optional<int> odSpinVal = xml.QueryOpt<int>(strXmlRoot+m_vecIntSpinNames[iSpinBox]);
		if(odSpinVal) m_vecIntSpinBoxes[iSpinBox]->setValue(*odSpinVal);
	}
	for(std::size_t iCombo=0; iCombo<m_vecComboBoxes.size(); ++iCombo)
	{
		boost::optional<int> oiComboIdx = xml.QueryOpt<int>(strXmlRoot+m_vecComboNames[iCombo]);
		if(oiComboIdx) m_vecComboBoxes[iCombo]->setCurrentIndex(*oiComboIdx);
	}
	for(std::size_t iEditBox=0; iEditBox<m_vecEditBoxes.size(); ++iEditBox)
	{
		boost::optional<std::string> odEditVal = xml.QueryOpt<std::string>(strXmlRoot+m_vecEditNames[iEditBox]);
		if(odEditVal) m_vecEditBoxes[iEditBox]->setText((*odEditVal).c_str());
	}
	for(std::size_t iEditBox=0; iEditBox<m_vecTextBoxes.size(); ++iEditBox)
	{
		boost::optional<std::string> odEditVal = xml.QueryOpt<std::string>(strXmlRoot+m_vecTextNames[iEditBox]);
		if(odEditVal) m_vecTextBoxes[iEditBox]->setPlainText((*odEditVal).c_str());
	}

	if(m_pFavDlg)
		m_pFavDlg->Load(xml, strXmlRoot + "monteconvo/");

	boost::optional<std::string> opSqw = xml.QueryOpt<std::string>(strXmlRoot + "monteconvo/sqw");
	if(opSqw)
	{
		QString qstrSqw = (*opSqw).c_str();
		comboSqw->setCurrentIndex(comboSqw->findData(qstrSqw));

		m_bAllowSqwReinit = 1;
		QString qstrSqwConf = xml.Query<std::string>(strXmlRoot + "monteconvo/sqw_conf", "").c_str();
		createSqwModel(qstrSqwConf);

		if(m_pSqw)
		{
			load_sqw_params(m_pSqw.get(), xml, strXmlRoot + "monteconvo/");
			emit SqwLoaded(m_pSqw->GetVars(), &m_pSqw->GetFitVars());
		}
	}
	m_bAllowSqwReinit = 1;
}


void ConvoDlg::Save(std::map<std::string, std::string>& mapConf, const std::string& strXmlRoot)
{
	for(std::size_t iSpinBox=0; iSpinBox<m_vecSpinBoxes.size(); ++iSpinBox)
	{
		std::ostringstream ostrVal;
		ostrVal.precision(g_iPrec);
		ostrVal << std::scientific;
		ostrVal << m_vecSpinBoxes[iSpinBox]->value();

		mapConf[strXmlRoot + m_vecSpinNames[iSpinBox]] = ostrVal.str();
	}
	for(std::size_t iSpinBox=0; iSpinBox<m_vecIntSpinBoxes.size(); ++iSpinBox)
	{
		std::ostringstream ostrVal;
		ostrVal << std::scientific;
		ostrVal << m_vecIntSpinBoxes[iSpinBox]->value();

		mapConf[strXmlRoot + m_vecIntSpinNames[iSpinBox]] = ostrVal.str();
	}
	for(std::size_t iEditBox=0; iEditBox<m_vecEditBoxes.size(); ++iEditBox)
	{
		std::string strVal = m_vecEditBoxes[iEditBox]->text().toStdString();
		mapConf[strXmlRoot + m_vecEditNames[iEditBox]] = strVal;
	}
	for(std::size_t iCombo=0; iCombo<m_vecComboBoxes.size(); ++iCombo)
		mapConf[strXmlRoot + m_vecComboNames[iCombo]] = tl::var_to_str<int>(m_vecComboBoxes[iCombo]->currentIndex());
	for(std::size_t iCheckBox=0; iCheckBox<m_vecCheckBoxes.size(); ++iCheckBox)
		mapConf[strXmlRoot + m_vecCheckNames[iCheckBox]] = (m_vecCheckBoxes[iCheckBox]->isChecked() ? "1" : "0");

	if(m_pFavDlg)
		m_pFavDlg->Save(mapConf, strXmlRoot + "monteconvo/");
	if(m_pSqw)
	{
		save_sqw_params(m_pSqw.get(), mapConf, strXmlRoot + "monteconvo/");
		mapConf[strXmlRoot + "monteconvo/sqw"] = 
			comboSqw->itemData(comboSqw->currentIndex()).toString().toStdString();
	}

	//mapConf[strXmlRoot + "meta/timestamp"] = tl::var_to_str<t_real>(tl::epoch<t_real>());
}


// -----------------------------------------------------------------------------
// exporting

void ConvoDlg::SaveConvofit()
{
	const std::string strXmlRoot("taz/");

	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(m_pSett && !m_pSett->value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = "";
	if(m_pSett)
		strDirLast = m_pSett->value("monteconvo/last_dir_convofit", ".").toString();
	QString _strFile = QFileDialog::getSaveFileName(this,
		"Export to Convofit", strDirLast, "Convofit files (*.job *.JOB)",
		nullptr, fileopt);

	if(_strFile == "")
		return;

	std::string strFile = _strFile.toStdString();
	std::string strDir = tl::get_dir(strFile);
	if(tl::get_fileext(strFile,1) != "job")
		strFile += ".job";


	std::map<std::string, std::string> mapConf;
	Save(mapConf, strXmlRoot);

	tl::Prop<std::string> xml;
	xml.Add(mapConf);

	if(convert_monteconvo(xml, strFile, checkRel->isChecked()) != strFile)
	{
		QMessageBox::critical(this, "Error", "Could not export convofit job file.");
		return;
	}

	if(m_pSett)
		m_pSett->setValue("monteconvo/last_dir_convofit", QString(strDir.c_str()));
}


// -----------------------------------------------------------------------------


void ConvoDlg::SaveResult()
{
	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(m_pSett && !m_pSett->value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = ".";
	if(m_pSett)
		strDirLast = m_pSett->value("convo/last_dir_result", ".").toString();

	QString strFile = QFileDialog::getSaveFileName(this,
		"Save Scan", strDirLast, "Data Files (*.dat *.DAT)", nullptr, fileopt);

	if(strFile == "")
		return;

	std::string strFile1 = strFile.toStdString();
	std::string strDir = tl::get_dir(strFile1);
	if(tl::get_fileext(strFile1,1) != "dat")
		strFile1 += ".dat";


	std::ofstream ofstr(strFile1);
	if(!ofstr)
	{
		QMessageBox::critical(this, "Error", "Could not open file.");
		return;
	}

	std::string strResult = textResult->toPlainText().toStdString();
	ofstr.write(strResult.c_str(), strResult.size());

	if(m_pSett)
		m_pSett->setValue("convo/last_dir_result", QString(strDir.c_str()));
}
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// loading & saving of recent configuration
void ConvoDlg::LoadSettings()
{
	if(!m_pSett) return;
	m_bAllowSqwReinit = 0;

	for(std::size_t iSpinBox=0; iSpinBox<m_vecSpinBoxes.size(); ++iSpinBox)
	{
		if(!m_pSett->contains(m_vecSpinNames[iSpinBox].c_str()))
			continue;
		m_vecSpinBoxes[iSpinBox]->setValue(m_pSett->value(m_vecSpinNames[iSpinBox].c_str()).value<t_real>());
	}
	for(std::size_t iSpinBox=0; iSpinBox<m_vecIntSpinBoxes.size(); ++iSpinBox)
	{
		if(!m_pSett->contains(m_vecIntSpinNames[iSpinBox].c_str()))
			continue;
		m_vecIntSpinBoxes[iSpinBox]->setValue(m_pSett->value(m_vecIntSpinNames[iSpinBox].c_str()).value<int>());
	}
	for(std::size_t iCombo=0; iCombo<m_vecComboBoxes.size(); ++iCombo)
	{
		if(!m_pSett->contains(m_vecComboNames[iCombo].c_str()))
			continue;
		m_vecComboBoxes[iCombo]->setCurrentIndex(m_pSett->value(m_vecComboNames[iCombo].c_str()).value<int>());
	}

	if(m_pSett->contains("monteconvo/sqw"))
		comboSqw->setCurrentIndex(comboSqw->findData(m_pSett->value("monteconvo/sqw").toString()));

	for(std::size_t iEditBox=0; iEditBox<m_vecEditBoxes.size(); ++iEditBox)
	{
		if(!m_pSett->contains(m_vecEditNames[iEditBox].c_str()))
			continue;
		m_vecEditBoxes[iEditBox]->setText(m_pSett->value(m_vecEditNames[iEditBox].c_str()).toString());
	}
	for(std::size_t iCheckBox=0; iCheckBox<m_vecCheckBoxes.size(); ++iCheckBox)
	{
		if(!m_pSett->contains(m_vecCheckNames[iCheckBox].c_str()))
			continue;
		m_vecCheckBoxes[iCheckBox]->setChecked(m_pSett->value(m_vecCheckNames[iCheckBox].c_str()).value<bool>());
	}

	//if(editSqw->text() != "")
	{
		m_bAllowSqwReinit = 1;
		createSqwModel(editSqw->text());
	}
	m_bAllowSqwReinit = 1;


	if(m_pSett->contains("monteconvo/geo"))
		restoreGeometry(m_pSett->value("monteconvo/geo").toByteArray());
}

void ConvoDlg::showEvent(QShowEvent *pEvt)
{
	//LoadSettings();
	QDialog::showEvent(pEvt);
}

void ConvoDlg::accept()
{
	if(!m_pSett) return;

	for(std::size_t iSpinBox=0; iSpinBox<m_vecSpinBoxes.size(); ++iSpinBox)
		m_pSett->setValue(m_vecSpinNames[iSpinBox].c_str(), m_vecSpinBoxes[iSpinBox]->value());
	for(std::size_t iSpinBox=0; iSpinBox<m_vecIntSpinBoxes.size(); ++iSpinBox)
		m_pSett->setValue(m_vecIntSpinNames[iSpinBox].c_str(), m_vecIntSpinBoxes[iSpinBox]->value());
	for(std::size_t iEditBox=0; iEditBox<m_vecEditBoxes.size(); ++iEditBox)
		m_pSett->setValue(m_vecEditNames[iEditBox].c_str(), m_vecEditBoxes[iEditBox]->text());
	for(std::size_t iEditBox=0; iEditBox<m_vecTextBoxes.size(); ++iEditBox)
		m_pSett->setValue(m_vecTextNames[iEditBox].c_str(), m_vecTextBoxes[iEditBox]->toPlainText());
	for(std::size_t iCombo=0; iCombo<m_vecComboBoxes.size(); ++iCombo)
		m_pSett->setValue(m_vecComboNames[iCombo].c_str(), m_vecComboBoxes[iCombo]->currentIndex());
	for(std::size_t iCheck=0; iCheck<m_vecCheckBoxes.size(); ++iCheck)
		m_pSett->setValue(m_vecCheckNames[iCheck].c_str(), m_vecCheckBoxes[iCheck]->isChecked());

	m_pSett->setValue("monteconvo/sqw", comboSqw->itemData(comboSqw->currentIndex()).toString());
	m_pSett->setValue("monteconvo/geo", saveGeometry());

	QDialog::accept();
}


// -----------------------------------------------------------------------------

void ConvoDlg::browseCrysFiles()
{
	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(m_pSett && !m_pSett->value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = ".";
	if(m_pSett)
		strDirLast = m_pSett->value("convo/last_dir_crys", ".").toString();
	QString strFile = QFileDialog::getOpenFileName(this,
		"Open Crystal File...", strDirLast, "Takin files (*.taz *.TAZ)",
		nullptr, fileopt);
	if(strFile == "")
		return;

	editCrys->setText(strFile);

	std::string strDir = tl::get_dir(strFile.toStdString());
	if(m_pSett)
		m_pSett->setValue("convo/last_dir_crys", QString(strDir.c_str()));
}

void ConvoDlg::browseResoFiles()
{
	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(m_pSett && !m_pSett->value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = ".";
	if(m_pSett)
		strDirLast = m_pSett->value("convo/last_dir_reso", ".").toString();
	QString strFile = QFileDialog::getOpenFileName(this,
		"Open Resolution File...", strDirLast, "Takin files (*.taz *.TAZ)",
		nullptr, fileopt);
	if(strFile == "")
		return;

	editRes->setText(strFile);

	std::string strDir = tl::get_dir(strFile.toStdString());
	if(m_pSett)
		m_pSett->setValue("convo/last_dir_reso", QString(strDir.c_str()));
}

void ConvoDlg::browseSqwFiles()
{
	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(m_pSett && !m_pSett->value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = ".";
	if(m_pSett)
		strDirLast = m_pSett->value("convo/last_dir_sqw", ".").toString();
	QString strFile = QFileDialog::getOpenFileName(this,
		"Open S(q,w) File...", strDirLast, "All S(q,w) files (*.dat *.DAT *.py *.PY *.jl *.JL)",
		nullptr, fileopt);
	if(strFile == "")
		return;

	editSqw->setText(strFile);

	std::string strDir = tl::get_dir(strFile.toStdString());
	if(m_pSett)
		m_pSett->setValue("convo/last_dir_sqw", QString(strDir.c_str()));
}

void ConvoDlg::browseScanFiles()
{
	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(m_pSett && !m_pSett->value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = ".";
	if(m_pSett)
		strDirLast = m_pSett->value("convo/last_dir_scan", ".").toString();
	QString strFile = QFileDialog::getOpenFileName(this,
		"Open S(q,w) File...", strDirLast, "All scan files (*.dat *.DAT *.scn *.SCN)",
		nullptr, fileopt);
	if(strFile == "")
		return;

	editScan->setText(strFile);

	std::string strDir = tl::get_dir(strFile.toStdString());
	if(m_pSett)
		m_pSett->setValue("convo/last_dir_scan", QString(strDir.c_str()));
}

// -----------------------------------------------------------------------------
