#include "pickbottom.h"
#include "pickbottomcommand.h"
#include "autopickbottomcommand.h"
#include "kernel/kernelui.h"

using namespace creative_kernel;
PickBottom::PickBottom(QObject* parent)
	:QObject(parent)
	, m_command(nullptr),m_autocommand(nullptr)
{
}

PickBottom::~PickBottom()
{
}

QString PickBottom::name()
{
	return "";
}

QString PickBottom::info()
{
	return "";
}

void PickBottom::initialize()
{
	if (!m_command)
		m_command = new PickBottomCommand(this);
		getKernelUI()->addToolCommand(m_command,
			qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_MAIN,
			qtuser_qml::ToolCommandType::PICK_BUTTON);
	/*if(!m_autocommand)
		m_autocommand = new AutoPickBottomCommand(this);
		getKernelUI()->addToolCommand(m_autocommand,
			qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_MAIN,
			qtuser_qml::ToolCommandType::PICK_BUTTON);*/
	//addLCommand(m_command);
}

void PickBottom::uninitialize()
{
  getKernelUI()->removeToolCommand(m_command, qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_MAIN);
  //getKernelUI()->removeToolCommand(m_autocommand, qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_MAIN);
	//removeLCommand(m_command);
}
