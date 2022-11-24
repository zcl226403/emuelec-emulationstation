#include "GuiNetPlaySettings.h"
#include "SystemConf.h"
#include "ApiSystem.h"
#include "ThreadedHasher.h"
#include "components/SwitchComponent.h"
#include "GuiHashStart.h"
#include "views/UIModeController.h"
#include "GuiLoading.h"
#include "components/MultiLineMenuEntry.h"
#include "components/BatteryIndicatorComponent.h"
#include "guis/GuiMsgBox.h"

GuiNetPlaySettings::GuiNetPlaySettings(Window* window) : GuiSettings(window, _("NETPLAY SETTINGS").c_str())
{
	std::string port = SystemConf::getInstance()->get("global.netplay.port");
	if (port.empty())
		SystemConf::getInstance()->set("global.netplay.port", "55435");

	addGroup(_("SETTINGS"));

	auto enableNetplay = std::make_shared<SwitchComponent>(mWindow);
	enableNetplay->setState(SystemConf::getInstance()->getBool("global.netplay"));

if (UIModeController::getInstance()->isUIModeFull())
	{	
	addWithLabel(_("ENABLE NETPLAY"), enableNetplay);
	}

    addEntry(_("ENABLE NETPLAY SERVER"), true, [this] { 
    	if (ApiSystem::getInstance()->getIpAdress() == "NOT CONNECTED")
			{
				mWindow->pushGui(new GuiMsgBox(mWindow, _("YOU ARE NOT CONNECTED TO A NETWORK"), _("OK"), nullptr));
				return;
			}
    	mWindow->pushGui(new GuiMsgBox(mWindow, _("Warning: \n must connect cables, access server to be successful, \n make sure to open the server?"), _("YES"),
				[] { 
					runSystemCommand("netplay -d netplay0 -c jxz -k jxz -u 1000 -g 1000 -l 43.138.61.62:11001", "", nullptr);
				}, _("NO"), nullptr));
     });

  //  addWithLabel(_("NETPLAY IP"), _("IP ADDRESS"));

	addInputTextRow(_("NICKNAME"), "global.netplay.nickname", false);
if (UIModeController::getInstance()->isUIModeFull())
	{
	addInputTextRow(_("PORT"), "global.netplay.port", false);
	addOptionList(_("USE RELAY SERVER"), { { _("NONE"), "" },{ _("NEW YORK") , "nyc" },{ _("MADRID") , "madrid" },{ _("MONTREAL") , "montreal" },{ _("SAO PAULO") , "saopaulo" } }, "global.netplay.relay", false);
	addSwitch(_("SHOW UNAVAILABLE GAMES"), "NetPlayShowMissingGames", true);

	addGroup(_("GAME INDEXES"));

	addSwitch(_("INDEX NEW GAMES AT STARTUP"), "NetPlayCheckIndexesAtStart", true);
	addEntry(_("INDEX GAMES"), true, [this]
	{
		if (ThreadedHasher::checkCloseIfRunning(mWindow))
			mWindow->pushGui(new GuiHashStart(mWindow, ThreadedHasher::HASH_NETPLAY_CRC));
	});
	}

	Window* wnd = mWindow;
	addSaveFunc([wnd, enableNetplay]
	{
		if (SystemConf::getInstance()->setBool("global.netplay", enableNetplay->getState()))
		{
			if (!ThreadedHasher::isRunning() && enableNetplay->getState())
			{
				ThreadedHasher::start(wnd, ThreadedHasher::HASH_NETPLAY_CRC, false, true);
			}
		}
	});
}
