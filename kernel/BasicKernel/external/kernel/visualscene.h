#ifndef CREATIVE_KERNEL_VISUALSCENE_1592188654160_H
#define CREATIVE_KERNEL_VISUALSCENE_1592188654160_H
#include "data/header.h"
#include "data/kernelenum.h"
#include "data/interface.h"
#include "qtuser3d/framegraph/rendergraph.h"
#include "qtuser3d/framegraph/surface.h"
#include <QtCore/QTimer>

namespace qtuser_3d
{
	class Surface;
	class FacePicker;
	class ColorPicker;
	class TextureRenderTarget;
	class RectLineEntity;
	class XEntity;
}

namespace creative_kernel
{
	class VisSceneHandler;
	class WipeTowerHandler;

	class BASIC_KERNEL_API VisualScene : public qtuser_3d::RenderGraph
		, public KernelPhase
	{
		Q_OBJECT
	signals:
		void sceneChanged();

	public:
		VisualScene(Qt3DCore::QNode* parent = nullptr);
		virtual ~VisualScene();

		bool isDefaultScene();
		void useDefaultScene();
		void useCustomScene();
		void showCustom(Qt3DCore::QEntity* entity);
		void hideCustom(Qt3DCore::QEntity* entity);

		void show(Qt3DCore::QEntity* entity);
		void hide(Qt3DCore::QEntity* entity);
		void enableHandler(bool enable);

		qtuser_3d::FacePicker* facePicker();
		qtuser_3d::TextureRenderTarget* textureRenderTarget();

		void initialize();

		bool shouldMultipleSelect();
		bool didSelectAnyEntity(const QPoint& p);
		void showRectangleSelector(const QRect& rect);
		void dismissRectangleSelector();

		void delayCapture(int msec);
		
		Qt3DRender::QFrameGraphNode* getCameraViewportFrameGraphNode();

		QSize surfaceSize();

		void setSceneClearColor(const QColor& color);

		void updatePick(bool sync);

	public slots:
		void updateRender(bool updatePick = false);

	public:
		Qt3DCore::QEntity* sceneGraph() override;

		void begineRender() override;
		void endRender() override;
		void updateRenderSize(const QSize& size) override;

		void onStartPhase() override;
		void onStopPhase() override;
#ifdef ENABLE_DEBUG_OVERLAY
		bool showDebugOverlay() override;
		void setShowDebugOverlay(bool showDebugOverlay) override;
#endif

		qtuser_3d::XEntity* heightReferEntity();
	private:
		qtuser_3d::Surface* m_surface;
		qtuser_3d::ColorPicker* m_colorPicker;
		qtuser_3d::TextureRenderTarget* m_textureRenderTarget;
		qtuser_3d::RectLineEntity* m_rectLine;
		qtuser_3d::XEntity* m_heightReferEntity;

		Qt3DCore::QEntity* m_rootEntity;
		Qt3DCore::QEntity* m_customRootEntity;

		VisSceneHandler* m_handler;
		WipeTowerHandler* m_towerHandler;

		bool m_handlerEnabled;

		QTimer* m_updateTimer;
	};
}
#endif // CREATIVE_KERNEL_VISUALSCENE_1592188654160_H
