/**
 * Scattering Triangle
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 2014 - 2016
 * @license GPLv2
 */

#ifndef __TAZ_SCATT_TRIAG_H__
#define __TAZ_SCATT_TRIAG_H__

#include <memory>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsTextItem>
#include <QWheelEvent>
#if QT_VER>=5
	#include <QtWidgets>
#endif

#include "tlibs/math/linalg.h"
#include "tlibs/math/kd.h"
#include "tlibs/phys/lattice.h"
#include "tlibs/phys/powder.h"
#include "tlibs/phys/bz.h"
#include "tlibs/phys/neutrons.h"

#include "libs/globals.h"
#include "libs/globals_qt.h"
#include "libs/spacegroups/spacegroup.h"
#include "libs/formfactors/formfact.h"
#include "libs/spacegroups/latticehelper.h"

#include "tasoptions.h"
#include "dialogs/RecipParamDlg.h"	// for RecipParams struct
#include "dialogs/AtomsDlg.h"


#define TRIANGLE_NODE_TYPE_KEY	0

enum ScatteringTriangleNodeType
{
	NODE_Q,
	NODE_q,

	NODE_BRAGG,
	NODE_KIQ,
	NODE_KIKF,

	NODE_OTHER
};

enum EwaldSphere : int
{
	EWALD_NONE,
	EWALD_KI,
	EWALD_KF
};


class ScatteringTriangle;
class ScatteringTriangleNode : public QGraphicsItem
{
	protected:
		ScatteringTriangle *m_pParentItem;

	protected:
		virtual QRectF boundingRect() const override;
		virtual void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;

		virtual void mousePressEvent(QGraphicsSceneMouseEvent *pEvt) override;
		virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *pEvt) override;
		virtual QVariant itemChange(GraphicsItemChange change, const QVariant &val) override;

	public:
		ScatteringTriangleNode(ScatteringTriangle* pSupItem);
};

class RecipPeak : public QGraphicsItem
{
	protected:
		QColor m_color = Qt::red;
		QString m_strLabel;
		t_real_glob m_dRadius = 3.;

	protected:
		virtual QRectF boundingRect() const override;
		virtual void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;

	public:
		RecipPeak();

		void SetLabel(const QString& str) { m_strLabel = str; }
		void SetColor(const QColor& col) { m_color = col; }

		void SetRadius(t_real_glob dRad) { m_dRadius = dRad; }
		t_real_glob GetRadius() const { return m_dRadius; }
};

class ScatteringTriangleScene;
class ScatteringTriangle : public QGraphicsItem
{
	public:
		using t_powderline = typename tl::Powder<int,t_real_glob>::t_peak;

	protected:
		bool m_bReady=0, m_bUpdate=0;

		ScatteringTriangleScene &m_scene;

		std::unique_ptr<ScatteringTriangleNode> m_pNodeKiQ,
			m_pNodeKiKf, m_pNodeKfQ, m_pNodeGq;

		t_real_glob m_dScaleFactor = 150.;	// pixels per A^-1 for zoom == 1.
		t_real_glob m_dZoom = 1.;
		t_real_glob m_dPlaneDistTolerance = 0.01;
		int m_iMaxPeaks = 7;

		tl::Lattice<t_real_glob> m_lattice, m_recip;
		ublas::matrix<t_real_glob> m_matPlane, m_matPlane_inv;
		std::vector<RecipPeak*> m_vecPeaks;

		std::vector<t_powderline> m_vecPowderLines;
		std::vector<t_real_glob> m_vecPowderLineWidths;
		tl::Kd<t_real_glob> m_kdLattice;

		bool m_bShowBZ = 1;
		tl::Brillouin2D<t_real_glob> m_bz;
		tl::Brillouin3D<t_real_glob> m_bz3;
		std::vector<ublas::vector<t_real_glob>> m_vecBZ3VertsUnproj, m_vecBZ3Verts;
		std::vector<ublas::vector<t_real_glob>> m_vecBZ3SymmPts;

		bool m_bqVisible = 0;
		bool m_bShowEwaldSphere = 1;
		bool m_bEwaldAroundKi = 0;

