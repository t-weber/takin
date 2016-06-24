/*
 * Space Group List Dialog
 * @author tweber
 * @date oct-2015
 * @license GPLv2
 */

#include "SgListDlg.h"
#include <sstream>
//#include <iostream>
#include "tlibs/string/string.h"
#include "libs/spacegroups/spacegroup.h"
#include "libs/globals.h"

using t_real = t_real_glob;


SgListDlg::SgListDlg(QWidget *pParent)
	: QDialog(pParent, Qt::WindowTitleHint|Qt::WindowCloseButtonHint|Qt::WindowMinMaxButtonsHint),
		m_settings("tobis_stuff", "sglist")
{
	setupUi(this);
	QFont font;
	if(m_settings.contains("main/font_gen") && font.fromString(m_settings.value("main/font_gen", "").toString()))
		setFont(font);


	SetupSpacegroups();

	QObject::connect(listSGs, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
		this, SLOT(SGSelected(QListWidgetItem*, QListWidgetItem*)));
	QObject::connect(checkMatrices, SIGNAL(toggled(bool)), this, SLOT(UpdateSG()));

	for(QSpinBox* pSpin : {spinH, spinK, spinL})
		QObject::connect(pSpin, SIGNAL(valueChanged(int)), this, SLOT(RecalcBragg()));

	for(QDoubleSpinBox* pSpin : {spinX, spinY, spinZ, spinW})
		QObject::connect(pSpin, SIGNAL(valueChanged(double)), this, SLOT(CalcTrafo()));

	QObject::connect(editFilter, SIGNAL(textEdited(const QString&)),
		this, SLOT(SearchSG(const QString&)));


	if(m_settings.contains("sglist/geo"))
		restoreGeometry(m_settings.value("sglist/geo").toByteArray());
}

SgListDlg::~SgListDlg()
{}

void SgListDlg::closeEvent(QCloseEvent* pEvt)
{
	m_settings.setValue("sglist/geo", saveGeometry());
}


static QListWidgetItem* create_header_item(const char *pcTitle)
{
	QListWidgetItem *pHeaderItem = new QListWidgetItem(pcTitle);
	pHeaderItem->setTextAlignment(Qt::AlignHCenter);

	QFont fontHeader = pHeaderItem->font();
	fontHeader.setBold(1);
	pHeaderItem->setFont(fontHeader);

	QBrush brushHeader = pHeaderItem->foreground();
	brushHeader.setColor(QColor(0xff, 0xff, 0xff));
	pHeaderItem->setForeground(brushHeader);

	pHeaderItem->setData(Qt::UserRole, 1000);
	pHeaderItem->setBackgroundColor(QColor(0x65, 0x65, 0x65));

	return pHeaderItem;
}

void SgListDlg::SetupSpacegroups()
{
	listSGs->clear();

	std::shared_ptr<const SpaceGroups<t_real>> sgs = SpaceGroups<t_real>::GetInstance();
	const SpaceGroups<t_real>::t_vecSpaceGroups* pvecSG = sgs->get_space_groups_vec();

	// actually: space group TYPE, not space group...
	for(unsigned int iSG=0; iSG<pvecSG->size(); ++iSG)
	{
		const SpaceGroup<t_real>* psg = pvecSG->at(iSG);
		unsigned int iSgNr = psg->GetNr();

		// list headers
		if(iSgNr==1) listSGs->addItem(create_header_item("Triclinic"));
		else if(iSgNr==3) listSGs->addItem(create_header_item("Monoclinic"));
		else if(iSgNr==16) listSGs->addItem(create_header_item("Orthorhombic"));
		else if(iSgNr==75) listSGs->addItem(create_header_item("Tetragonal"));
		else if(iSgNr==143) listSGs->addItem(create_header_item("Trigonal"));
		else if(iSgNr==168) listSGs->addItem(create_header_item("Hexagonal"));
		else if(iSgNr==195) listSGs->addItem(create_header_item("Cubic"));


		std::ostringstream ostrSg;
		ostrSg << "No. " << iSgNr << ": " << psg->GetName();

		QListWidgetItem* pItem = new QListWidgetItem(ostrSg.str().c_str());
		pItem->setData(Qt::UserRole, iSG);
		listSGs->addItem(pItem);
	}
}

