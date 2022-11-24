#include "GuiNetPlaySettings.h"
#include "SystemConf.h"
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
	addWithLabel(_("ENABLE NETPLAY"), enableNetplay);

    addEntry(_("CLOUD BACKUP SETTINGS AND GAME SAVES"), true, [this] { 
    mWindow->pushGui(new GuiMsgBox(mWindow, _("WARNING THIS WILL RESTART EMULATIONSTATION!\n\nThis will backup your game saves, savestates and emuelec configs to the cloud service configured on rclone.conf\n\nBACKUP TO CLOUD AND RESTART?"), _("YES"),
				[] { 
				runSystemCommand("systemd-run /usr/bin/emuelec-utils ee_cloud_backup backup", "", nullptr);
				}, _("NO"), nullptr));
     });

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
