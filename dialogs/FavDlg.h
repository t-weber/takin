/**
 * Favourite Positions Dialog
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date aug-2016
 * @license GPLv2
 */

#ifndef __FAV_DLG_H__
#define __FAV_DLG_H__

#include <QDialog>
#include <QSettings>
#include "ui/ui_fav.h"

#include "tlibs/file/prop.h"
#include "libs/globals.h"
#include "libs/globals_qt.h"


struct FavHklPos
{
	t_real_glob dhstart=0, dkstart=0, dlstart=0, dEstart=0;
	t_real_glob dhstop=0, dkstop=0, dlstop=0, dEstop=0;
};


class FavDlg : public QDialog, Ui::FavDlg
{ Q_OBJECT
	public:
		FavDlg(QWidget* pParent=0, QSettings* pSett=0);
		virtual ~FavDlg();

	protected:
		QSettings *m_pSettings = 0;
		FavHklPos m_curPos;

		void ApplyPos(const FavHklPos* pPos);

	public slots:
		void ClearList();

		void AddPosToList(const FavHklPos& pos);
		void AddPosToList();
		void RemPosFromList();

		void ListItemSelected();
		void ListItemDoubleClicked(QListWidgetItem*);

		void UpdateCurPos(const struct FavHklPos& pos) { m_curPos = pos; }

	signals:
		void ChangePos(const struct FavHklPos& pos);

	protected slots:
		void ButtonBoxClicked(QAbstractButton* pBtn);

	protected:
		virtual void showEvent(QShowEvent *pEvt) override;

	public:
		void Save(std::map<std::string, std::string>& mapConf, const std::string& strXmlRoot);
		void Load(tl::Prop<std::string>& xml, const std::string& strXmlRoot);
};

#endif
