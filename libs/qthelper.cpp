/**
 * qt&qwt helpers
 * @author Tobias Weber
 * @date feb-2016
 * @license GPLv2
 */

#include "qthelper.h"
#include "globals.h"
#include "globals_qt.h"
#include "tlibs/math/math.h"
#include "tlibs/string/string.h"

#include <algorithm>
#include <fstream>
//#include <iostream>
#include <iomanip>
#include <memory>

#include <qwt_picker_machine.h>
#include <qwt_plot_canvas.h>

#include <QFileDialog>
#include <QMessageBox>
#include <QMouseEvent>


class MyQwtPlotZoomer : public QwtPlotZoomer
{
protected:
	virtual void widgetMouseReleaseEvent(QMouseEvent *pEvt) override
	{
		// filter out middle button event which is already used for panning
		if(pEvt->button() & Qt::MiddleButton)
			return;

		QwtPlotZoomer::widgetMouseReleaseEvent(pEvt);
	}

public:
	template<class t_widget_or_canvas>
	explicit MyQwtPlotZoomer(t_widget_or_canvas* ptr) : QwtPlotZoomer(ptr)
	{
		QwtPlotZoomer::setMaxStackDepth(-1);
		QwtPlotZoomer::setEnabled(1);
	}

	virtual ~MyQwtPlotZoomer() {}
};


// ----------------------------------------------------------------------------


class MyQwtPlotPicker : public QwtPlotPicker
{
protected:
	QwtPlotWrapper *m_pPlotWrap = nullptr;

	virtual void widgetKeyPressEvent(QKeyEvent *pEvt) override
	{
		const int iKey = pEvt->key();
		//std::cout << "Plot key: " <<  iKey << std::endl;
		if(iKey == Qt::Key_S)
			m_pPlotWrap->SavePlot();
		else
			QwtPlotPicker::widgetKeyPressEvent(pEvt);
	}

public:
	MyQwtPlotPicker(QwtPlotWrapper *pPlotWrap, bool bNoTrackerSignal=0) :
		QwtPlotPicker(pPlotWrap->GetPlot()->xBottom, pPlotWrap->GetPlot()->yLeft,
#if QWT_VER<6
		QwtPlotPicker::PointSelection,
#endif
		QwtPlotPicker::NoRubberBand,
		bNoTrackerSignal ? QwtPicker::AlwaysOn : QwtPicker::AlwaysOff,
		pPlotWrap->GetPlot()->canvas()),
		m_pPlotWrap(pPlotWrap)
	{
#if QWT_VER>=6
		QwtPlotPicker::setStateMachine(new QwtPickerTrackerMachine());
		//connect(this, SIGNAL(moved(const QPointF&)), this, SLOT(cursorMoved(const QPointF&)));
#endif
		QwtPlotPicker::setEnabled(1);
	}

	virtual ~MyQwtPlotPicker() {}
};


// ----------------------------------------------------------------------------


QwtPlotWrapper::QwtPlotWrapper(QwtPlot *pPlot,
	unsigned int iNumCurves, bool bNoTrackerSignal)
	: m_pPlot(pPlot)
{
	QPen penGrid;
	penGrid.setColor(QColor(0x99,0x99,0x99));
	penGrid.setStyle(Qt::DashLine);

	QPen penCurve;
	penCurve.setColor(QColor(0,0,0x99));
	penCurve.setWidth(2);

	QColor colorBck(240, 240, 240, 255);
	m_pPlot->setCanvasBackground(colorBck);

	m_vecCurves.reserve(iNumCurves);
	for(unsigned int iCurve=0; iCurve<iNumCurves; ++iCurve)
	{
		QwtPlotCurve* pCurve = new QwtPlotCurve();
		pCurve->setPen(penCurve);
		pCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
		pCurve->attach(m_pPlot);
		m_vecCurves.push_back(pCurve);
	}

	m_pGrid = new QwtPlotGrid();
	m_pGrid->setPen(penGrid);
	m_pGrid->attach(m_pPlot);


	m_pPanner = new QwtPlotPanner(m_pPlot->canvas());
	m_pPanner->setMouseButton(Qt::MiddleButton);


#if QWT_VER>=6
	m_pZoomer = new MyQwtPlotZoomer(m_pPlot->canvas());
#else
	// display tracker overlay anyway
	if(bNoTrackerSignal==0) bNoTrackerSignal = 1;
#endif


	m_pPicker = new MyQwtPlotPicker(this, bNoTrackerSignal);
	m_pPlot->canvas()->setMouseTracking(1);


	// fonts
	QwtText txt = m_pPlot->title();
	txt.setFont(g_fontGfx);
	m_pPlot->setTitle(txt);
	for(QwtPlot::Axis ax : {QwtPlot::yLeft, QwtPlot::yRight, QwtPlot::xBottom, QwtPlot::xTop})
	{
		txt = m_pPlot->axisTitle(ax);
		txt.setFont(g_fontGfx);
		m_pPlot->setAxisTitle(ax, txt);
	}
}


