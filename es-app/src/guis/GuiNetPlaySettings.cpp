#include "GuiNetPlaySettings.h"
#include "GuiSystemInformation.h"
#include "SystemConf.h"
#include "ApiSystem.h"
#include "ThreadedHasher.h"
#include "components/SwitchComponent.h"
#include "components/OptionListComponent.h"
#include "GuiHashStart.h"
#include "views/UIModeController.h"
#include "GuiLoading.h"
#include "components/MultiLineMenuEntry.h"
#include "components/BatteryIndicatorComponent.h"
#include "guis/GuiMsgBox.h"
#include "ThemeData.h"
#include "guis/GuiSettings.h"


GuiNetPlaySettings::GuiNetPlaySettings(Window* window) : GuiSettings(window, _("NETPLAY SETTINGS").c_str())
{
	std::string port = SystemConf::getInstance()->get("global.netplay.port");
	if (port.empty())
		SystemConf::getInstance()->set("global.netplay.port", "55435");

	//std::string OTHERIP = SystemConf::getInstance()->get("global.netplay.opspip");//jiancha shuru de IP,ranhou fuzhi
	//if (OTHERIP.empty())
	//SystemConf::getInstance()->set("global.netplay.opspip", "116.116.0.10");

	runSystemCommand("systemd-run /usr/bin/newjb s_netplay_ip", "", nullptr);
	std::string jxznetplay = std::string(getShOutput(R"(/usr/bin/newjb xg_netplay_ip2)"));
	//std::string jxznetplay = SystemConf::getInstance()->get("global.jxznetplay.ip");
	if (jxznetplay.empty())
		SystemConf::getInstance()->set("global.jxznetplay.ip", "NONE");
	else
		SystemConf::getInstance()->set("global.jxznetplay.ip", jxznetplay);

	addGroup(_("SETTINGS"));
	addInputTextRow(_("NICKNAME"), "global.netplay.nickname", false);
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
				[this] { 
					//runSystemCommand("systemd-run /usr/bin/newjb xg_netplay_ip", "", nullptr);
					std::string jxznetplay2 = std::string(getShOutput(R"(/usr/bin/newjb xg_netplay_ip2)"));
					if (jxznetplay2.empty())
					{
						runSystemCommand("nohup netplay -d netplay -c jxz -k jxz -u 1000 -g 1000 -l 139.9.249.246:11001 > /dev/null 2>&1 &", "", nullptr);//dalu
						mWindow->pushGui(new GuiMsgBox(mWindow, _("In connection...")));
					}
					else
					{
						mWindow->pushGui(new GuiMsgBox(mWindow, _("Has launched the online server"), _("OK"), nullptr));
						return;
					}

				}, _("NO"), nullptr));
     });

    auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

    auto emuelec_netplay_max_connections = std::make_shared< OptionListComponent<std::string> >(mWindow, _("MAX CONNECTIONS"), false);
	emuelec_enetplay_max_connections->addRange({ { _("AUTO"), "" },{ "1", "1" },{ "2", "2" },{ "3", "3" },{ "4", "4" },{ "5", "5" },{ "6", "6" },{ "7", "7" },{ "8", "8" },{ "9", "9" },{ "10", "10" },{ "11", "11" },{ "12", "12" },{ "13", "13" },{ "14", "14" },{ "15", "15" },{ "16", "16" },{ "17", "17" },{ "18", "18" },{ "19", "19" },{ "20", "20" },{ "21", "21" },{ "22", "22" },{ "23", "23" },{ "24", "24" },{ "25", "25" },{ "26", "26" },{ "27", "27" },{ "28", "28" },{ "29", "29" },{ "30", "30" } }, SystemConf::getInstance()->get("netplay.max.connections"));
	addWithDescription(_("MAX CONNECTIONS"), _("The maximum number of people allowed in your room is 4 by default."), emuelec_netplay_max_connections);
	emuelec_netplay_max_connections->setSelectedChangedCallback([emuelec_netplay_max_connections](std::string name) { 
        if (SystemConf::getInstance()->set("netplay.max.connections", name)) 
            SystemConf::getInstance()->saveSystemConf();
        });

	auto netplay_input_latency_frames_range = std::make_shared< OptionListComponent<std::string> >(mWindow, _("INPUT LATENCY FRAMES RANGE"), false);
	netplay_input_latency_frames_range->addRange({ { "0", "0" },{ "1", "1" },{ "2", "2" },{ "3", "3" },{ "4", "4" } }, SystemConf::getInstance()->get("netplay.latency.frames"));
	addWithDescription(_("INPUT LATENCY FRAMES RANGE"), _("The higher the value, the smoother the online, and the higher the delay. The default recommended value is 2."), netplay_input_latency_frames_range);
	netplay_input_latency_frames_range->setSelectedChangedCallback([netplay_input_latency_frames_range](std::string name) { 
        if (SystemConf::getInstance()->set("netplay.latency.frames", name)) 
            SystemConf::getInstance()->saveSystemConf();
        });

	auto NetPlayIP = std::make_shared<TextComponent>(mWindow, SystemConf::getInstance()->get("global.jxznetplay.ip"), font, color);
    addWithLabel(_("NETPLAY IP"), NetPlayIP);
    //addInputTextRow(_("NETPLAY IP"), "global.jxznetplay.ip", false);
    
    auto status = std::make_shared<TextComponent>(mWindow, ApiSystem::getInstance()->ping() ? _("CONNECTED") : _("NOT CONNECTED"), font, color);
	addWithLabel(_("INTERNET STATUS"), status);	

	//addInputTextRow(_("NICKNAME"), "global.netplay.nickname", false);

	addGroup(_("PSP"));

	addEntry(_("Map Server IP to PSP"), true, [this] //fuzhi zhuji IP gei psp
	{
		mWindow->pushGui(new GuiMsgBox(mWindow, _("You are about to map the IP address of the server to the local PSP as the host of the PSP. Are you sure to map?"), _("YES"),
			[this] {
				std::string jxznetplay3 = std::string(getShOutput(R"(/usr/bin/newjb xg_netplay_ip2)"));
				if (jxznetplay3.empty())
				{
					mWindow->pushGui(new GuiMsgBox(mWindow, _("You do not have started online server"), _("OK"), nullptr));
					return;
				}
				runSystemCommand("systemd-run /usr/bin/newjb server_netplay_psp_ip", "", nullptr);
				mWindow->pushGui(new GuiMsgBox(mWindow, _("Server IP mapping completed"), _("OK"), nullptr));
			}, _("NO"), nullptr));
	});
	addInputTextRow(_("other PSP IP"), "global.netplay.opspip", false);

	addEntry(_("Map Other IP to PSP"), true, [this] //fuzhi duifang IP gei psp
	{
		mWindow->pushGui(new GuiMsgBox(mWindow, _("You will map the IP of the other party to the local PSP as the secondary host of the other party. Are you sure you want to map?"), _("YES"),
			[this] {
				std::string jxznetplay4 = std::string(getShOutput(R"(/usr/bin/newjb xg_netplay_ip2)"));
				if (jxznetplay4.empty())
				{
					mWindow->pushGui(new GuiMsgBox(mWindow, _("You do not have started online server"), _("OK"), nullptr));
					return;
				}
				runSystemCommand("systemd-run /usr/bin/newjb other_netplay_psp_ip", "", nullptr);
				mWindow->pushGui(new GuiMsgBox(mWindow, _("The opposite IP mapping is completed"), _("OK"), nullptr));
			}, _("NO"), nullptr));
	});


if (UIModeController::getInstance()->isUIModeFull())
{	
	addInputTextRow(_("PORT"), "global.netplay.port", false);
	addOptionList(_("USE RELAY SERVER"), { { _("NONE"), "" },{ _("NEW YORK") , "nyc" },{ _("MADRID") , "madrid" },{ _("MONTREAL") , "montreal" },{ _("SAO PAULO") , "saopaulo" } }, "global.netplay.relay", false);
	addSwitch(_("SHOW UNAVAILABLE GAMES"), _("Show rooms for games not present on this machine."), "NetPlayShowMissingGames", true, nullptr);

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
