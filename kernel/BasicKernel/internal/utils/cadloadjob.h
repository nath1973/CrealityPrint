#ifndef _CREATIVE_KERNEL_CADLOADJOB_1590984808257_H
#define _CREATIVE_KERNEL_CADLOADJOB_1590984808257_H
#include "qtusercore/module/job.h"
#include "cxkernel/kernel/data.h"

namespace creative_kernel
{
	class CADLoadJob: public qtuser_core::Job
	{
	public:
		CADLoadJob(QObject* parent = nullptr);
		virtual ~CADLoadJob();

		void setFileName(const QString& fileName);
	protected:
		QString name();
		QString description();
		void failed();                        // invoke from main thread
		void successed(qtuser_core::Progressor* progressor);                     // invoke from main thread
		void work(qtuser_core::Progressor* progressor);    // invoke from worker thread

	protected:
		QString m_fileName;
		TriMeshPtr  m_mesh;
	};
}
#endif // _CREATIVE_KERNEL_MESHLOADJOB_1590984808257_H