void SgListDlg::UpdateSG()
{
	SGSelected(listSGs->currentItem(), nullptr);
}

void SgListDlg::SGSelected(QListWidgetItem *pItem, QListWidgetItem*)
{
	listSymOps->clear();
	listTrafo->clear();
	for(QLineEdit *pEdit : {editHM, /*editHall,*/ editLaue, editNr/*, editAxisSym*/})
		pEdit->setText("");
	if(!pItem) return;


	std::shared_ptr<const SpaceGroups<t_real>> sgs = SpaceGroups<t_real>::GetInstance();
	const SpaceGroups<t_real>::t_vecSpaceGroups* pvecSG = sgs->get_space_groups_vec();

	// header selected?
	unsigned int iSG = pItem->data(Qt::UserRole).toUInt();
	if(iSG >= pvecSG->size())
		return;

	const SpaceGroup<t_real>* psg = pvecSG->at(iSG);
	unsigned int iSgNr = psg->GetNr();

	const std::string& strHM = psg->GetName();
	const std::string& strPointGroup = psg->GetPointGroup();
	const std::string& strLaue = psg->GetLaueGroup();
	const std::string& strCrysSys = psg->GetCrystalSystemName();

	editNr->setText(tl::var_to_str(iSgNr).c_str());
	editHM->setText(strHM.c_str());
	//editHall->setText(psg.symbol_hall().c_str());
	editLaue->setText(("PG: " + strPointGroup + ", LG: " + strLaue +
		" (" + strCrysSys + ")").c_str());

	bool bShowMatrices = checkMatrices->isChecked();

	// all trafos
	const std::vector<SpaceGroup<t_real>::t_mat>& vecTrafos = psg->GetTrafos();
	{
		std::ostringstream ostr;
		ostr << "All Symmetry Operations (" << vecTrafos.size() << ")";
		listSymOps->addItem(create_header_item(ostr.str().c_str()));

		for(unsigned int iSymOp=0; iSymOp<vecTrafos.size(); ++iSymOp)
		{
			if(bShowMatrices)
				listSymOps->addItem(print_matrix(vecTrafos[iSymOp]).c_str());
			else
				listSymOps->addItem(get_trafo_desc(vecTrafos[iSymOp]).c_str());
		}
	}


	// primitive trafos
	const std::vector<unsigned int>& vecPrim = psg->GetPrimTrafos();

	if(vecPrim.size())
	{
		std::ostringstream ostr;
		ostr << "Primitive Symmetry Operations (" << (vecPrim.size()) << ")";
		listSymOps->addItem(create_header_item(ostr.str().c_str()));
		for(unsigned int iSymOp=0; iSymOp<vecPrim.size(); ++iSymOp)
		{
			if(bShowMatrices)
				listSymOps->addItem(print_matrix(vecTrafos[vecPrim[iSymOp]]).c_str());
			else
				listSymOps->addItem(get_trafo_desc(vecTrafos[vecPrim[iSymOp]]).c_str());
		}
	}


	// inverting trafos
	const std::vector<unsigned int>& vecInv = psg->GetInvTrafos();

	if(vecInv.size())
	{
		std::ostringstream ostr;
		ostr << "Inverting Symmetry Operations (" << (vecInv.size()) << ")";
		listSymOps->addItem(create_header_item(ostr.str().c_str()));
		for(unsigned int iSymOp=0; iSymOp<vecInv.size(); ++iSymOp)
		{
			if(bShowMatrices)
				listSymOps->addItem(print_matrix(vecTrafos[vecInv[iSymOp]]).c_str());
			else
				listSymOps->addItem(get_trafo_desc(vecTrafos[vecInv[iSymOp]]).c_str());
		}
	}

	// centering trafos
	const std::vector<unsigned int>& vecCenter = psg->GetCenterTrafos();

	if(vecCenter.size())
	{
		std::ostringstream ostr;
		ostr << "Centering Symmetry Operations (" << (vecCenter.size()) << ")";
		listSymOps->addItem(create_header_item(ostr.str().c_str()));
		for(unsigned int iSymOp=0; iSymOp<vecCenter.size(); ++iSymOp)
		{
			if(bShowMatrices)
				listSymOps->addItem(print_matrix(vecTrafos[vecCenter[iSymOp]]).c_str());
			else
				listSymOps->addItem(get_trafo_desc(vecTrafos[vecCenter[iSymOp]]).c_str());
		}
	}


	RecalcBragg();
	CalcTrafo();
}

