/**
 * recent files helper
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date feb-2015
 * @license GPLv2
 */

#ifndef __RECENT_FILES_H__
#define __RECENT_FILES_H__

#include <QSettings>
#include <QString>
#include <QStringList>
#include <QSignalMapper>
#include <QMenu>


template<class t_cont, class t_str = typename t_cont::value_type>
QStringList cont_to_qstrlist(const t_cont& cont)
{
	QStringList lst;
	for(const t_str& str : cont)
		lst.push_back(str.c_str());
	return lst;
}

template<class t_cont, class t_str = typename t_cont::value_type>
t_cont qstrlist_to_cont(const QStringList& lstStr)
{
	t_cont cont;
	for(int i=0; i<lstStr.size(); ++i)
		cont.push_back(lstStr[i].toStdString());
	return cont;
}


class RecentFiles
{
	protected:
		QSettings *m_pSettings = nullptr;
		std::string m_strKey;
		typedef std::list<std::string> t_lstFiles;
		t_lstFiles m_lstFiles;
		std::size_t m_iMaxFiles = 16;

	public:
		RecentFiles(QSettings *pSett, const char* pcKey);
		virtual ~RecentFiles();

		void LoadList();
		void SaveList();

		bool HasFile(const char* pcFile) const;
		void AddFile(const char* pcFile);
		void RemoveFile(const char* pcFile);

		void SetMaxSize(std::size_t iMaxFiles);

		void FillMenu(QMenu*, QSignalMapper*);
};

#endif
