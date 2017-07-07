/**
 * Dead Angles Dialog
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date jun-2017
 * @license GPLv2
 */

#include "DeadAnglesDlg.h"
#include <QComboBox>
#include "tlibs/string/string.h"


using t_real = t_real_glob;


enum class AngleInfo : int
{
	START_ANGLE = 0,
	STOP_ANGLE = 1,
	OFFSET_ANGLE = 2,

	CENTRE = 3,
	RELATIVE = 4,
};


DeadAnglesDlg::DeadAnglesDlg(QWidget* pParent, QSettings *pSettings)
	: QDialog(pParent), m_pSettings(pSettings)
{
	setupUi(this);
	if(m_pSettings)
	{
		QFont font;
		if(m_pSettings->contains("main/font_gen") && font.fromString(m_pSettings->value("main/font_gen", "").toString()))
			setFont(font);
	}

	btnAdd->setIcon(load_icon("res/icons/list-add.svg"));
	btnDel->setIcon(load_icon("res/icons/list-remove.svg"));

#if QT_VER >= 5
	QObject::connect(btnAdd, &QAbstractButton::clicked, this, &DeadAnglesDlg::AddAngle);
	QObject::connect(btnDel, &QAbstractButton::clicked, this, &DeadAnglesDlg::RemoveAngle);
	QObject::connect(buttonBox, &QDialogButtonBox::clicked, this, &DeadAnglesDlg::ButtonBoxClicked);
#else
	QObject::connect(btnAdd, SIGNAL(clicked(bool)), this, SLOT(AddAngle()));
	QObject::connect(btnDel, SIGNAL(clicked(bool)), this, SLOT(RemoveAngle()));
	QObject::connect(buttonBox, SIGNAL(clicked(QAbstractButton*)), this,
		SLOT(ButtonBoxClicked(QAbstractButton*)));
#endif

	if(m_pSettings && m_pSettings->contains("deadangles/geo"))
		restoreGeometry(m_pSettings->value("deadangles/geo").toByteArray());
}


/**
 * removes the currently selected items from the dead angles list
 */
void DeadAnglesDlg::RemoveAngle()
{
	const bool bSort = tableAngles->isSortingEnabled();
	tableAngles->setSortingEnabled(0);

	bool bNothingRemoved = 1;

	// remove selected rows
	QList<QTableWidgetSelectionRange> lstSel = tableAngles->selectedRanges();
	for(QTableWidgetSelectionRange& range : lstSel)
	{
		for(int iRow=range.bottomRow(); iRow>=range.topRow(); --iRow)
		{
			tableAngles->removeRow(iRow);
			bNothingRemoved = 0;
		}
	}

	// remove last row if nothing is selected
	if(bNothingRemoved)
		tableAngles->removeRow(tableAngles->rowCount()-1);

	tableAngles->setSortingEnabled(bSort);
}


/**
 * adds a single new item to the dead angles list
 */
void DeadAnglesDlg::AddAngle()
{
	const bool bSort = tableAngles->isSortingEnabled();
	tableAngles->setSortingEnabled(0);

	int iRow = tableAngles->rowCount();
	tableAngles->insertRow(iRow);

	tableAngles->setItem(iRow, static_cast<int>(AngleInfo::START_ANGLE), new QTableWidgetItem("0"));
	tableAngles->setItem(iRow, static_cast<int>(AngleInfo::STOP_ANGLE), new QTableWidgetItem("0"));
	tableAngles->setItem(iRow, static_cast<int>(AngleInfo::OFFSET_ANGLE), new QTableWidgetItem("0"));

	QComboBox *pComboCentreOn = new QComboBox(tableAngles);
	pComboCentreOn->addItem("Mono.");
	pComboCentreOn->addItem("Sample");
	pComboCentreOn->addItem("Ana.");
	pComboCentreOn->setCurrentIndex(1);
	tableAngles->setCellWidget(iRow, static_cast<int>(AngleInfo::CENTRE), pComboCentreOn);

	QComboBox *pComboRelativeTo = new QComboBox(tableAngles);
	pComboRelativeTo->addItem("Xtal Angle");
	pComboRelativeTo->addItem("In Axis");
	pComboRelativeTo->addItem("Out Axis");
	tableAngles->setCellWidget(iRow, static_cast<int>(AngleInfo::RELATIVE), pComboRelativeTo);

	tableAngles->setSortingEnabled(bSort);
}


