/**
 * qwt helpers
 * @author Tobias Weber
 * @date feb-2016
 * @license GPLv2
 */

#include "qwthelper.h"
#include "globals.h"
#include "globals_qt.h"
#include "version.h"

#include "tlibs/math/math.h"
#include "tlibs/string/string.h"
#include "tlibs/helper/misc.h"
#include "tlibs/time/chrono.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <memory>

#include <qwt_picker_machine.h>
#include <qwt_plot_canvas.h>
#include <qwt_curve_fitter.h>
#include <qwt_scale_widget.h>

#include <QFileDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QCoreApplication>


class MyQwtPlotZoomer : public QwtPlotZoomer
{ /*Q_OBJECT*/
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

/*public slots:
	virtual void setZoomBase(const QRectF& rect) override
	{
		QwtPlotZoomer::setZoomBase(rect);
	}*/
};


// ----------------------------------------------------------------------------


class MyQwtPlotPicker : public QwtPlotPicker
{
protected:
	QwtPlotWrapper *m_pPlotWrap = nullptr;

	virtual void widgetKeyPressEvent(QKeyEvent *pEvt) override
	{
		const int iKey = pEvt->key();
		if(iKey == Qt::Key_S)
			m_pPlotWrap->SavePlot();
		else if(iKey == Qt::Key_G)
			m_pPlotWrap->ExportGpl();
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
	unsigned int iNumCurves, bool bNoTrackerSignal, bool bUseSpline, bool bUseSpectrogram)
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

	if(bUseSpectrogram)
	{
		m_pSpec = new QwtPlotSpectrogram();
		m_pSpec->setRenderHint(QwtPlotItem::RenderAntialiased, true);
		m_pSpec->setDisplayMode(QwtPlotSpectrogram::ImageMode, 1);

		auto create_colmap = []() -> QwtLinearColorMap*
		{
			QwtLinearColorMap *pCol = new QwtLinearColorMap();
			pCol->setColorInterval(QColor(0x00, 0x00, 0xff), QColor(0xff, 0x00, 0x00));
			return pCol;
		};

		t_real_qwt dCBMin=0., dCBMax=1.;
		m_pPlot->enableAxis(QwtPlot::yRight);
		QwtScaleWidget *pRightAxis = m_pPlot->axisWidget(QwtPlot::yRight);
		pRightAxis->setColorBarEnabled(1);
		m_pPlot->setAxisScale(QwtPlot::yRight, dCBMin, dCBMax);

#if QWT_VER>=6
		m_pSpec->setColorMap(create_colmap());
		pRightAxis->setColorMap(QwtInterval(dCBMin, dCBMax), create_colmap());
#else
		QwtLinearColorMap *pColMap = create_colmap();
		m_pSpec->setColorMap(*create_colmap());
		pRightAxis->setColorMap(QwtInterval(dCBMin, dCBMax), *create_colmap());
		//delete pColMap;
#endif

		m_pRaster = new MyQwtRasterData();
#if QWT_VER>=6
		m_pSpec->setData(m_pRaster);
#else
		m_pSpec->setData(*m_pRaster);
#endif

		m_pSpec->attach(m_pPlot);
	}
	else
	{
		m_vecCurves.reserve(iNumCurves);
		for(unsigned int iCurve=0; iCurve<iNumCurves; ++iCurve)
		{
			QwtPlotCurve* pCurve = new QwtPlotCurve();
			pCurve->setPen(penCurve);
			pCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);

			if(bUseSpline)
			{
				pCurve->setCurveFitter(new QwtSplineCurveFitter());
				((QwtSplineCurveFitter*)pCurve->curveFitter())->setFitMode(
					QwtSplineCurveFitter::/*Parametric*/Spline);
				pCurve->setCurveAttribute(QwtPlotCurve::Fitted);
			}

			pCurve->attach(m_pPlot);
			m_vecCurves.push_back(pCurve);
		}

