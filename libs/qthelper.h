/*
 * qt&qwt helpers
 * @author Tobias Weber
 * @date feb-2016
 * @license GPLv2
 */

#ifndef __QT_HELPER_H__
#define __QT_HELPER_H__

#include <vector>
#include <type_traits>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_panner.h>
#include "tlibs/string/string.h"


using t_real_qwt = double;		// qwt's intrinsic value type


class QwtPlotWrapper
{
protected:
	QwtPlot *m_pPlot = nullptr;
	std::vector<QwtPlotCurve*> m_vecCurves;
	QwtPlotGrid *m_pGrid = nullptr;
	QwtPlotPicker *m_pPicker = nullptr;
	QwtPlotZoomer *m_pZoomer = nullptr;
	QwtPlotPanner *m_pPanner = nullptr;

	bool m_bHasDataPtrs = 1;
	std::vector<std::pair<const std::vector<t_real_qwt>*, const std::vector<t_real_qwt>*>> m_vecDataPtrs;

public:
	QwtPlotWrapper(QwtPlot *pPlot, unsigned int iNumCurves=1, bool bNoTrackerSignal=0);
	virtual ~QwtPlotWrapper();

	QwtPlot* GetPlot() { return m_pPlot; }
	QwtPlotCurve* GetCurve(unsigned int iCurve=0) { return m_vecCurves[iCurve]; }
	QwtPlotZoomer* GetZoomer() { return m_pZoomer; }
	QwtPlotPicker* GetPicker() { return m_pPicker; }
	bool HasTrackerSignal() const;

	void SetData(const std::vector<t_real_qwt>& vecX, const std::vector<t_real_qwt>& vecY,
		unsigned int iCurve=0, bool bReplot=1, bool bCopy=0);

	void SavePlot() const;
};


// ----------------------------------------------------------------------------

template<typename t_real, bool bSetDirectly=std::is_same<t_real, t_real_qwt>::value>
struct set_qwt_data
{
	void operator()(QwtPlotWrapper& plot, const std::vector<t_real>& vecX, const std::vector<t_real>& vecY,
		unsigned int iCurve=0, bool bReplot=1) {}
};

// same types -> set data directly
template<typename t_real>
struct set_qwt_data<t_real, 1>
{
	void operator()(QwtPlotWrapper& plot, const std::vector<t_real>& vecX, const std::vector<t_real>& vecY,
		unsigned int iCurve=0, bool bReplot=1)
	{
		plot.SetData(vecX, vecY, iCurve, bReplot, 0);
	}
};

// different types -> copy & convert data first
template<typename t_real>
struct set_qwt_data<t_real, 0>
{
	void operator()(QwtPlotWrapper& plot, const std::vector<t_real>& vecX, const std::vector<t_real>& vecY,
		unsigned int iCurve=0, bool bReplot=1)
	{
		std::vector<t_real_qwt> vecNewX, vecNewY;
		vecNewX.reserve(vecX.size());
		vecNewY.reserve(vecY.size());

		for(t_real d : vecX) vecNewX.push_back(d);
		for(t_real d : vecY) vecNewY.push_back(d);

		plot.SetData(vecNewX, vecNewY, iCurve, bReplot, 1);
	}
};

// ----------------------------------------------------------------------------


extern bool save_table(const char* pcFile, const QTableWidget* pTable);

extern void set_zoomer_base(QwtPlotZoomer *pZoomer,
	const std::vector<t_real_qwt>& vecX, const std::vector<t_real_qwt>& vecY,
	bool bMetaCall=false);


// ----------------------------------------------------------------------------


// a table widget item with a stored numeric value
template<typename T=double>
class QTableWidgetItemWrapper : public QTableWidgetItem
{
protected:
	T m_tVal;
	unsigned int m_iPrec = std::numeric_limits<T>::digits10;

public:
	QTableWidgetItemWrapper() : QTableWidgetItem(), m_tVal()
	{}
	// same value and item text
	QTableWidgetItemWrapper(T tVal)
		: QTableWidgetItem(tl::var_to_str<T>(tVal, std::numeric_limits<T>::digits10).c_str()),
		m_tVal(tVal)
	{}
	// one value, but different text
	QTableWidgetItemWrapper(T tVal, const std::string& strText)
		: QTableWidgetItem(strText.c_str()),
		m_tVal(tVal)
	{}

	virtual ~QTableWidgetItemWrapper() = default;

	unsigned int GetPrec() const { return m_iPrec; }
	void SetPrec(unsigned int iPrec) { m_iPrec = iPrec; }

	T GetValue() const { return m_tVal; }

	// same value and item text
	void SetValue(T val)
	{
		m_tVal = val;
		this->setText(tl::var_to_str<T>(m_tVal, m_iPrec).c_str());
	}
	// one value, but different text
	void SetValue(T val, const std::string& str)
	{
		m_tVal = val;
		this->setText(str.c_str());
	}
	void SetValueKeepText(T val)
	{
		m_tVal = val;
	}

	virtual bool operator<(const QTableWidgetItem& item) const override
	{
		const QTableWidgetItemWrapper<T>* pItem =
			dynamic_cast<const QTableWidgetItemWrapper<T>*>(&item);
		if(!pItem) return 0;

		return this->GetValue() < pItem->GetValue();
	}
};


#endif