void SgListDlg::RecalcBragg()
{
	const QListWidgetItem* pItem = listSGs->currentItem();
	if(!pItem) return;

	const int h = spinH->value();
	const int k = spinK->value();
	const int l = spinL->value();

	//std::cout << h << k << l << std::endl;

	std::shared_ptr<const SpaceGroups<t_real>> sgs = SpaceGroups<t_real>::GetInstance();
	const SpaceGroups<t_real>::t_vecSpaceGroups* pvecSG = sgs->get_space_groups_vec();
	const unsigned int iSG = pItem->data(Qt::UserRole).toUInt();
	if(iSG >= pvecSG->size())
		return;

	const SpaceGroup<t_real>* psg = pvecSG->at(iSG);
	const bool bForbidden = !psg->HasReflection(h,k,l);;

	QFont font = spinH->font();
	font.setStrikeOut(bForbidden);
	for(QSpinBox* pSpin : {spinH, spinK, spinL})
		pSpin->setFont(font);
}

void SgListDlg::SearchSG(const QString& qstr)
{
	QList<QListWidgetItem*> lstItems = listSGs->findItems(qstr, Qt::MatchContains);
	if(lstItems.size())
		listSGs->setCurrentItem(lstItems[0], QItemSelectionModel::SelectCurrent);
}


void SgListDlg::CalcTrafo()
{
	listTrafo->clear();

	const QListWidgetItem* pItem = listSGs->currentItem();
	if(!pItem)
		return;

	std::shared_ptr<const SpaceGroups<t_real>> sgs = SpaceGroups<t_real>::GetInstance();
	const SpaceGroups<t_real>::t_vecSpaceGroups* pvecSG = sgs->get_space_groups_vec();
	const unsigned int iSG = pItem->data(Qt::UserRole).toUInt();
	if(iSG >= pvecSG->size())
		return;
	const SpaceGroup<t_real>* psg = pvecSG->at(iSG);

	SpaceGroup<t_real>::t_vec vecIn =
		tl::make_vec({spinX->value(), spinY->value(), spinZ->value(), spinW->value()});

	const std::vector<SpaceGroup<t_real>::t_mat>& vecTrafos = psg->GetTrafos();
	std::vector<SpaceGroup<t_real>::t_vec> vecUnique;

	listTrafo->addItem(create_header_item("All Transformation Results"));
	for(const SpaceGroup<t_real>::t_mat& mat : vecTrafos)
	{
		SpaceGroup<t_real>::t_vec vec = ublas::prod(mat, vecIn);
		listTrafo->addItem(print_vector(vec).c_str());

		if(!is_vec_in_container<std::vector, SpaceGroup<t_real>::t_vec>(vecUnique, vec))
			vecUnique.push_back(vec);
	}

	listTrafo->addItem(create_header_item("Unique Transformation Results"));
	for(const SpaceGroup<t_real>::t_vec& vec : vecUnique)
	{
		listTrafo->addItem(print_vector(vec).c_str());
	}
}

#include "SgListDlg.moc"