/**
 * loads the items in the dead angles list from 'vecAngles'
 */
void DeadAnglesDlg::SetDeadAngles(const std::vector<DeadAngle<t_real>>& vecAngles)
{
	const bool bSort = tableAngles->isSortingEnabled();
	tableAngles->setSortingEnabled(0);

	// add missing rows
	while(tableAngles->rowCount() < int(vecAngles.size()))
		AddAngle();

	// remove superfluous rows
	while(tableAngles->rowCount() > int(vecAngles.size()))
		RemoveAngle();


	for(std::size_t iRow=0; iRow<vecAngles.size(); ++iRow)
	{
		const DeadAngle<t_real>& angle = vecAngles[iRow];

		tableAngles->item(iRow, static_cast<int>(AngleInfo::START_ANGLE))->
			setText(tl::var_to_str(angle.dAngleStart).c_str());
		tableAngles->item(iRow, static_cast<int>(AngleInfo::STOP_ANGLE))->
			setText(tl::var_to_str(angle.dAngleEnd).c_str());
		tableAngles->item(iRow, static_cast<int>(AngleInfo::OFFSET_ANGLE))->
			setText(tl::var_to_str(angle.dAngleOffs).c_str());

		QComboBox* pComboCentreOn = (QComboBox*)tableAngles->
			cellWidget(iRow, static_cast<int>(AngleInfo::CENTRE));
		QComboBox* pComboRelativeTo = (QComboBox*)tableAngles->
			cellWidget(iRow, static_cast<int>(AngleInfo::RELATIVE));

		pComboCentreOn->setCurrentIndex(angle.iCentreOn);
		pComboRelativeTo->setCurrentIndex(angle.iRelativeTo);
	}

	tableAngles->setSortingEnabled(bSort);
}


/**
 * emits the current list of dead angles
 */
void DeadAnglesDlg::SendApplyDeadAngles()
{
	std::vector<DeadAngle<t_real>> vecAngles;
	vecAngles.reserve(tableAngles->rowCount());

	for(int iRow=0; iRow<tableAngles->rowCount(); ++iRow)
	{
		DeadAngle<t_real> angle;
		angle.dAngleStart = 
			tl::str_to_var<t_real>(tableAngles->item(
				iRow, static_cast<int>(AngleInfo::START_ANGLE))->text().toStdString());
		angle.dAngleEnd = 
			tl::str_to_var<t_real>(tableAngles->item(
				iRow, static_cast<int>(AngleInfo::STOP_ANGLE))->text().toStdString());
		angle.dAngleOffs = 
			tl::str_to_var<t_real>(tableAngles->item(
				iRow, static_cast<int>(AngleInfo::OFFSET_ANGLE))->text().toStdString());

		QComboBox* pComboCentreOn = (QComboBox*)tableAngles->
			cellWidget(iRow, static_cast<int>(AngleInfo::CENTRE));
		QComboBox* pComboRelativeTo = (QComboBox*)tableAngles->
			cellWidget(iRow, static_cast<int>(AngleInfo::RELATIVE));

		angle.iCentreOn = pComboCentreOn->currentIndex();
		angle.iRelativeTo = pComboRelativeTo->currentIndex();

		vecAngles.emplace_back(std::move(angle));
	}

	emit ApplyDeadAngles(vecAngles);
}


void DeadAnglesDlg::ButtonBoxClicked(QAbstractButton* pBtn)
{
	if(buttonBox->buttonRole(pBtn) == QDialogButtonBox::ApplyRole ||
		buttonBox->buttonRole(pBtn) == QDialogButtonBox::AcceptRole)
	{
		SendApplyDeadAngles();
	}
	else if(buttonBox->buttonRole(pBtn) == QDialogButtonBox::RejectRole)
	{
		reject();
	}

	if(buttonBox->buttonRole(pBtn) == QDialogButtonBox::AcceptRole)
	{
		if(m_pSettings)
			m_pSettings->setValue("deadangles/geo", saveGeometry());

		QDialog::accept();
	}
}

void DeadAnglesDlg::closeEvent(QCloseEvent* pEvt)
{
	QDialog::closeEvent(pEvt);
}


#include "DeadAnglesDlg.moc"