		m_pGrid = new QwtPlotGrid();
		m_pGrid->setPen(penGrid);
		m_pGrid->attach(m_pPlot);
	}

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
	if(m_pGrid) { delete m_pGrid; m_pGrid = nullptr; }
	if(m_pZoomer) { delete m_pZoomer; m_pZoomer = nullptr; }
	if(m_pPanner) { delete m_pPanner; m_pPanner = nullptr; }

	for(QwtPlotCurve* pCurve : m_vecCurves)
		if(pCurve) delete pCurve;
	m_vecCurves.clear();

	if(m_pSpec) { delete m_pSpec; m_pSpec = nullptr; }
}


#if QWT_VER>=6
	bool QwtPlotWrapper::HasTrackerSignal() const { return 1; }
#else
	bool QwtPlotWrapper::HasTrackerSignal() const { return 0; }
#endif


void QwtPlotWrapper::SetData(const std::vector<t_real_qwt>& vecX, const std::vector<t_real_qwt>& vecY,
	unsigned int iCurve, bool bReplot, bool bCopy, const std::vector<t_real_qwt>* pvecYErr)
{
	std::lock_guard<decltype(m_mutex)> lock(m_mutex);

	if(!bCopy)	// copy pointers
	{
#if QWT_VER>=6
		m_vecCurves[iCurve]->setRawSamples(vecX.data(), vecY.data(), std::min(vecX.size(), vecY.size()));
#else
		m_vecCurves[iCurve]->setRawData(vecX.data(), vecY.data(), std::min(vecX.size(), vecY.size()));
#endif

		m_bHasDataPtrs = 1;
		if(iCurve >= m_vecDataPtrs.size())
			m_vecDataPtrs.resize(iCurve+1);

		std::get<0>(m_vecDataPtrs[iCurve]) = &vecX;
		std::get<1>(m_vecDataPtrs[iCurve]) = &vecY;
		std::get<2>(m_vecDataPtrs[iCurve]) = pvecYErr;
	}
	else		// copy data, TODO: errorbars
	{
#if QWT_VER>=6
		m_vecCurves[iCurve]->setSamples(vecX.data(), vecY.data(), std::min(vecX.size(), vecY.size()));
#else
		m_vecCurves[iCurve]->setData(vecX.data(), vecY.data(), std::min(vecX.size(), vecY.size()));
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
	if(!m_bHasDataPtrs && !m_pRaster)
	{
		// if data was deep-copied, it is now lost somewhere in the plot objects...
		QMessageBox::critical(m_pPlot, "Error", "Cannot get plot data.");
		return;
	}

	std::lock_guard<decltype(m_mutex)> lock(m_mutex);
	std::string strFile = QFileDialog::getSaveFileName(m_pPlot,
		"Save Plot Data", nullptr, "Data files (*.dat *.DAT)",
		nullptr).toStdString();

	tl::trim(strFile);
	if(strFile == "") return;

	std::ofstream ofstrDat(strFile);
	if(!ofstrDat)
	{
		std::string strErr = "Cannot open file \"" + strFile + "\" for saving.";
		QMessageBox::critical(m_pPlot, "Error", strErr.c_str());
		return;
	}

	t_real_qwt dEpoch = tl::epoch<t_real_qwt>();

	ofstrDat.precision(g_iPrec);
	ofstrDat << "#\n";
	ofstrDat << "# comment: Created with Takin version " << TAKIN_VER << ".\n";
	ofstrDat << "# timestamp: " << tl::var_to_str<t_real_qwt>(dEpoch)
		<< " (" << tl::epoch_to_str<t_real_qwt>(dEpoch) << ").\n";
	ofstrDat << "# title: " << m_pPlot->title().text().toStdString() << "\n";
	ofstrDat << "# x_label: " << m_pPlot->axisTitle(QwtPlot::xBottom).text().toStdString() << "\n";
	ofstrDat << "# y_label: " << m_pPlot->axisTitle(QwtPlot::yLeft).text().toStdString() << "\n";
	ofstrDat << "# x_label_2: " << m_pPlot->axisTitle(QwtPlot::xTop).text().toStdString() << "\n";
	ofstrDat << "# y_label_2: " << m_pPlot->axisTitle(QwtPlot::yRight).text().toStdString() << "\n";

	if(m_pRaster)	// 2d plot
	{
		ofstrDat << "# x_size: " << m_pRaster->GetWidth() << "\n";
		ofstrDat << "# y_size: " << m_pRaster->GetHeight() << "\n";
		ofstrDat << "# x_range: " << m_pRaster->GetXMin() << " " << m_pRaster->GetXMax() << "\n";
		ofstrDat << "# y_range: " << m_pRaster->GetYMin() << " " << m_pRaster->GetYMax() << "\n";
		ofstrDat << "# z_range: " << m_pRaster->GetZMin() << " " << m_pRaster->GetZMax() << "\n";
		ofstrDat << "#\n";

		for(std::size_t iY=0; iY<m_pRaster->GetHeight(); ++iY)
		{
			for(std::size_t iX=0; iX<m_pRaster->GetWidth(); ++iX)
			{
				ofstrDat << std::left << std::setw(g_iPrec*2)
					<< m_pRaster->GetPixel(iX, iY) << " ";
			}
			ofstrDat << "\n";
		}
	}
	else			// 1d plot(s)
	{
		ofstrDat << "#\n";

		std::size_t iDataSet=0;
		for(const auto& pairVecs : m_vecDataPtrs)
		{
			const std::vector<t_real_qwt>* pVecX = std::get<0>(pairVecs);
			const std::vector<t_real_qwt>* pVecY = std::get<1>(pairVecs);
			const std::vector<t_real_qwt>* pVecYErr = std::get<2>(pairVecs);

			const std::size_t iSize = std::min(pVecX->size(), pVecY->size());
			// if no data points are available, skip the curve
			if(!iSize) continue;


			if(m_vecDataPtrs.size() > 1)
				ofstrDat << "## -------------------------------- begin of dataset " << (iDataSet+1)
					<< " --------------------------------\n";

			for(std::size_t iCur=0; iCur<iSize; ++iCur)
			{
				ofstrDat << std::left << std::setw(g_iPrec*2)
					<< pVecX->operator[](iCur) << " ";
				ofstrDat << std::left << std::setw(g_iPrec*2)
					<< pVecY->operator[](iCur) << " ";
				if(pVecYErr)
				{
					ofstrDat << std::left << std::setw(g_iPrec*2)
						<< pVecYErr->operator[](iCur) << " ";
				}
				ofstrDat << "\n";
			}

			if(m_vecDataPtrs.size() > 1)
				ofstrDat << "## -------------------------------- end of dataset " << (iDataSet+1)
					<< " ----------------------------------\n\n";
			++iDataSet;
		}
	}
}

void QwtPlotWrapper::ExportGpl() const
{
	if(!m_bHasDataPtrs && !m_pRaster)
	{
		// if data was deep-copied, it is now lost somewhere in the plot objects...
		QMessageBox::critical(m_pPlot, "Error", "Cannot get plot data.");
		return;
	}

	std::lock_guard<decltype(m_mutex)> lock(m_mutex);
	std::string strFile = QFileDialog::getSaveFileName(m_pPlot,
		"Export as Gnuplot Script", nullptr, "Gnuplot scripts (*.gpl *.GPL)",
		nullptr).toStdString();

	tl::trim(strFile);
	if(strFile == "") return;

	std::ofstream ofstrDat(strFile);
	if(!ofstrDat)
	{
		std::string strErr = "Cannot open file \"" + strFile + "\" for saving.";
		QMessageBox::critical(m_pPlot, "Error", strErr.c_str());
		return;
	}

	t_real_qwt dEpoch = tl::epoch<t_real_qwt>();

	ofstrDat.precision(g_iPrec);
	ofstrDat << "#\n";
	ofstrDat << "# comment: Created with Takin version " << TAKIN_VER << ".\n";
	ofstrDat << "# timestamp: " << tl::var_to_str<t_real_qwt>(dEpoch)
		<< " (" << tl::epoch_to_str<t_real_qwt>(dEpoch) << ").\n";
	ofstrDat << "#\n";
	ofstrDat << "\n";

	ofstrDat << "set title \"" << m_pPlot->title().text().toStdString() << "\"\n";
	ofstrDat << "set xlabel \"" << m_pPlot->axisTitle(QwtPlot::xBottom).text().toStdString() << "\"\n";
	ofstrDat << "set ylabel \"" << m_pPlot->axisTitle(QwtPlot::yLeft).text().toStdString() << "\"\n";
	ofstrDat << "set x2label \"" << m_pPlot->axisTitle(QwtPlot::xTop).text().toStdString() << "\"\n";
	ofstrDat << "set y2label \"" << m_pPlot->axisTitle(QwtPlot::yRight).text().toStdString() << "\"\n";
	ofstrDat << "\n";

	if(m_pRaster)	// 2d plot
	{
		std::size_t iWidth = m_pRaster->GetWidth();
		std::size_t iHeight = m_pRaster->GetHeight();

		t_real_qwt dXMin = m_pRaster->GetXMin();
		t_real_qwt dXMax = m_pRaster->GetXMax();
		t_real_qwt dYMin = m_pRaster->GetYMin();
		t_real_qwt dYMax = m_pRaster->GetYMax();
		t_real_qwt dZMin = m_pRaster->GetZMin();
		t_real_qwt dZMax = m_pRaster->GetZMax();

		t_real_qwt dXScale = (dXMax-dXMin)/t_real_qwt(iWidth);
		t_real_qwt dYScale = (dYMax-dYMin)/t_real_qwt(iHeight);

		ofstrDat << "xscale = " << dXScale << "\n";
		ofstrDat << "yscale = " << dYScale << "\n";
		ofstrDat << "xmin = " << dXMin+dXScale*0.5 << "\n";
		ofstrDat << "xmax = " << dXMax+dXScale*0.5 << "\n";
		ofstrDat << "ymin = " << dYMin+dYScale*0.5 << "\n";
		ofstrDat << "ymax = " << dYMax+dYScale*0.5 << "\n";
		ofstrDat << "zmin = " << dZMin << "\n";
		ofstrDat << "zmax = " << dZMax << "\n";
		ofstrDat << "zscale = 1.\n";
		ofstrDat << "\n";

		ofstrDat << "set xrange [xmin-xscale*0.5 : xmax-xscale*0.5]\n";
		ofstrDat << "set yrange [ymin-yscale*0.5 : ymax-yscale*0.5]\n";
		ofstrDat << "set cbrange [zmin : zmax]\n";
		ofstrDat << "\n";

		ofstrDat << "set palette defined (0 \"#0000ff\", 1 \"#ff0000\")\n";
		ofstrDat << "set tics out scale 0.75\n";
		ofstrDat << "unset key\n";
		ofstrDat << "set size ratio 1\n";
		ofstrDat << "\n";

		ofstrDat << "plot \"-\" "
			<< "using (xmin + $1*xscale) : (ymin + $2*yscale) : ($3*zscale) "
			<< "matrix with image\n";

		for(std::size_t iY=0; iY<m_pRaster->GetHeight(); ++iY)
		{
			for(std::size_t iX=0; iX<m_pRaster->GetWidth(); ++iX)
			{
				ofstrDat << std::left << std::setw(g_iPrec*2)
					<< m_pRaster->GetPixel(iX, iY) << " ";
			}
			ofstrDat << "\n";
		}

		ofstrDat << "end\n";
	}
	else			// 1d plot(s)
	{
		ofstrDat << "scale = 1.\n";
		ofstrDat << "plot \\\n";

		const std::size_t iNumCurves = m_vecDataPtrs.size();
		for(std::size_t iCurve=0; iCurve<iNumCurves; ++iCurve)
		{
			// if no data points are available, skip the curve
			const std::vector<t_real_qwt>* pVecX = std::get<0>(m_vecDataPtrs[iCurve]);
			const std::vector<t_real_qwt>* pVecY = std::get<1>(m_vecDataPtrs[iCurve]);
			const std::size_t iSize = std::min(pVecX->size(), pVecY->size());
			if(!iSize) continue;

			// points or lines?
			if(m_vecCurves[iCurve]->style() /*& QwtPlotCurve::CurveStyle::Lines*/
				== QwtPlotCurve::CurveStyle::Lines)
			{
				ofstrDat << "\t\"-\" using ($1):($2*scale) with lines title \"dataset "
					<< (iCurve+1) << "\"";
			}
			else
			{
				if(std::get<2>(m_vecDataPtrs[iCurve]))	// with errorbars?
				{
					ofstrDat << "\t\"-\" using ($1):($2*scale):($3*scale) "
						<< "with yerrorbars pointtype 7 title \"dataset "
						<< (iCurve+1) << "\"";
				}
				else	// without errorbars
				{
					ofstrDat << "\t\"-\" using ($1):($2*scale) "
						<< "with points pointtype 7 title \"dataset "
						<< (iCurve+1) << "\"";
				}
			}

			// TODO: doesn't work if following curves are defined and empty
			if(iCurve < iNumCurves-1) ofstrDat << ", \\";
			ofstrDat << "\n";
		}

		for(const auto& pairVecs : m_vecDataPtrs)
		{
			const std::vector<t_real_qwt>* pVecX = std::get<0>(pairVecs);
			const std::vector<t_real_qwt>* pVecY = std::get<1>(pairVecs);
			const std::vector<t_real_qwt>* pVecYErr = std::get<2>(pairVecs);

			// if no data points are available, skip the curve
			const std::size_t iSize = std::min(pVecX->size(), pVecY->size());
			if(!iSize) continue;

			for(std::size_t iCur=0; iCur<iSize; ++iCur)
			{
				ofstrDat << std::left << std::setw(g_iPrec*2)
					<< pVecX->operator[](iCur) << " ";
				ofstrDat << std::left << std::setw(g_iPrec*2)
					<< pVecY->operator[](iCur) << " ";
				if(pVecYErr)
				{
					ofstrDat << std::left << std::setw(g_iPrec*2)
						<< pVecYErr->operator[](iCur) << " ";
				}
				ofstrDat << "\n";
			}

			ofstrDat << "end\n";
		}
	}
}

void QwtPlotWrapper::setAxisTitle(int iAxis, const QString& str)
{
	if(!m_pPlot) return;
	std::lock_guard<decltype(m_mutex)> lock(m_mutex);

	m_pPlot->setAxisTitle(iAxis, str);
}

void QwtPlotWrapper::setZoomBase(const QRectF& rect)
{
	if(!m_pZoomer) return;
	std::lock_guard<decltype(m_mutex)> lock(m_mutex);

	m_pZoomer->setZoomBase(rect);
}

void QwtPlotWrapper::scaleColorBar()
{
	if(!m_pPlot || !m_pRaster) return;
	std::lock_guard<decltype(m_mutex)> lock(m_mutex);

	t_real_qwt dMin = m_pRaster->GetZMin();
	t_real_qwt dMax = m_pRaster->GetZMax();

	QwtScaleWidget *pRightAxis = m_pPlot->axisWidget(QwtPlot::yRight);
#if QWT_VER>=6
	pRightAxis->setColorMap(QwtInterval(dMin, dMax), (QwtColorMap*)pRightAxis->colorMap());
#else
	pRightAxis->setColorMap(QwtInterval(dMin, dMax), *pRightAxis->colorMap().copy());
#endif
	m_pPlot->setAxisScale(QwtPlot::yRight, dMin, dMax);
}

void QwtPlotWrapper::doUpdate()
{
	if(!m_pPlot) return;
	std::lock_guard<decltype(m_mutex)> lock(m_mutex);

#if QWT_VER<6
	if(m_pSpec && m_pRaster)
	{
		m_pSpec->setData(*m_pRaster);
		m_pPlot->setAxisScale(QwtPlot::xBottom, m_pRaster->GetXMin(), m_pRaster->GetXMax());
		m_pPlot->setAxisScale(QwtPlot::yLeft, m_pRaster->GetYMin(), m_pRaster->GetYMax());
	}
#endif

	m_pPlot->replot();

	// flush pending "QMetaObject::invokeMethod" calls from the event loop
	QCoreApplication::processEvents(QEventLoop::AllEvents);
}

// ----------------------------------------------------------------------------


void MyQwtRasterData::SetXRange(t_real_qwt dMin, t_real_qwt dMax)
{
	t_real_qwt dOffs = 0.5*(dMax-dMin)/t_real_qwt(m_iW);
	dMin -= dOffs; dMax -= dOffs;

	m_dXRange[0] = dMin; m_dXRange[1] = dMax;
#if QWT_VER>=6
	setInterval(Qt::XAxis, QwtInterval(dMin, dMax, QwtInterval::ExcludeMaximum));
#endif
}

void MyQwtRasterData::SetYRange(t_real_qwt dMin, t_real_qwt dMax)
{
	t_real_qwt dOffs = 0.5*(dMax-dMin)/t_real_qwt(m_iH);
	dMin -= dOffs; dMax -= dOffs;

	m_dYRange[0] = dMin; m_dYRange[1] = dMax;
#if QWT_VER>=6
	setInterval(Qt::YAxis, QwtInterval(dMin, dMax, QwtInterval::ExcludeMaximum));
#endif
}

void MyQwtRasterData::SetZRange(t_real_qwt dMin, t_real_qwt dMax)
{
	m_dZRange[0] = dMin; m_dZRange[1] = dMax;
#if QWT_VER>=6
	setInterval(Qt::ZAxis, QwtInterval(dMin, dMax));
#endif
}

void MyQwtRasterData::SetZRange()	// automatically determined range
{
	if(!m_pData) return;

	auto minmax = std::minmax_element(m_pData.get(), m_pData.get()+m_iW*m_iH);
	m_dZRange[0] = *minmax.first; m_dZRange[1] = *minmax.second;
#if QWT_VER>=6
	setInterval(Qt::ZAxis, QwtInterval(m_dZRange[0], m_dZRange[1]));
#endif
}


t_real_qwt MyQwtRasterData::value(t_real_qwt dx, t_real_qwt dy) const
{
	t_real_qwt dXMin = std::min(m_dXRange[0], m_dXRange[1]);
	t_real_qwt dXMax = std::max(m_dXRange[0], m_dXRange[1]);
	t_real_qwt dYMin = std::min(m_dYRange[0], m_dYRange[1]);
	t_real_qwt dYMax = std::max(m_dYRange[0], m_dYRange[1]);

	if(dx<dXMin || dy<dYMin || dx>=dXMax || dy>=dYMax)
		return t_real_qwt(0);

	std::size_t iX = tl::tic_trafo_inv(m_iW, m_dXRange[0], m_dXRange[1], 0, dx);
	std::size_t iY = tl::tic_trafo_inv(m_iH, m_dYRange[0], m_dYRange[1], 0, dy);

	return GetPixel(iX, iY);
}

MyQwtRasterData::~MyQwtRasterData()
{}

MyQwtRasterData::MyQwtRasterData(const MyQwtRasterData& dat)
{
	this->operator=(dat);
}

/**
 * shallow copy
 */
const MyQwtRasterData& MyQwtRasterData::operator=(const MyQwtRasterData& dat)
{
	m_pData = dat.m_pData;
	m_iW = dat.m_iW;
	m_iH = dat.m_iH;

	m_dXRange[0] = dat.m_dXRange[0]; m_dXRange[1] = dat.m_dXRange[1];
	m_dYRange[0] = dat.m_dYRange[0]; m_dYRange[1] = dat.m_dYRange[1];
	m_dZRange[0] = dat.m_dZRange[0]; m_dZRange[1] = dat.m_dZRange[1];

	return *this;
}

/**
 * shallow copy
 */
QwtRasterData* MyQwtRasterData::copy() const
{
	return new MyQwtRasterData(*this);
}

/**
 * deep copy
 */
QwtRasterData* MyQwtRasterData::clone() const
{
	MyQwtRasterData* pDat = new MyQwtRasterData(m_iW, m_iH);
	std::copy(m_pData.get(), m_pData.get()+m_iW*m_iH, pDat->m_pData.get());

	pDat->m_dXRange[0] = m_dXRange[0]; pDat->m_dXRange[1] = m_dXRange[1];
	pDat->m_dYRange[0] = m_dYRange[0]; pDat->m_dYRange[1] = m_dYRange[1];
	pDat->m_dZRange[0] = m_dZRange[0]; pDat->m_dZRange[1] = m_dZRange[1];

	return pDat;
}

QwtInterval MyQwtRasterData::range() const
{
	return QwtInterval(GetZMin(), GetZMax());
}

// ----------------------------------------------------------------------------


void set_zoomer_base(QwtPlotZoomer *pZoomer,
	t_real_qwt dL, t_real_qwt dR, t_real_qwt dT, t_real_qwt dB,
	bool bMetaCall, QwtPlotWrapper* pPlotWrap)
{
	if(!pZoomer) return;

	QRectF rect;
	rect.setLeft(dL);	rect.setRight(dR);
	rect.setTop(dT);	rect.setBottom(dB);

	if(bMetaCall)
	{
		QMetaObject::invokeMethod(pZoomer, "zoom",
			Qt::ConnectionType::BlockingQueuedConnection,
			Q_ARG(QRectF, rect));

		if(pPlotWrap)
		{
			// use auxilliary slot in QwtPlotWrapper
			// since setZoomBase is not a slot in QwtPlotZoomer
			QMetaObject::invokeMethod(pPlotWrap, "setZoomBase",
				Qt::ConnectionType::BlockingQueuedConnection,
				Q_ARG(const QRectF&, rect));
		}
	}
	else
	{
		pZoomer->zoom(rect);
		pZoomer->setZoomBase(rect);
	}
}


/**
 * calculate zoom rectangle from x and y data points
 */
void set_zoomer_base(QwtPlotZoomer *pZoomer,
	const std::vector<t_real_qwt>& vecX, const std::vector<t_real_qwt>& vecY,
	bool bMetaCall, QwtPlotWrapper* pPlotWrap)
{
	if(!pZoomer || !vecX.size() || !vecY.size())
		return;

	const auto minmaxX = std::minmax_element(vecX.begin(), vecX.end());
	const auto minmaxY = std::minmax_element(vecY.begin(), vecY.end());

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

	set_zoomer_base(pZoomer,
		dminmax[0],dminmax[1],dminmax[2],dminmax[3],
		bMetaCall, pPlotWrap);
}


/**
 * calculate zoom rectangle from sets of x and y data points
 */
void set_zoomer_base(QwtPlotZoomer *pZoomer,
	const std::vector<std::vector<t_real_qwt>>& vecvecX, 
	const std::vector<std::vector<t_real_qwt>>& vecvecY,
	bool bMetaCall, QwtPlotWrapper* pPlotWrap)
{
	if(!pZoomer || !vecvecX.size() || !vecvecY.size())
		return;

	std::vector<t_real_qwt> vecX, vecY;
	for(std::size_t iVec=0; iVec<vecvecX.size(); ++iVec)
	{
		vecX.insert(vecX.end(), vecvecX[iVec].begin(), vecvecX[iVec].end());
		vecY.insert(vecY.end(), vecvecY[iVec].begin(), vecvecY[iVec].end());
	}

	set_zoomer_base(pZoomer, vecX, vecY, bMetaCall, pPlotWrap);
}

/**
 * calculate zoom rectangle from x and a set of y data points
 */
void set_zoomer_base(QwtPlotZoomer *pZoomer,
	const std::vector<t_real_qwt>& vecX, 
	const std::vector<std::vector<t_real_qwt>>& vecvecY,
	bool bMetaCall, QwtPlotWrapper* pPlotWrap)
{
	if(!pZoomer || !vecX.size() || !vecvecY.size())
		return;

	std::vector<t_real_qwt> vecY;
	for(std::size_t iVec=0; iVec<vecvecY.size(); ++iVec)
		vecY.insert(vecY.end(), vecvecY[iVec].begin(), vecvecY[iVec].end());

	set_zoomer_base(pZoomer, vecX, vecY, bMetaCall, pPlotWrap);
}


#include "qwthelper.moc"