QwtPlotWrapper::~QwtPlotWrapper()
{
	if(m_pPicker)
	{
		m_pPicker->setEnabled(0);
		delete m_pPicker;
		m_pPicker = nullptr;
	}
	if(m_pGrid)
	{
		delete m_pGrid;
		m_pGrid = nullptr;
	}
	if(m_pZoomer)
	{
		delete m_pZoomer;
		m_pZoomer = nullptr;
	}
	if(m_pPanner)
	{
		delete m_pPanner;
		m_pPanner = nullptr;
	}
}


#if QWT_VER>=6
	bool QwtPlotWrapper::HasTrackerSignal() const { return 1; }
#else
	bool QwtPlotWrapper::HasTrackerSignal() const { return 0; }
#endif


void QwtPlotWrapper::SetData(const std::vector<t_real_qwt>& vecX, const std::vector<t_real_qwt>& vecY, 
	unsigned int iCurve, bool bReplot, bool bCopy)
{
	if(!bCopy)	// copy pointers
	{
#if QWT_VER>=6
		m_vecCurves[iCurve]->setRawSamples(vecX.data(), vecY.data(), std::min<t_real_qwt>(vecX.size(), vecY.size()));
#else
		m_vecCurves[iCurve]->setRawData(vecX.data(), vecY.data(), std::min<t_real_qwt>(vecX.size(), vecY.size()));
#endif

		m_bHasDataPtrs = 1;
		if(iCurve >= m_vecDataPtrs.size())
			m_vecDataPtrs.resize(iCurve+1);

		m_vecDataPtrs[iCurve].first = &vecX;
		m_vecDataPtrs[iCurve].second = &vecY;
	}
	else		// copy data
	{
#if QWT_VER>=6
		m_vecCurves[iCurve]->setSamples(vecX.data(), vecY.data(), std::min<t_real_qwt>(vecX.size(), vecY.size()));
#else
		m_vecCurves[iCurve]->setData(vecX.data(), vecY.data(), std::min<t_real_qwt>(vecX.size(), vecY.size()));
#endif

		m_bHasDataPtrs = 0;
		m_vecDataPtrs.clear();
	}

	if(bReplot)
	{
		set_zoomer_base(m_pZoomer, vecX, vecY);
		m_pPlot->replot();
	}
}


void QwtPlotWrapper::SavePlot() const
{
	if(!m_bHasDataPtrs)
	{
		// if data was deep-copied, it is now lost somewhere in the plot objects...
		QMessageBox::critical(m_pPlot, "Error", "Cannot get plot data.");
		return;
	}

	QFileDialog::Option fileopt = QFileDialog::Option(0);
	//if(!m_settings.value("main/native_dialogs", 1).toBool())
	//	fileopt = QFileDialog::DontUseNativeDialog;

	std::string strFile = QFileDialog::getSaveFileName(m_pPlot,
		"Save Plot Data", nullptr, "Data files (*.dat *.DAT)",
		nullptr, fileopt).toStdString();

	tl::trim(strFile);
	if(strFile == "")
		return;

	std::ofstream ofstrDat(strFile);
	if(!ofstrDat)
	{
		std::string strErr = "Cannot open file \"" + strFile + "\" for saving.";
		QMessageBox::critical(m_pPlot, "Error", strErr.c_str());
		return;
	}

	ofstrDat.precision(g_iPrec);
	ofstrDat << "# title: " << m_pPlot->title().text().toStdString() << "\n";
	ofstrDat << "# x_label: " << m_pPlot->axisTitle(QwtPlot::xBottom).text().toStdString() << "\n";
	ofstrDat << "# y_label: " << m_pPlot->axisTitle(QwtPlot::yLeft).text().toStdString() << "\n";
	ofstrDat << "# x_label_2: " << m_pPlot->axisTitle(QwtPlot::xTop).text().toStdString() << "\n";
	ofstrDat << "# y_label_2: " << m_pPlot->axisTitle(QwtPlot::yRight).text().toStdString() << "\n";
	ofstrDat << "\n";

	std::size_t iDataSet=0;
	for(const auto& pairVecs : m_vecDataPtrs)
	{
		if(m_vecDataPtrs.size() > 1)
			ofstrDat << "## -------------------------------- begin of dataset " << (iDataSet+1) 
				<< " --------------------------------\n";

		const std::vector<t_real_qwt>* pVecX = pairVecs.first;
		const std::vector<t_real_qwt>* pVecY = pairVecs.second;

		const std::size_t iSize = std::min(pVecX->size(), pVecY->size());

		for(std::size_t iCur=0; iCur<iSize; ++iCur)
		{
			ofstrDat << std::left << std::setw(g_iPrec*2) << pVecX->operator[](iCur) << " ";
			ofstrDat << std::left << std::setw(g_iPrec*2) << pVecY->operator[](iCur) << "\n";
		}

		if(m_vecDataPtrs.size() > 1)
			ofstrDat << "## -------------------------------- end of dataset " << (iDataSet+1)
				<< " ----------------------------------\n\n";
		++iDataSet;
	}
}

