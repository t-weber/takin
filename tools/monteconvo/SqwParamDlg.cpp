/**
 * S(q,w) parameters dialog
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date aug-2015
 * @license GPLv2
 */

#include "SqwParamDlg.h"
#include <QMessageBox>
#include <QCheckBox>


enum
{
	SQW_NAME = 0,
	SQW_TYPE = 1,
	SQW_VAL = 2,

	SQW_ERR = 3,
	SQW_FIT = 4
};

SqwParamDlg::SqwParamDlg(QWidget* pParent, QSettings* pSett)
	: QDialog(pParent), m_pSett(pSett)
{
	setupUi(this);
	if(m_pSett)
	{
		QFont font;
		if(m_pSett->contains("main/font_gen") && font.fromString(m_pSett->value("main/font_gen", "").toString()))
			setFont(font);
	}

	tableParams->verticalHeader()->setDefaultSectionSize(tableParams->verticalHeader()->minimumSectionSize()+2);

	connect(buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(ButtonBoxClicked(QAbstractButton*)));


	if(m_pSett && m_pSett->contains("monteconvo/param_geo"))
		restoreGeometry(m_pSett->value("monteconvo/param_geo").toByteArray());
}

SqwParamDlg::~SqwParamDlg()
{}


void SqwParamDlg::SqwLoaded(const std::vector<SqwBase::t_var>& vecVars,
	const std::vector<SqwBase::t_var_fit>* pvecFit)
{
	const bool bSortTable = tableParams->isSortingEnabled();
	tableParams->setSortingEnabled(0);

	tableParams->setRowCount(vecVars.size());
	tableParams->setColumnWidth(SQW_NAME, 125);
	tableParams->setColumnWidth(SQW_TYPE, 75);
	tableParams->setColumnWidth(SQW_VAL, 175);
	tableParams->setColumnWidth(SQW_ERR, 100);
	tableParams->setColumnWidth(SQW_FIT, 50);

	int iRow=0;
	for(const SqwBase::t_var& var : vecVars)
	{
		const std::string& strName = std::get<SQW_NAME>(var);
		const std::string& strType = std::get<SQW_TYPE>(var);
		const std::string& strVal = std::get<SQW_VAL>(var);
		const std::string* pstrErr = nullptr;
		bool bFit = 0;

		// look for associated fit parameters, if given
		if(pvecFit)
		{
			// match with basic variable ident
			auto iterFit = std::find_if(pvecFit->begin(), pvecFit->end(),
				[&strName](const SqwBase::t_var_fit& varFit) -> bool
				{ return strName == std::get<0>(varFit); });

			if(iterFit != pvecFit->end())
			{
				pstrErr = &std::get<1>(*iterFit);	// error
				bFit = std::get<2>(*iterFit);		// "is fit param" flag
			}
		}


		// name
		QTableWidgetItem *pItemName = tableParams->item(iRow, SQW_NAME);
		if(!pItemName)
		{
			pItemName = new QTableWidgetItem();
			tableParams->setItem(iRow, SQW_NAME, pItemName);
		}
		pItemName->setFlags((pItemName->flags() & ~Qt::ItemIsEditable) | Qt::ItemIsUserCheckable);
		pItemName->setCheckState(Qt::Unchecked);
		pItemName->setText(strName.c_str());


		// type
		QTableWidgetItem *pItemType = tableParams->item(iRow, SQW_TYPE);
		if(!pItemType)
		{
			pItemType = new QTableWidgetItem();
			tableParams->setItem(iRow, SQW_TYPE, pItemType);
		}
		pItemType->setFlags(pItemType->flags() & ~Qt::ItemIsEditable);
		pItemType->setText(strType.c_str());


		// value
		QTableWidgetItem *pItemVal = tableParams->item(iRow, SQW_VAL);
		if(!pItemVal)
		{
			pItemVal = new QTableWidgetItem();
			tableParams->setItem(iRow, SQW_VAL, pItemVal);
		}
		pItemVal->setFlags(pItemVal->flags() | Qt::ItemIsEditable);
		pItemVal->setText(strVal.c_str());


		// error
		QTableWidgetItem *pItemErr = tableParams->item(iRow, SQW_ERR);
		if(!pItemErr)
		{
			pItemErr = new QTableWidgetItem();
			tableParams->setItem(iRow, SQW_ERR, pItemErr);
		}
		pItemErr->setFlags(pItemErr->flags() | Qt::ItemIsEditable);
		pItemErr->setText(pstrErr ? pstrErr->c_str() : "0");


		// is fit param?
		QCheckBox *pItemFit = reinterpret_cast<QCheckBox*>(tableParams->cellWidget(iRow, SQW_FIT));
		if(!pItemFit)
		{
			pItemFit = new QCheckBox();
			tableParams->setCellWidget(iRow, SQW_FIT, pItemFit);
		}
		pItemFit->setChecked(bFit);


		++iRow;
	}

	tableParams->setSortingEnabled(bSortTable);
}

void SqwParamDlg::SaveSqwParams()
{
	std::vector<SqwBase::t_var> vecVars;
	std::vector<SqwBase::t_var_fit> vecVarsFit;
	bool bAnythingSelected=0;

	for(int iRow=0; iRow<tableParams->rowCount(); ++iRow)
	{
		if(tableParams->item(iRow, SQW_NAME)->checkState() != Qt::Checked)
			continue;

		// basic vars
		SqwBase::t_var var;
		std::get<SQW_NAME>(var) = tableParams->item(iRow, SQW_NAME)->text().toStdString();
		std::get<SQW_TYPE>(var) = tableParams->item(iRow, SQW_TYPE)->text().toStdString();
		std::get<SQW_VAL>(var) = tableParams->item(iRow, SQW_VAL)->text().toStdString();

		// fit vars
		SqwBase::t_var_fit varFit;
		std::get<0>(varFit) = std::get<SQW_NAME>(var);
		std::get<1>(varFit) = tableParams->item(iRow, SQW_ERR)->text().toStdString();
		std::get<2>(varFit) = reinterpret_cast<QCheckBox*>(
			tableParams->cellWidget(iRow, SQW_FIT))->isChecked();

		vecVars.push_back(std::move(var));
		vecVarsFit.push_back(std::move(varFit));
		bAnythingSelected = 1;
	}

	if(bAnythingSelected)
		emit SqwParamsChanged(vecVars, &vecVarsFit);
	else
		QMessageBox::warning(this, "Warning", "No variable is selected for update.");
}



void SqwParamDlg::showEvent(QShowEvent *pEvt)
{
	QDialog::showEvent(pEvt);
}

void SqwParamDlg::ButtonBoxClicked(QAbstractButton *pBtn)
{
	if(buttonBox->buttonRole(pBtn) == QDialogButtonBox::ApplyRole ||
	   buttonBox->buttonRole(pBtn) == QDialogButtonBox::AcceptRole)
	{
		SaveSqwParams();
	}
	else if(buttonBox->buttonRole(pBtn) == QDialogButtonBox::RejectRole)
		QDialog::reject();

	if(buttonBox->buttonRole(pBtn) == QDialogButtonBox::AcceptRole)
	{
		if(m_pSett)
			m_pSett->setValue("monteconvo/param_geo", saveGeometry());

		QDialog::accept();
	}
}


#include "SqwParamDlg.moc"
