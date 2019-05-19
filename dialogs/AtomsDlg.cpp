/**
 * Atom Positions Dialog
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date nov-2015
 * @license GPLv2
 */

#include "AtomsDlg.h"
#include "tlibs/string/string.h"
#include "tlibs/math/linalg.h"
#include "libs/formfactors/formfact.h"
#include <QMessageBox>

using t_real = t_real_glob;


enum class AtInfo : int
{
	NAME = 0,
	POS_X = 1,
	POS_Y = 2,
	POS_Z = 3,

	SPIN_X = 4,
	SPIN_Y = 5,
	SPIN_Z = 6
};


AtomsDlg::AtomsDlg(QWidget* pParent, QSettings *pSettings, bool bEnableSpin)
	: QDialog(pParent), m_pSettings(pSettings), m_bEnableSpin(bEnableSpin)
{
	setupUi(this);
	if(m_pSettings)
	{
		QFont font;
		if(m_pSettings->contains("main/font_gen") && font.fromString(m_pSettings->value("main/font_gen", "").toString()))
			setFont(font);
	}

	if(m_bEnableSpin)
	{
		tableAtoms->setColumnCount(7);
		tableAtoms->setHorizontalHeaderItem(static_cast<int>(AtInfo::SPIN_X), new QTableWidgetItem("Spin x"));
		tableAtoms->setHorizontalHeaderItem(static_cast<int>(AtInfo::SPIN_Y), new QTableWidgetItem("Spin y"));
		tableAtoms->setHorizontalHeaderItem(static_cast<int>(AtInfo::SPIN_Z), new QTableWidgetItem("Spin z"));
	}

	tableAtoms->setColumnWidth(int(AtInfo::NAME), 75);
	btnAdd->setIcon(load_icon("res/icons/list-add.svg"));
	btnDel->setIcon(load_icon("res/icons/list-remove.svg"));

#if QT_VER >= 5
	QObject::connect(btnAdd, &QAbstractButton::clicked, this, &AtomsDlg::AddAtom);
	QObject::connect(btnDel, &QAbstractButton::clicked, this, &AtomsDlg::RemoveAtom);
	QObject::connect(buttonBox, &QDialogButtonBox::clicked, this, &AtomsDlg::ButtonBoxClicked);
	QObject::connect(tableAtoms, &QTableWidget::cellChanged, this, &AtomsDlg::AtomCellChanged);
#else
	QObject::connect(btnAdd, SIGNAL(clicked(bool)), this, SLOT(AddAtom()));
	QObject::connect(btnDel, SIGNAL(clicked(bool)), this, SLOT(RemoveAtom()));
	QObject::connect(buttonBox, SIGNAL(clicked(QAbstractButton*)), this,
		SLOT(ButtonBoxClicked(QAbstractButton*)));
	QObject::connect(tableAtoms, SIGNAL(cellChanged(int, int)), this, SLOT(AtomCellChanged(int, int)));
#endif

	if(m_pSettings && m_pSettings->contains("atoms/geo"))
		restoreGeometry(m_pSettings->value("atoms/geo").toByteArray());
}


AtomsDlg::~AtomsDlg()
{}


void AtomsDlg::RemoveAtom()
{
	const bool bSort = tableAtoms->isSortingEnabled();
	tableAtoms->setSortingEnabled(0);

	bool bNothingRemoved = 1;

	// remove selected rows
	QList<QTableWidgetSelectionRange> lstSel = tableAtoms->selectedRanges();
	for(QTableWidgetSelectionRange& range : lstSel)
	{
		//std::cout << range.bottomRow() << " " << range.topRow() << std::endl;
		for(int iRow=range.bottomRow(); iRow>=range.topRow(); --iRow)
		{
			tableAtoms->removeRow(iRow);
			bNothingRemoved = 0;
		}
	}

	// remove last row if nothing is selected
	if(bNothingRemoved)
		tableAtoms->removeRow(tableAtoms->rowCount()-1);

	tableAtoms->setSortingEnabled(bSort);
}


void AtomsDlg::AddAtom()
{
	const bool bSort = tableAtoms->isSortingEnabled();
	tableAtoms->setSortingEnabled(0);

	int iRow = tableAtoms->rowCount();
	tableAtoms->insertRow(iRow);
	tableAtoms->setItem(iRow, 0, new QTableWidgetItem("H"));
	for(unsigned int i=0; i<3; ++i)
		tableAtoms->setItem(iRow, static_cast<int>(AtInfo::POS_X)+i, new QTableWidgetItem("0"));
	if(m_bEnableSpin)
		for(unsigned int i=0; i<3; ++i)
			tableAtoms->setItem(iRow, static_cast<int>(AtInfo::SPIN_X)+i, new QTableWidgetItem("0"));

	tableAtoms->setSortingEnabled(bSort);
	CheckAtoms();
}


