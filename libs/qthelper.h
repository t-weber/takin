/**
 * qt helpers
 * @author Tobias Weber
 * @date feb-2016
 * @license GPLv2
 */

#ifndef __QT_HELPER_H__
#define __QT_HELPER_H__

#include <vector>
#include <string>
#include <type_traits>

#include <QTableWidget>
#include <QTableWidgetItem>

#include "tlibs/string/string.h"


extern bool save_table(const char* pcFile, const QTableWidget* pTable);


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


// ----------------------------------------------------------------------------


enum class QtStdPath
{
	HOME,
	FONTS
};

extern std::vector<std::string> get_qt_std_path(QtStdPath path);


#endif
