/**
 * Crystal lattice projections
 * @author tweber
 * @date may-2016
 * @license GPLv2
 */

#ifndef __TAZ_PROJ_LATTICE_H__
#define __TAZ_PROJ_LATTICE_H__

#include "tlibs/math/linalg.h"
#include "tlibs/math/lattice.h"
#include "tlibs/math/neutrons.h"
#include "tlibs/math/geo.h"

#include "libs/globals.h"
#include "libs/globals_qt.h"
#include "libs/spacegroups/spacegroup.h"
#include "libs/spacegroups/latticehelper.h"

#include "tasoptions.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsTextItem>
#include <QWheelEvent>
#if QT_VER>=5
	#include <QtWidgets>
#endif

namespace ublas = boost::numeric::ublas;

enum class LatticeProj
{
	GNOMONIC,
	STEREOGRAPHIC,
	PARALLEL,
	PERSPECTIVE
};

class ProjLattice;

class ProjLatticePoint : public QGraphicsItem
{
	protected:
		QColor m_color = Qt::red;
		QString m_strLabel;
		QString m_strTT;
		t_real_glob m_dRadius = 0.;

	protected:
		virtual QRectF boundingRect() const override;
		virtual void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;

	public:
		ProjLatticePoint();

		void SetLabel(const QString& str) { m_strLabel = str; }
		void SetColor(const QColor& col) { m_color = col; }

		void AddTooltip(const QString& strTT) { if(m_strTT.length()) m_strTT+=", "; m_strTT += strTT; }
		void SetTooltip() { setToolTip(m_strTT);}

		void SetRadius(t_real_glob dRad) { m_dRadius = dRad; }
		void AddRadius(t_real_glob dRad) { m_dRadius += dRad; }
		t_real_glob GetRadius() const { return m_dRadius; }
};


class ProjLatticeScene;
class ProjLattice : public QGraphicsItem
{
	protected:
		bool m_bReady = 0;
		ProjLatticeScene &m_scene;

		t_real_glob m_dScaleFactor = 150.;	// pixels per A for zoom == 1.
		t_real_glob m_dZoom = 1.;
		int m_iMaxPeaks = 5;

		tl::Lattice<t_real_glob> m_lattice;
		ublas::matrix<t_real_glob> m_matPlane, m_matPlane_inv;
		std::vector<ProjLatticePoint*> m_vecPeaks;

		LatticeProj m_proj = LatticeProj::STEREOGRAPHIC;

	protected:
		virtual QRectF boundingRect() const override;

	public:
		ProjLattice(ProjLatticeScene& scene);
		virtual ~ProjLattice();

		virtual void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;

		void SetReady(bool bReady) { m_bReady = bReady; }
		bool IsReady() const { return m_bReady; }

		const ublas::matrix<t_real_glob>& GetPlane() const { return m_matPlane; }

		void SetProjection(LatticeProj proj) { m_proj = proj; }

	public:
		bool HasPeaks() const { return m_vecPeaks.size()!=0 && m_lattice.IsInited(); }
		void ClearPeaks();
		void CalcPeaks(const LatticeCommon<t_real_glob>& recipcommon, bool bIsRecip=1);

		void SetMaxPeaks(int iMax) { m_iMaxPeaks = iMax; }
		unsigned int GetMaxPeaks() const { return m_iMaxPeaks; }
		void SetZoom(t_real_glob dZoom);
		t_real_glob GetZoom() const { return m_dZoom; }

	public:
		t_real_glob GetScaleFactor() const { return m_dScaleFactor; }
		void SetScaleFactor(t_real_glob dScale) { m_dScaleFactor = dScale; }

		const tl::Lattice<t_real_glob>& GetProjLattice() const { return m_lattice; }
};


class ProjLatticeScene : public QGraphicsScene
{	Q_OBJECT
	protected:
		ProjLattice *m_pLatt;

		bool m_bSnap = 0;
		bool m_bMousePressed = 0;

	public:
		ProjLatticeScene(QObject *pParent=nullptr);
		virtual ~ProjLatticeScene();

		const ProjLattice* GetLattice() const { return m_pLatt; }
		ProjLattice* GetLattice() { return m_pLatt; }

	public slots:
		void scaleChanged(t_real_glob dTotalScale);

	signals:
		void coordsChanged(t_real_glob dh, t_real_glob dk, t_real_glob dl,
			bool bHasNearest,
			t_real_glob dNearestH, t_real_glob dNearestK, t_real_glob dNearestL);

	protected:
		virtual void mousePressEvent(QGraphicsSceneMouseEvent *pEvt) override;
		virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *pEvt) override;
		virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *pEvt) override;

		virtual void keyPressEvent(QKeyEvent *pEvt) override;
		virtual void keyReleaseEvent(QKeyEvent *pEvt) override;

		virtual void drawBackground(QPainter*, const QRectF&) override;
};


class ProjLatticeView : public QGraphicsView
{
	Q_OBJECT
	protected:
		t_real_glob m_dTotalScale = 1.;
		virtual void wheelEvent(QWheelEvent* pEvt) override;

	public:
		ProjLatticeView(QWidget* pParent = 0);
		virtual ~ProjLatticeView();

	signals:
		void scaleChanged(t_real_glob dTotalScale);
};

#endif
