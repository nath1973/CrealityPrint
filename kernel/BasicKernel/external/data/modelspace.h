#ifndef creative_kernel_MODEL_SPACE_H
#define creative_kernel_MODEL_SPACE_H
#include "basickernelexport.h"
#include "data/interface.h"
#include "cxkernel/data/modelndata.h"

#include <QtCore/QList>

#include "data/modelgroup.h"
#include <Qt3DCore/QEntity>

namespace creative_kernel
{
	class BASIC_KERNEL_API ModelSpace : public QObject
		, public cxkernel::ModelNDataProcessor
	{
		Q_OBJECT
		Q_PROPERTY(int modelNNum READ modelNNum NOTIFY modelNNumChanged)
	public:
		ModelSpace(QObject* parent = nullptr);
		~ModelSpace();

		void initialize();
		void uninitialize();

		void addSpaceTracer(SpaceTracer* tracer);
		void removeSpaceTracer(SpaceTracer* tracer);

		qtuser_3d::Box3D baseBoundingBox();
		qtuser_3d::Box3D sceneBoundingBox();
		void setBaseBoundingBox(const qtuser_3d::Box3D& box);

		void onModelSceneChanged();

		void addModelGroup(ModelGroup* modelGroup);  // add a model group
		void removeModelGroup(ModelGroup* modelGroup);  // remove a model group
		void addModel(ModelN* model);
		void removeModel(ModelN* model);

		QList<ModelN*> modelns();
		QString mainModelName();
		int modelNNum();
		bool haveModelN();

		Qt3DCore::QEntity* rootEntity();
		ModelGroup* currentGroup();
		QList<ModelGroup*> modelGroups();

		void setSpaceDirty(bool spaceDirty);
		bool spaceDirty();

		void appendResizeModel(ModelN* model);

		Q_INVOKABLE void adaptTempModels();
		Q_INVOKABLE void clearTempModels();
		Q_INVOKABLE void ignoreTempModels();
		Q_INVOKABLE bool haveModelsOutPlatform();
		Q_INVOKABLE bool modelOutPlatform(ModelN* amodel);

		QList<ModelN*> getModelnsBySerialName(const QStringList& names);
		ModelN* getModelNBySerialName(const QString& name);
		ModelN* getModelNByObjectName(const QString& objectName);
		
		void uniformName(ModelN* item);

		void modelMeshLoadStarted(int iMeshNum) override;
		void onMeshLoadFail() override;

		void checkCollide();
		int checkModelRange();
		int checkBedRange();

		bool checkLayerHeightEqual(const std::vector<std::vector<double>>& objectsLayers);

	signals:
		void signalVisualChanged(bool capture);
		void modelNNumChanged();
		void sigAddModel();
		void sigRemoveModel();


	protected:
		void process(cxkernel::ModelNDataPtr data) override;

	private:
		void layoutAddedModel(ModelN* aModel);
		bool checkModelSizeForLayout(ModelN* aModel);
		void reCheckLayoutWhenLoadingFinish();

	private:
		qtuser_3d::Box3D m_baseBoundingBox;

		Qt3DCore::QEntity* m_root;
		ModelGroup* m_currentModelGroup;

		QList<SpaceTracer*> m_spaceTracers;

		bool m_spaceDirty;
		QList<ModelN*> m_needResizeModels;

		int m_loadMeshCount;
		QList<ModelN*> m_loadedModels;
		bool m_layoutDoneFlag;
	};
}
#endif // creative_kernel_MODEL_SPACE_H