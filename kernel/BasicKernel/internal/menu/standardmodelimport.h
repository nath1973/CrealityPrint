#ifndef STANDARDMODELIMPORT_H
#define STANDARDMODELIMPORT_H
#include "menu/actioncommand.h"
#include "menu/actioncommandmodel.h"
#include "data/interface.h"
#include <QtCore/QSettings>

namespace creative_kernel
{
    class BASIC_KERNEL_API StandardModelImport : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
        Q_PROPERTY(QAbstractListModel* subMenuActionmodel READ subMenuActionmodel CONSTANT)
    public:
        enum ShapeType
        {
            ST_CUBOID, //������
            ST_SPHERICAL, //����
            ST_CYLINDER,//Բ����
            ST_SPHERICALSHELL,//���
            ST_CONE,//Բ׶��
            ST_TRUNCATEDCONE,//��Բ׶
            ST_RING,//Բ��
            ST_PYRAMID,//��׶
            ST_PRISM,//����
            //ST_RING,//Ring
        };

        explicit StandardModelImport(ShapeType shapeType);
        virtual ~StandardModelImport();

        Q_INVOKABLE void execute();
        void initActionModel();
        QAbstractListModel* subMenuActionmodel();

    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

    private:
        void setActionNameByType(ShapeType type);
        ActionCommandModel* m_actionModelList = nullptr;
        ShapeType m_ShapeType;
    };
}

#endif // STANDARDMODELIMPORT_H
