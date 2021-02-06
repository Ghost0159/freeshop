#include "FreeShop.hpp"
#include "DownloadQueue.hpp"
#include "TitleKeys.hpp"
#include "Config.hpp"
#include "States/BrowseState.hpp"
#include "States/SleepState.hpp"
#include "States/SyncState.hpp"

#ifndef EMULATION
#include <3ds.h>
#include "MCU/Mcu.hpp"
#endif

#ifndef EMULATION
namespace {

aptHookCookie cookie;

void aptHookFunc(APT_HookType hookType, void *param)
{
	switch (hookType) {
		case APTHOOK_ONSUSPEND:
			if (FreeShop::SleepState::isSleeping && R_SUCCEEDED(gspLcdInit()))
			{
				GSPLCD_PowerOnBacklight(GSPLCD_SCREEN_BOTH);
				gspLcdExit();
			}

			if (FreeShop::SleepState::isSleeping && R_SUCCEEDED(FreeShop::MCU::getInstance().mcuInit())) {
				FreeShop::MCU::getInstance().dimLeds(0xFF);
				FreeShop::MCU::getInstance().mcuExit();
			}
			// Fall through
		case APTHOOK_ONSLEEP:
			FreeShop::DownloadQueue::getInstance().suspend();
			break;
		case APTHOOK_ONRESTORE:
		case APTHOOK_ONWAKEUP:
			FreeShop::SleepState::isSleeping = false;
			FreeShop::SleepState::clock.restart();
			FreeShop::BrowseState::clockDownloadInactivity.restart();
			FreeShop::DownloadQueue::getInstance().resume();
			FreeShop::g_browseState->wokeUp();
			break;
		case APTHOOK_ONEXIT:
			FreeShop::SyncState::exitRequired = true;
			break;
		default:
			break;
	}
}

}
#endif


int main(int argc, char** argv)
{
#ifndef NDEBUG
	// Console for reading stdout
	cpp3ds::Console::enable(cpp3ds::BottomScreen, cpp3ds::Color::Black);
#endif

	cpp3ds::Service::enable(cpp3ds::Audio);
	cpp3ds::Service::enable(cpp3ds::Config);
	cpp3ds::Service::enable(cpp3ds::Network);
	cpp3ds::Service::enable(cpp3ds::SSL);
	cpp3ds::Service::enable(cpp3ds::Httpc);
	cpp3ds::Service::enable(cpp3ds::AM);

#ifndef EMULATION
	aptHook(&cookie, aptHookFunc, nullptr);
	AM_InitializeExternalTitleDatabase(false);
	aptSetSleepAllowed(true);
#endif
	srand(time(NULL));
	auto game = new FreeShop::FreeShop();
	game->run();
	FreeShop::FreeShop::prepareToCloseApp();
#ifndef EMULATION
	nimsExit();
	amExit();
	ptmuExit();
	acExit();
	ptmSysmExit();
	newsExit();

	if (FreeShop::g_requestShutdown) {
		//Init the services and turn off the console
		ptmSysmInit();
		PTMSYSM_ShutdownAsync(0);
		ptmSysmExit();
	}

	if (FreeShop::g_requestReboot) {
		//Init the services and reboot the console
		ptmSysmInit();
		PTMSYSM_RebootAsync(0);
		ptmSysmExit();
	}
#endif
	delete game;
	return 0;
}
