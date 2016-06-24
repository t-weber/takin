/*
 * TAS layout
 * @author tweber
 * @date feb-2014
 * @license GPLv2
 */

#ifndef __TAS_LAYOUT_H__
#define __TAS_LAYOUT_H__

#include "tlibs/helper/flags.h"
#include "libs/globals.h"
#include "libs/globals_qt.h"

#include <cmath>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsTextItem>
#include <QWheelEvent>
#if QT_VER>=5
	#include <QtWidgets>
#endif

#include "tasoptions.h"
#include "dialogs/RealParamDlg.h"	// for RealParams struct
#include "dialogs/RecipParamDlg.h"	// for RecipParams struct


class TasLayout;
class TasLayoutNode : public QGraphicsItem
{
	protected:
		TasLayout *m_pParentItem;

	protected:
		virtual QRectF boundingRect() const override;
		virtual void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;

		virtual QVariant itemChange(GraphicsItemChange change, const QVariant &val) override;

	public:
		TasLayoutNode(TasLayout* pSupItem);
};


class TasLayoutScene;
class TasLayout : public QGraphicsItem
{
	protected:
		bool m_bReady = 0;
		TasLayoutScene& m_scene;

		TasLayoutNode *m_pSrc = 0;
		TasLayoutNode *m_pMono = 0;
		TasLayoutNode *m_pSample = 0;
		TasLayoutNode *m_pAna = 0;
		TasLayoutNode *m_pDet = 0;

		t_real_glob m_dMonoTwoTheta = 3.1415/2.;
		t_real_glob m_dAnaTwoTheta = 3.1415/2.;
		t_real_glob m_dTwoTheta = 3.1415/2.;
		t_real_glob m_dTheta = 3.1415/4.;
		t_real_glob m_dAngleKiQ = 3.1415/4.;

		t_real_glob m_dLenMonoSample = 150.;
		t_real_glob m_dLenSampleAna = 100.;
		t_real_glob m_dLenAnaDet = 50.;
		t_real_glob m_dLenSample = 25.;

		t_real_glob m_dScaleFactor = 1.4; // pixels per cm for zoom == 1
		t_real_glob m_dZoom = 1.;

		bool m_bRealQVisible = 1;
		bool m_bAllowChanges = 1;

	public:
		t_real_glob GetMonoTwoTheta() const { return m_dMonoTwoTheta; }
		t_real_glob GetMonoTheta() const { return m_dMonoTwoTheta/2.; }
		t_real_glob GetAnaTwoTheta() const { return m_dAnaTwoTheta; }
		t_real_glob GetAnaTheta() const { return m_dAnaTwoTheta/2.; }
		t_real_glob GetSampleTwoTheta() const { return m_dTwoTheta; }
		t_real_glob GetSampleTheta() const { return m_dTheta; }
		t_real_glob GetLenMonoSample() const { return m_dLenMonoSample; }
		t_real_glob GetLenSampleAna() const { return m_dLenSampleAna; }
		t_real_glob GetLenAnaDet() const { return m_dLenAnaDet; }

	protected:
		virtual QRectF boundingRect() const override;

	public:
		TasLayout(TasLayoutScene& scene);
		virtual ~TasLayout();

		bool IsReady() const { return m_bReady; }

		virtual void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;

		void SetSampleTwoTheta(t_real_glob dTT);
		void SetSampleTheta(t_real_glob dT);
		void SetMonoTwoTheta(t_real_glob dTT);
		void SetAnaTwoTheta(t_real_glob dTT);
		void SetAngleKiQ(t_real_glob dKiQ);

		void SetReady(bool bReady) { m_bReady = bReady; }
		void nodeMoved(const TasLayoutNode* pNode=0);

		void AllowChanges(bool bAllow) { m_bAllowChanges = bAllow; };
		void AllowMouseMove(bool bAllow);

		void SetZoom(t_real_glob dZoom);
		t_real_glob GetZoom() const { return m_dZoom; }

	public:
		std::vector<TasLayoutNode*> GetNodes();
		std::vector<std::string> GetNodeNames() const;

		t_real_glob GetScaleFactor() const { return m_dScaleFactor; }
		void SetScaleFactor(t_real_glob dScale) { m_dScaleFactor = dScale; }

		void SetRealQVisible(bool bVisible);
		bool GetRealQVisible() const { return m_bRealQVisible; }
};


class TasLayoutScene : public QGraphicsScene
{	Q_OBJECT
	protected:
		TasLayout *m_pTas;
		bool m_bDontEmitChange = 1;

	protected:
		virtual void mousePressEvent(QGraphicsSceneMouseEvent *pEvt) override;
		virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *pEvt) override;


	public:
		TasLayoutScene(QObject *pParent=nullptr);
		virtual ~TasLayoutScene();

		void SetEmitChanges(bool bEmit) { m_bDontEmitChange = !bEmit; }
		void emitUpdate(const TriangleOptions& opts);

		TasLayout* GetTasLayout() { return m_pTas; }

		void emitAllParams();

	public slots:
		void triangleChanged(const TriangleOptions& opts);
		void scaleChanged(t_real_glob dTotalScale);

		void recipParamsChanged(const RecipParams& params);

	signals:
		// relevant parameters for triangle view
		void tasChanged(const TriangleOptions& opts);
		// all parameters
		void paramsChanged(const RealParams& parms);

		void nodeEvent(bool bStarted);
};


class TasLayoutView : public QGraphicsView
{
	Q_OBJECT
	protected:
		t_real_glob m_dTotalScale = 1.;
		virtual void wheelEvent(QWheelEvent* pEvt) override;

	public:
		TasLayoutView(QWidget* pParent = 0);
		virtual ~TasLayoutView();

	signals:
		void scaleChanged(t_real_glob dTotalScale);
};


#endif