// ----------------------------------------------------------------------------


bool save_table(const char* pcFile, const QTableWidget* pTable)
{
	std::ofstream ofstr(pcFile);
	if(!ofstr)
		return false;

	const int iNumCols = pTable->columnCount();
	const int iNumRows = pTable->rowCount();

	// item lengths
	std::unique_ptr<int[]> ptrMaxTxtLen(new int[iNumCols]);
	for(int iCol=0; iCol<iNumCols; ++iCol)
		ptrMaxTxtLen[iCol] = 0;

	for(int iCol=0; iCol<iNumCols; ++iCol)
	{
		const QTableWidgetItem *pItem = pTable->horizontalHeaderItem(iCol);
		ptrMaxTxtLen[iCol] = std::max(pItem ? pItem->text().length() : 0, ptrMaxTxtLen[iCol]);
	}

	for(int iRow=0; iRow<iNumRows; ++iRow)
		for(int iCol=0; iCol<iNumCols; ++iCol)
		{
			const QTableWidgetItem *pItem = pTable->item(iRow, iCol);
			ptrMaxTxtLen[iCol] = std::max(pItem ? pItem->text().length() : 0, ptrMaxTxtLen[iCol]);
		}


	// write items
	for(int iCol=0; iCol<iNumCols; ++iCol)
	{
		const QTableWidgetItem *pItem = pTable->horizontalHeaderItem(iCol);
		ofstr << std::setw(ptrMaxTxtLen[iCol]+4) << (pItem ? pItem->text().toStdString() : "");
	}
	ofstr << "\n";

	for(int iRow=0; iRow<iNumRows; ++iRow)
	{
		for(int iCol=0; iCol<iNumCols; ++iCol)
		{
			const QTableWidgetItem *pItem = pTable->item(iRow, iCol);
			ofstr << std::setw(ptrMaxTxtLen[iCol]+4) << (pItem ? pItem->text().toStdString() : "");
		}

		ofstr << "\n";
	}

	return true;
}


// ----------------------------------------------------------------------------


void set_zoomer_base(QwtPlotZoomer *pZoomer,
	const std::vector<t_real_qwt>& vecX, const std::vector<t_real_qwt>& vecY,
	bool bMetaCall)
{
	if(!pZoomer)
		return;

	const auto minmaxX = std::minmax_element(vecX.begin(), vecX.end());
	const auto minmaxY = std::minmax_element(vecY.begin(), vecY.end());
	//tl::log_debug("min: ", *minmaxX.first, " ", *minmaxY.first);
	//tl::log_debug("max: ", *minmaxX.second, " ", *minmaxY.second);

	t_real_qwt dminmax[] = {*minmaxX.first, *minmaxX.second,
		*minmaxY.first, *minmaxY.second};

	if(tl::float_equal<t_real_qwt>(dminmax[0], dminmax[1]))
	{
		dminmax[0] -= dminmax[0]/10.;
		dminmax[1] += dminmax[1]/10.;
	}
	if(tl::float_equal<t_real_qwt>(dminmax[2], dminmax[3]))
	{
		dminmax[2] -= dminmax[2]/10.;
		dminmax[3] += dminmax[3]/10.;
	}

	QRectF rect;
	rect.setLeft(dminmax[0]);
	rect.setRight(dminmax[1]);
	rect.setTop(dminmax[2]);
	rect.setBottom(dminmax[3]);

	if(bMetaCall)
	{
		QMetaObject::invokeMethod(pZoomer, "zoom",
			Qt::ConnectionType::DirectConnection,
			Q_ARG(QRectF, rect));
		// unfortunately not a metafunction...
		//QMetaObject::invokeMethod(pZoomer, "setZoomBase", 
		//	Qt::ConnectionType::DirectConnection,
		//	Q_ARG(QRectF, rect));
	}
	else
	{
		pZoomer->zoom(rect);
		pZoomer->setZoomBase(rect);
	}
}

// ----------------------------------------------------------------------------