void AtomsDlg::SetAtoms(const std::vector<xtl::AtomPos<t_real>>& vecAtoms)
{
	const bool bSort = tableAtoms->isSortingEnabled();
	tableAtoms->setSortingEnabled(0);

	tableAtoms->setRowCount(vecAtoms.size());

	for(std::size_t iRow=0; iRow<vecAtoms.size(); ++iRow)
	{
		// add missing items
		for(int iCol=0; iCol<tableAtoms->columnCount(); ++iCol)
			if(!tableAtoms->item(iRow, iCol))
				tableAtoms->setItem(iRow, iCol, new QTableWidgetItem(""));

		const xtl::AtomPos<t_real>& atom = vecAtoms[iRow];
		tableAtoms->item(iRow, 0)->setText(atom.strAtomName.c_str());
		for(unsigned int i=0; i<3; ++i)
			tableAtoms->item(iRow, static_cast<int>(AtInfo::POS_X)+i)->setText(tl::var_to_str(atom.vecPos[i]).c_str());

		if(m_bEnableSpin)
			for(unsigned int i=0; i<3; ++i)
				tableAtoms->item(iRow, static_cast<int>(AtInfo::SPIN_X)+i)->setText(tl::var_to_str(atom.vecSpin[i]).c_str());
	}

	tableAtoms->setSortingEnabled(bSort);
	CheckAtoms();
}


void AtomsDlg::AtomCellChanged(int iRow, int iCol)
{
	if(iCol == static_cast<int>(AtInfo::NAME))
		CheckAtoms();
}


/**
 * checks if atom names are in the scattering lenth table
 */
void AtomsDlg::CheckAtoms()
{
	std::ostringstream ostrErr;

	std::shared_ptr<const xtl::FormfactList<t_real>> lstff = xtl::FormfactList<t_real>::GetInstance();

	//QColor colOk = qApp->palette().color(QPalette::Base);
	QColor colOk{0x00, 0x88, 0x00};
	QColor colFail{0xff, 0x00, 0x00};

	for(int iRow=0; iRow<tableAtoms->rowCount(); ++iRow)
	{
		QTableWidgetItem* pItem = tableAtoms->item(iRow, static_cast<int>(AtInfo::NAME));
		if(!pItem)
			continue;
		std::string strAtomName = pItem->text().toStdString();
		tl::trim(strAtomName);
		bool bFound = (lstff && lstff->Find(strAtomName) != nullptr);
		if(!bFound)
			ostrErr << "\"" << strAtomName << "\" was not found, will be ignored in cross-section.\n";

		tableAtoms->item(iRow, static_cast<int>(AtInfo::NAME))->setBackground(bFound ? colOk : colFail);
	}

	m_strErr = ostrErr.str();
}


void AtomsDlg::ShowPossibleErrorDlg()
{
	if(m_strErr != "")
		QMessageBox::critical(this, "Error", m_strErr.c_str());
}


void AtomsDlg::SendApplyAtoms()
{
	std::vector<xtl::AtomPos<t_real>> vecAtoms;
	vecAtoms.reserve(tableAtoms->rowCount());

	for(int iRow=0; iRow<tableAtoms->rowCount(); ++iRow)
	{
		xtl::AtomPos<t_real> atom;
		atom.strAtomName = tableAtoms->item(iRow, static_cast<int>(AtInfo::NAME))->text().toStdString();
		tl::trim(atom.strAtomName);
		t_real dX = tl::str_to_var_parse<t_real>(tableAtoms->item(iRow, static_cast<int>(AtInfo::POS_X))->text().toStdString());
		t_real dY = tl::str_to_var_parse<t_real>(tableAtoms->item(iRow, static_cast<int>(AtInfo::POS_Y))->text().toStdString());
		t_real dZ = tl::str_to_var_parse<t_real>(tableAtoms->item(iRow, static_cast<int>(AtInfo::POS_Z))->text().toStdString());
		atom.vecPos = tl::make_vec({dX, dY, dZ});

		if(m_bEnableSpin)
		{
			t_real dSx = tl::str_to_var_parse<t_real>(tableAtoms->item(iRow, static_cast<int>(AtInfo::SPIN_X))->text().toStdString());
			t_real dSy = tl::str_to_var_parse<t_real>(tableAtoms->item(iRow, static_cast<int>(AtInfo::SPIN_Y))->text().toStdString());
			t_real dSz = tl::str_to_var_parse<t_real>(tableAtoms->item(iRow, static_cast<int>(AtInfo::SPIN_Z))->text().toStdString());
			atom.vecSpin = tl::make_vec({dSx, dSy, dSz});
		}

		vecAtoms.emplace_back(std::move(atom));
	}

	emit ApplyAtoms(vecAtoms);
}


void AtomsDlg::ButtonBoxClicked(QAbstractButton* pBtn)
{
	if(buttonBox->buttonRole(pBtn) == QDialogButtonBox::ApplyRole ||
		buttonBox->buttonRole(pBtn) == QDialogButtonBox::AcceptRole)
	{
		ShowPossibleErrorDlg();
		SendApplyAtoms();
	}
	else if(buttonBox->buttonRole(pBtn) == QDialogButtonBox::RejectRole)
	{
		reject();
	}

	if(buttonBox->buttonRole(pBtn) == QDialogButtonBox::AcceptRole)
	{
		if(m_pSettings)
			m_pSettings->setValue("atoms/geo", saveGeometry());

		QDialog::accept();
	}
}


void AtomsDlg::closeEvent(QCloseEvent* pEvt)
{
	QDialog::closeEvent(pEvt);
}


#include "AtomsDlg.moc"