	protected:
		virtual QRectF boundingRect() const override;

	public:
		ScatteringTriangle(ScatteringTriangleScene& scene);
		virtual ~ScatteringTriangle();

		virtual void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget *) override;

		void SetReady(bool bReady) { m_bReady = bReady; }
		void nodeMoved(const ScatteringTriangleNode* pNode=0);

		const ublas::matrix<t_real_glob>& GetPlane() const { return m_matPlane; }

		bool IsReady() const { return m_bReady; }
		t_real_glob GetTheta(bool bPosSense) const;
		t_real_glob GetTwoTheta(bool bPosSense) const;
		t_real_glob GetMonoTwoTheta(t_real_glob dMonoD, bool bPosSense) const;
		t_real_glob GetAnaTwoTheta(t_real_glob dAnaD, bool bPosSense) const;

		t_real_glob GetKi() const;
		t_real_glob GetKf() const;
		t_real_glob GetE() const;
		t_real_glob GetQ() const;
		t_real_glob Getq() const;

		t_real_glob GetAngleKiQ(bool bSense) const;
		t_real_glob GetAngleKfQ(bool bSense) const;
		t_real_glob GetAngleQVec0() const;

		void SetTwoTheta(t_real_glob dTT);
		void SetAnaTwoTheta(t_real_glob dTT, t_real_glob dAnaD);
		void SetMonoTwoTheta(t_real_glob dTT, t_real_glob dMonoD);

	public:
		bool HasPeaks() const { return m_vecPeaks.size()!=0 && m_recip.IsInited(); }
		void ClearPeaks();
		void CalcPeaks(const xtl::LatticeCommon<t_real_glob>& recipcommon, bool bIsPowder=0);

		void SetPlaneDistTolerance(t_real_glob dTol) { m_dPlaneDistTolerance = dTol; }
		void SetMaxPeaks(int iMax) { m_iMaxPeaks = iMax; }
		unsigned int GetMaxPeaks() const { return m_iMaxPeaks; }
		void SetZoom(t_real_glob dZoom);
		t_real_glob GetZoom() const { return m_dZoom; }

		void SetqVisible(bool bVisible);
		void SetBZVisible(bool bVisible);
		void SetEwaldSphereVisible(EwaldSphere iEw);

		const std::vector<t_powderline>& GetPowder() const { return m_vecPowderLines; }
		const tl::Kd<t_real_glob>& GetKdLattice() const { return m_kdLattice; }

		const tl::Brillouin3D<t_real_glob>& GetBZ3D() const { return m_bz3; }
		const std::vector<ublas::vector<t_real_glob>>& GetBZ3DPlaneVerts() const { return m_vecBZ3VertsUnproj; }
		const std::vector<ublas::vector<t_real_glob>>& GetBZ3DSymmVerts() const { return m_vecBZ3SymmPts; }

	public:
		std::vector<ScatteringTriangleNode*> GetNodes();
		std::vector<std::string> GetNodeNames() const;

		t_real_glob GetScaleFactor() const { return m_dScaleFactor; }
		void SetScaleFactor(t_real_glob dScale) { m_dScaleFactor = dScale; }

		ScatteringTriangleNode* GetNodeGq() { return m_pNodeGq.get(); }
		ScatteringTriangleNode* GetNodeKiQ() { return m_pNodeKiQ.get(); }
		ScatteringTriangleNode* GetNodeKfQ() { return m_pNodeKfQ.get(); }
		ScatteringTriangleNode* GetNodeKiKf() { return m_pNodeKiKf.get(); }

		ublas::vector<t_real_glob> GetHKLFromPlanePos(t_real_glob x, t_real_glob y) const;
		ublas::vector<t_real_glob> GetQVec(bool bSmallQ=0, bool bRLU=1) const;	// careful: check sign

		ublas::vector<t_real_glob> GetQVecPlane(bool bSmallQ=0) const;
		ublas::vector<t_real_glob> GetKiVecPlane() const;
		ublas::vector<t_real_glob> GetKfVecPlane() const;

		void RotateKiVec0To(bool bSense, t_real_glob dAngle);
		void SnapToNearestPeak(ScatteringTriangleNode* pNode,
						const ScatteringTriangleNode* pNodeOrg=0);
		bool KeepAbsKiKf(t_real_glob dQx, t_real_glob dQy);

		const tl::Lattice<t_real_glob>& GetRecipLattice() const { return m_recip; }
		QPointF GetGfxMid() const;

		void AllowMouseMove(bool bAllow);
};


