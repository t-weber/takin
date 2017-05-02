/**
 * exports
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date dec-2015
 * @license GPLv2
 */

#include "taz.h"
#include "tlibs/phys/atoms.h"
#include "tlibs/file/x3d.h"
#include "tlibs/log/log.h"
#include "tlibs/time/chrono.h"
#include "libs/version.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QtSvg/QSvgGenerator>


using t_real = t_real_glob;

//--------------------------------------------------------------------------------
// image exports

void TazDlg::ExportReal()
{
	TasLayout *pTas = m_sceneReal.GetTasLayout();
	if(!pTas) return;

	const t_real dZoom = pTas->GetZoom();
	pTas->SetZoom(1.);
	ExportSceneSVG(m_sceneReal);
	pTas->SetZoom(dZoom);
}

void TazDlg::ExportTof()
{
	TofLayout *pTof = m_sceneTof.GetTofLayout();
	if(!pTof) return;

	const t_real dZoom = pTof->GetZoom();
	pTof->SetZoom(1.);
	ExportSceneSVG(m_sceneTof);
	pTof->SetZoom(dZoom);
}

void TazDlg::ExportRealLattice()
{
	RealLattice *pLatt = m_sceneRealLattice.GetLattice();
	if(!pLatt) return;

	const t_real dZoom = pLatt->GetZoom();
	pLatt->SetZoom(1.);
	ExportSceneSVG(m_sceneRealLattice);
	pLatt->SetZoom(dZoom);
}

void TazDlg::ExportRecip()
{
	ScatteringTriangle *pTri = m_sceneRecip.GetTriangle();
	if(!pTri) return;

	const t_real dZoom = pTri->GetZoom();
	pTri->SetZoom(1.);
	ExportSceneSVG(m_sceneRecip);
	pTri->SetZoom(dZoom);
}

void TazDlg::ExportProj()
{
	ProjLattice *pLatt = m_sceneProjRecip.GetLattice();
	if(!pLatt) return;

	const t_real dZoom = pLatt->GetZoom();
	pLatt->SetZoom(1.);
	ExportSceneSVG(m_sceneProjRecip);
	pLatt->SetZoom(dZoom);
}

