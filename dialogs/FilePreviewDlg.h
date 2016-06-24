/*
 * File Preview Dialog
 * @author Tobias Weber
 * @date feb-2015
 * @license GPLv2
 */

#ifndef __FILE_PREV_DLG__
#define __FILE_PREV_DLG__

#include <vector>
#include <memory>

#include <QFileDialog>
#include <QString>
#include <QSettings>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

#include "libs/qthelper.h"
#include "libs/globals.h"


class FilePreviewDlg : public QFileDialog
{ Q_OBJECT
	protected:
		QSettings *m_pSettings = nullptr;

		std::unique_ptr<QwtPlot> m_pPlot;
		std::unique_ptr<QwtPlotWrapper> m_plotwrap;
		std::vector<t_real_glob> m_vecScn, m_vecCts;

	protected:
		void ClearPlot();

	public:
		FilePreviewDlg(QWidget* pParent, const char* pcTitle, QSettings* pSett=0);
		virtual ~FilePreviewDlg();

	protected slots:
		void FileSelected(const QString& strFile);
};

#endif
