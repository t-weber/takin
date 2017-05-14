/**
 * recent files helper
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date feb-2015
 * @license GPLv2
 */

#include "recent.h"
#include <algorithm>
#include <iostream>
#include <QAction>


RecentFiles::RecentFiles(QSettings *pSett, const char* pcKey)
	: m_pSettings(pSett), m_strKey(pcKey)
{
	LoadList();
}

RecentFiles::~RecentFiles()
{
	//SaveList();
}

void RecentFiles::LoadList()
{
	QString strKey(m_strKey.c_str());
	QStringList lstStr = m_pSettings->value(strKey).value<QStringList>();
	lstStr.removeDuplicates();
	m_lstFiles = qstrlist_to_cont<t_lstFiles>(lstStr);

	if(m_lstFiles.size() > m_iMaxFiles)
		m_lstFiles.resize(m_iMaxFiles);
}

void RecentFiles::SaveList()
{
	QString strKey(m_strKey.c_str());
	QStringList lstStr = cont_to_qstrlist<t_lstFiles>(m_lstFiles);
	m_pSettings->setValue(strKey, lstStr);
}

bool RecentFiles::HasFile(const char* pcFile) const
{
	t_lstFiles::const_iterator iter = std::find(m_lstFiles.begin(), m_lstFiles.end(), std::string(pcFile));
	return (iter != m_lstFiles.end());
}

void RecentFiles::RemoveFile(const char* pcFile)
{
	t_lstFiles::iterator iter = std::remove(m_lstFiles.begin(), m_lstFiles.end(), std::string(pcFile));
	//if(iter != m_lstFiles.end())
	//	m_lstFiles = t_lstFiles(m_lstFiles.begin(), iter);
	if(iter != m_lstFiles.end())
		m_lstFiles.resize(m_lstFiles.size()-1);
}

void RecentFiles::AddFile(const char* pcFile)
{
	//if(HasFile(pcFile))
	//	return;
	RemoveFile(pcFile);

	m_lstFiles.push_front(std::string(pcFile));
	if(m_lstFiles.size() > m_iMaxFiles)
		m_lstFiles.resize(m_iMaxFiles);
}

void RecentFiles::SetMaxSize(std::size_t iMaxFiles)
{
	m_iMaxFiles = iMaxFiles;
	if(m_lstFiles.size() > iMaxFiles)
		m_lstFiles.resize(iMaxFiles);
}

void RecentFiles::FillMenu(QMenu* pMenu, QSignalMapper* pMapper)
{
	// clear old actions
	QList<QAction*> lstActions = pMenu->actions();
	for(int iAction=0; iAction<lstActions.size(); ++iAction)
	{
		pMapper->removeMappings(lstActions[iAction]);
		delete lstActions[iAction];
		lstActions[iAction] = nullptr;
	}
	pMenu->clear();

	// setup new actions
	for(const t_lstFiles::value_type& str : m_lstFiles)
	{
		QAction *pAction = new QAction(pMenu);
		QObject::connect(pAction, SIGNAL(triggered()), pMapper, SLOT(map()));
		pMapper->setMapping(pAction, QString(str.c_str()));

		pAction->setText(str.c_str());
		pMenu->addAction(pAction);
	}
}