class ScatteringTriangleScene : public QGraphicsScene
{	Q_OBJECT
	protected:
		std::unique_ptr<ScatteringTriangle> m_pTri;
		t_real_glob m_dMonoD = 3.355;
		t_real_glob m_dAnaD = 3.355;

		bool m_bSamplePosSense = 1;
		bool m_bAnaPosSense = 0;
		bool m_bMonoPosSense = 0;

		bool m_bDontEmitChange = 0;
		bool m_bSnap = 0;
		bool m_bSnapq = 1;
		bool m_bMousePressed = 0;

		bool m_bKeepAbsKiKf = 1;
		bool m_bSnapKiKfToElastic = 0;

	public:
		ScatteringTriangleScene(QObject *pParent=nullptr);
		virtual ~ScatteringTriangleScene();

		void SetEmitChanges(bool bEmit) { m_bDontEmitChange = !bEmit; }
		// emits triangleChanged
		void emitUpdate();
		// emits paramsChanged
		void emitAllParams();
		void SetDs(t_real_glob dMonoD, t_real_glob dAnaD);

		void SetSampleSense(bool bPos);
		void SetMonoSense(bool bPos);
		void SetAnaSense(bool bPos);

		const ScatteringTriangle* GetTriangle() const { return m_pTri.get(); }
		ScatteringTriangle* GetTriangle() { return m_pTri.get(); }

		void CheckForSpurions();

		bool ExportBZAccurate(const char* pcFile) const;

	public slots:
		void tasChanged(const TriangleOptions& opts);
		void scaleChanged(t_real_glob dTotalScale);

		void setSnapq(bool bSnap);
		bool getSnapq() const { return m_bSnapq; }

		void setKeepAbsKiKf(bool bKeep) { m_bKeepAbsKiKf = bKeep; }
		bool getKeepAbsKiKf() const { return m_bKeepAbsKiKf; }

	signals:
		// relevant parameters for instrument view
		void triangleChanged(const TriangleOptions& opts);
		// all parameters
		void paramsChanged(const RecipParams& parms);

		void spurionInfo(const tl::ElasticSpurion& spuris,
			const std::vector<tl::InelasticSpurion<t_real_glob>>& vecInelCKI,
			const std::vector<tl::InelasticSpurion<t_real_glob>>& vecInelCKF);

		void coordsChanged(t_real_glob dh, t_real_glob dk, t_real_glob dl,
			bool bHasNearest,
			t_real_glob dNearestH, t_real_glob dNearestK, t_real_glob dNearestL);

		void nodeEvent(bool bStarted);

	protected:
		virtual void mousePressEvent(QGraphicsSceneMouseEvent *pEvt) override;
		virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *pEvt) override;
		virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *pEvt) override;

		virtual void keyPressEvent(QKeyEvent *pEvt) override;
		virtual void keyReleaseEvent(QKeyEvent *pEvt) override;

		virtual void drawBackground(QPainter*, const QRectF&) override;
};


class ScatteringTriangleView : public QGraphicsView
{
	Q_OBJECT
	protected:
		t_real_glob m_dTotalScale = 1.;

		void DoZoom(t_real_glob delta);
		virtual void wheelEvent(QWheelEvent* pEvt) override;
		virtual void keyPressEvent(QKeyEvent *pEvt) override;
		virtual void keyReleaseEvent(QKeyEvent *pEvt) override;

	public:
		ScatteringTriangleView(QWidget* pParent = 0);
		virtual ~ScatteringTriangleView();

	signals:
		void scaleChanged(t_real_glob dTotalScale);
};

#endif