void TazDlg::ExportSceneSVG(QGraphicsScene& scene)
{
	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(!m_settings.value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = m_settings.value("main/last_dir_export", ".").toString();
	QString strFile = QFileDialog::getSaveFileName(this,
		"Export SVG", strDirLast, "SVG files (*.svg *.SVG)", nullptr, fileopt);
	if(strFile == "")
		return;

	QRectF rect = scene.sceneRect();

	QSvgGenerator svg;
	svg.setFileName(strFile);
	svg.setSize(QSize(rect.width(), rect.height()));
	//svg.setResolution(300);
	svg.setViewBox(QRectF(0, 0, rect.width(), rect.height()));
	svg.setDescription("Created with Takin");

	QPainter painter;
	painter.begin(&svg);
	scene.render(&painter);
	painter.end();

	std::string strDir = tl::get_dir(strFile.toStdString());
	m_settings.setValue("main/last_dir_export", QString(strDir.c_str()));
}


/**
 * export the 3d BZ as 3D model
 */
void TazDlg::ExportBZ3DModel()
{
	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(!m_settings.value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = m_settings.value("main/last_dir_export", ".").toString();
	QString strFile = QFileDialog::getSaveFileName(this,
		"Export X3D", strDirLast, "X3D files (*.x3d *.X3D)", nullptr, fileopt);
	if(strFile == "")
		return;


	ScatteringTriangle *pTri = m_sceneRecip.GetTriangle();
	if(!pTri) return;

	const auto& bz = pTri->GetBZ3D();
	if(!bz.IsValid())
	{
		QMessageBox::critical(this, "Error", "3D Brillouin zone calculation is disabled or results are invalid.");
		return;
	}


	using t_vec = ublas::vector<t_real>;
	tl::X3d x3d;

	/*// vertices
	for(const t_vec& vec : bz.GetVertices())
	{
		tl::X3dTrafo *pTrafo = new tl::X3dTrafo();
		pTrafo->SetTrans(vec - bz.GetCentralReflex());

		tl::X3dSphere *pSphere = new tl::X3dSphere(0.025);
		pSphere->SetColor(tl::make_vec({1., 0., 0.}));
		pTrafo->AddChild(pSphere);

		x3d.GetScene().AddChild(pTrafo);
	}*/

	// symmetry points
	for(const t_vec& vec : pTri->GetBZ3DSymmVerts())
	{
		tl::X3dTrafo *pTrafo = new tl::X3dTrafo();
		pTrafo->SetTrans(vec - bz.GetCentralReflex());

		tl::X3dSphere *pSphere = new tl::X3dSphere(0.025);
		pSphere->SetColor(tl::make_vec({1., 0., 0.}));
		pTrafo->AddChild(pSphere);

		x3d.GetScene().AddChild(pTrafo);
	}

	// polygons
	for(const std::vector<t_vec>& vecPoly : bz.GetPolys())
	{
		tl::X3dPolygon *pPoly = new tl::X3dPolygon();
		pPoly->SetColor(tl::make_vec({0., 0., 1.}));

		for(const t_vec& vec : vecPoly)
			pPoly->AddVertex(vec - bz.GetCentralReflex());

		x3d.GetScene().AddChild(pPoly);
	}


	/*// test plane cut
	tl::Plane<t_real> plane(bz.GetCentralReflex(), tl::make_vec({1., -1., 0.}));
	auto tupLinesandVerts = bz.GetIntersection(plane);

	for(const t_vec& vec : std::get<1>(tupLinesandVerts))
	{
		tl::X3dTrafo *pTrafo = new tl::X3dTrafo();
		pTrafo->SetTrans(vec);

		tl::X3dSphere *pSphere = new tl::X3dSphere(0.05);
		pSphere->SetColor(tl::make_vec({1., 1., 0.}));
		pTrafo->AddChild(pSphere);

		x3d.GetScene().AddChild(pTrafo);
	}*/


	// Comment
	std::ostringstream ostrComment;
	ostrComment << "Brillouin zone contains: ";
	ostrComment << bz.GetPolys().size() << " faces, ";
	ostrComment << bz.GetVertices().size() << " vertices, ";
	ostrComment << pTri->GetBZ3DSymmVerts().size() << " symmetry points.\n";

	std::string strTakin = "\nCreated with Takin " + std::string(TAKIN_VER) + ".\n";
	strTakin += "Timestamp: " + tl::epoch_to_str<t_real>(tl::epoch<t_real>()) + "\n\n";
	x3d.SetComment(strTakin + ostrComment.str());

	bool bOk = x3d.Save(strFile.toStdString().c_str());


	if(!bOk)
		QMessageBox::critical(this, "Error", "Could not export 3D model.");

	if(bOk)
	{
		std::string strDir = tl::get_dir(strFile.toStdString());
		m_settings.setValue("main/last_dir_export", QString(strDir.c_str()));
	}
}



#ifdef USE_GIL
void TazDlg::ExportBZImage()
{
	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(!m_settings.value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = m_settings.value("main/last_dir_export", ".").toString();
	QString strFile = QFileDialog::getSaveFileName(this,
		"Export PNG", strDirLast, "PNG files (*.png *.PNG)", nullptr, fileopt);
	if(strFile == "")
		return;

	bool bOk = m_sceneRecip.ExportBZAccurate(strFile.toStdString().c_str());
	if(!bOk)
		QMessageBox::critical(this, "Error", "Could not export image.");

	if(bOk)
	{
		std::string strDir = tl::get_dir(strFile.toStdString());
		m_settings.setValue("main/last_dir_export", QString(strDir.c_str()));
	}
}

void TazDlg::ExportWSImage()
{
	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(!m_settings.value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = m_settings.value("main/last_dir_export", ".").toString();
	QString strFile = QFileDialog::getSaveFileName(this,
		"Export PNG", strDirLast, "PNG files (*.png *.PNG)", nullptr, fileopt);
	if(strFile == "")
		return;

	bool bOk = m_sceneRealLattice.ExportWSAccurate(strFile.toStdString().c_str());
	if(!bOk)
		QMessageBox::critical(this, "Error", "Could not export image.");

	if(bOk)
	{
		std::string strDir = tl::get_dir(strFile.toStdString());
		m_settings.setValue("main/last_dir_export", QString(strDir.c_str()));
	}
}
#else
void TazDlg::ExportBZImage() {}
void TazDlg::ExportWSImage() {}
#endif



//--------------------------------------------------------------------------------
// 3d model exports

void TazDlg::ExportUCModel()
{
	using t_mat = ublas::matrix<t_real>;
	using t_vec = ublas::vector<t_real>;


	if(m_vecAtoms.size() == 0)
	{
		QMessageBox::critical(this, "Error", "No atom positions defined for unit cell.");
		return;
	}


	SpaceGroup<t_real>* pSpaceGroup = nullptr;
	int iSpaceGroupIdx = comboSpaceGroups->currentIndex();
	if(iSpaceGroupIdx != 0)
		pSpaceGroup = (SpaceGroup<t_real>*)comboSpaceGroups->itemData(iSpaceGroupIdx).value<void*>();
	if(!pSpaceGroup)
	{
		QMessageBox::critical(this, "Error", "Invalid space group.");
		return;
	}

	const std::vector<t_mat>& vecTrafos = pSpaceGroup->GetTrafos();

	std::vector<t_vec> vecAtoms;
	std::vector<std::string> vecAtomNames;
	for(const AtomPos<t_real>& atom : m_vecAtoms)
	{
		t_vec vecAtomPos = atom.vecPos;
		vecAtomPos.resize(4, 1);
		vecAtomPos[3] = 1.;
		vecAtoms.push_back(std::move(vecAtomPos));
		vecAtomNames.push_back(atom.strAtomName);
	}


	const t_real a = editA->text().toDouble();
	const t_real b = editB->text().toDouble();
	const t_real c = editC->text().toDouble();
	const t_real alpha = tl::d2r(editAlpha->text().toDouble());
	const t_real beta = tl::d2r(editBeta->text().toDouble());
	const t_real gamma = tl::d2r(editGamma->text().toDouble());
	const tl::Lattice<t_real> lattice(a,b,c, alpha,beta,gamma);
	const t_mat matA = lattice.GetMetric();


	const std::vector<t_vec> vecColors =
	{
		tl::make_vec({1., 0., 0.}), tl::make_vec({0., 1., 0.}), tl::make_vec({0., 0., 1.}),
		tl::make_vec({1., 1., 0.}), tl::make_vec({0., 1., 1.}), tl::make_vec({1., 0., 1.}),
		tl::make_vec({0.5, 0., 0.}), tl::make_vec({0., 0.5, 0.}), tl::make_vec({0., 0., 0.5}),
		tl::make_vec({0.5, 0.5, 0.}), tl::make_vec({0., 0.5, 0.5}), tl::make_vec({0.5, 0., 0.5}),
		tl::make_vec({1., 1., 1.}), tl::make_vec({0., 0., 0.}), tl::make_vec({0.5, 0.5, 0.5}),
	};

	const std::vector<t_real> vecRadii =
	{
		0.025, 0.05, 0.075, 0.1, 0.125, 0.15, 0.175, 0.2
	};

	// to transform into program-specific coordinate systems
	const t_mat matGlobal = tl::make_mat(
	{	{-1., 0., 0., 0.},
		{ 0., 0., 1., 0.},
		{ 0., 1., 0., 0.},
		{ 0., 0., 0., 1.}	});


	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(!m_settings.value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = m_settings.value("main/last_dir_export", ".").toString();
	QString strFile = QFileDialog::getSaveFileName(this,
		"Export X3D", strDirLast, "X3D files (*.x3d *.X3D)", nullptr, fileopt);
	if(strFile == "")
		return;


	tl::X3d x3d;

	std::vector<t_vec> vecAllAtoms, vecAllAtomsFrac;
	std::vector<std::size_t> vecAllAtomTypes, vecIdxSC;

	// unit cell
	const t_real dUCSize = 1.;
	std::tie(std::ignore, vecAllAtoms, vecAllAtomsFrac, vecAllAtomTypes) =
		tl::generate_all_atoms<t_mat, t_vec, std::vector>
			(vecTrafos, vecAtoms,
			static_cast<const std::vector<std::string>*>(0), matA,
			-dUCSize*t_real(0.5), dUCSize*t_real(0.5), g_dEps);

	// super cell
	const std::size_t iSC_NN = 2;
	std::vector<t_vec> vecAtomsSC;
	std::vector<std::complex<t_real>> vecJUC;
	std::tie(vecAtomsSC, std::ignore, vecIdxSC) =
		tl::generate_supercell<t_vec, std::vector, t_real>
			(lattice, vecAllAtoms, vecJUC, iSC_NN);

	//for(std::size_t iAtom=0; iAtom<vecAllAtoms.size(); ++iAtom)
	for(std::size_t iAtom=0; iAtom<vecIdxSC.size(); ++iAtom)
	{
		//std::size_t iAtomType = vecAllAtomTypes[iAtom];
		//t_vec vecCoord = vecAllAtoms[iAtom];

		std::size_t iAtomType = vecAllAtomTypes[vecIdxSC[iAtom]];
		//if(iAtomType != 1) continue;	// testing: keep only one atom type

		const t_vec& vecCentral = vecAtomsSC[iAtom];
		t_vec vecCoord = vecCentral;
		vecCoord.resize(4,1); vecCoord[3] = 1.;

		tl::X3dTrafo *pTrafo = new tl::X3dTrafo();
		pTrafo->SetTrans(tl::mult<t_mat, t_vec>(matGlobal, vecCoord));
		t_real dRadius = vecRadii[iAtomType % vecRadii.size()];
		tl::X3dSphere *pSphere = new tl::X3dSphere(dRadius);
		pSphere->SetColor(vecColors[iAtomType % vecColors.size()]);
		pTrafo->AddChild(pSphere);

		// surroundings
		/*if(iAtomType == 0)
		{
			std::vector<std::vector<std::size_t>> vecNN =
				tl::get_neighbours<t_vec, std::vector, t_real>
					(vecAtomsSC, vecCentral);

			if(vecNN.size() > 1 && vecNN[1].size() > 2)
			{
				tl::X3dPolygon *pPoly = new tl::X3dPolygon();

				for(std::size_t iNN : vecNN[1])
				{
					const t_vec& vecNeighbour = vecAtomsSC[iNN];
					pPoly->AddVertex(vecNeighbour);
				}

				x3d.GetScene().AddChild(pPoly);
			}
		}*/

		x3d.GetScene().AddChild(pTrafo);
	}

	// comment
	std::ostringstream ostrComment;
	ostrComment << "Unit cell.\n";

	for(std::size_t iAtom=0; iAtom<vecAtoms.size(); ++iAtom)
	{
		ostrComment << "Atom " << (iAtom+1) << ": " << vecAtomNames[iAtom]
			<< ", colour: " << vecColors[iAtom % vecColors.size()] << "\n";
	}
	ostrComment << "Unit cell contains " << vecAllAtoms.size() << " atoms.\n";
	if(iSC_NN > 1)
		ostrComment << "Super cell contains " << vecIdxSC.size() << " atoms.\n";


	if(lattice.IsCubic())
	{
		tl::X3dCube *pCube = new tl::X3dCube(a,b,c);
		pCube->SetColor(tl::make_vec({1., 1., 1., 0.75}));
		x3d.GetScene().AddChild(pCube);
	}


	tl::log_info(ostrComment.str());
	std::string strTakin = "\nCreated with Takin " + std::string(TAKIN_VER) + ".\n";
	strTakin += "Timestamp: " + tl::epoch_to_str<t_real>(tl::epoch<t_real>()) + "\n\n";
	x3d.SetComment(strTakin + ostrComment.str());

	bool bOk = x3d.Save(strFile.toStdString().c_str());

	if(bOk)
	{
		std::string strDir = tl::get_dir(strFile.toStdString());
		m_settings.setValue("main/last_dir_export", QString(strDir.c_str()));
	}
	else
	{
		QMessageBox::critical(this, "Error", "Error exporting x3d file.");
	}
}
