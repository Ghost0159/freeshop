#include "BotInformations.hpp"
#include "AssetManager.hpp"
#include "DownloadQueue.hpp"
#include "Notification.hpp"
#include "Theme.hpp"
#include "States/StateIdentifiers.hpp"
#include "States/DialogState.hpp"
#include <cpp3ds/System/I18n.hpp>
#include <time.h>
#ifndef EMULATION
#include <3ds.h>
#endif

namespace FreeShop {

BotInformations::BotInformations()
: m_threadRefresh(&BotInformations::refresh, this)
{
	//Texts
	m_textSD.setString(_("SD"));
	if (Theme::isTextThemed)
		m_textSD.setFillColor(Theme::primaryTextColor);
	else
		m_textSD.setFillColor(cpp3ds::Color::Black);
	m_textSD.setCharacterSize(10);
	m_textSD.setPosition(2.f, 31.f);

	m_textSDStorage.setString(_(""));
	if (Theme::isTextThemed)
		m_textSDStorage.setFillColor(Theme::secondaryTextColor);
	else
		m_textSDStorage.setFillColor(cpp3ds::Color(130, 130, 130, 255));
	m_textSDStorage.setCharacterSize(10);
	m_textSDStorage.setPosition(2.f, 43.f);

	m_textNAND.setString(_("TWL NAND"));
	if (Theme::isTextThemed)
		m_textNAND.setFillColor(Theme::primaryTextColor);
	else
		m_textNAND.setFillColor(cpp3ds::Color::Black);
	m_textNAND.setCharacterSize(10);
	m_textNAND.setPosition(2.f, 59.f);

	m_textNANDStorage.setString(_(""));
	if (Theme::isTextThemed)
		m_textNANDStorage.setFillColor(Theme::secondaryTextColor);
	else
		m_textNANDStorage.setFillColor(cpp3ds::Color(130, 130, 130, 255));
	m_textNANDStorage.setCharacterSize(10);
	m_textNANDStorage.setPosition(2.f, 71.f);

	//Progress bars
	m_progressBarNAND.setFillColor(cpp3ds::Color(0, 0, 0, 50));
	m_progressBarNAND.setPosition(0.f, 58.f);
	m_progressBarNAND.setSize(cpp3ds::Vector2f(0, 29));

	m_progressBarSD.setFillColor(cpp3ds::Color(0, 0, 0, 50));
	m_progressBarSD.setPosition(0.f, 30.f);
	m_progressBarSD.setSize(cpp3ds::Vector2f(0, 29));

	//Initializing booleans for transitions
	m_isProgressSDTransitioning = false;
	m_isProgressNANDTransitioning = false;

	//Progress bars' backgrounds
	if (Theme::isFSBGNAND9Themed)
		m_backgroundNAND.setTexture(&AssetManager<cpp3ds::Texture>::get(FREESHOP_DIR "/theme/images/fsbgnand.9.png"));
	else
		m_backgroundNAND.setTexture(&AssetManager<cpp3ds::Texture>::get("images/fsbgnand.9.png"));

	m_backgroundNAND.setSize(320, 23);
	m_backgroundNAND.setPosition(0.f, 58.f);

	if (Theme::isFSBGSD9Themed)
		m_backgroundSD.setTexture(&AssetManager<cpp3ds::Texture>::get(FREESHOP_DIR "/theme/images/fsbgsd.9.png"));
	else
		m_backgroundSD.setTexture(&AssetManager<cpp3ds::Texture>::get("images/fsbgsd.9.png"));

	m_backgroundSD.setSize(320, 23);
	m_backgroundSD.setPosition(0.f, 30.f);

	//Sleep download instruction text
	m_textSleepDownloads.setString(_(""));
	if (Theme::isTextThemed)
		m_textSleepDownloads.setFillColor(Theme::secondaryTextColor);
	else
		m_textSleepDownloads.setFillColor(cpp3ds::Color(0, 0, 0, 120));
	m_textSleepDownloads.setCharacterSize(11);
	m_textSleepDownloads.setPosition(2.f, 190.f);
	m_textSleepDownloads.useSystemFont();

	m_updateClock.restart();
}

BotInformations::~BotInformations()
{

}

void BotInformations::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	states.transform *= getTransform();

	//Firstly, draw the backgrounds for progress bars
	target.draw(m_backgroundNAND);
	target.draw(m_backgroundSD);

	//Secondly, draw the progress bars
	target.draw(m_progressBarNAND);
	target.draw(m_progressBarSD);

	//Finally, draw all the texts
	target.draw(m_textSD);
	target.draw(m_textSDStorage);
	target.draw(m_textNAND);
	target.draw(m_textNANDStorage);
	target.draw(m_textSleepDownloads);
}

void BotInformations::update(float delta)
{
	if (m_updateClock.getElapsedTime() >= cpp3ds::seconds(2.f)) {
		// Reset the clock
		m_updateClock.restart();

		m_threadRefresh.wait();
		m_threadRefresh.launch();
	}

	m_tweenManager.update(delta);
}

void BotInformations::refresh()
{
	//Update filesystems size and their progress bars
	#ifndef EMULATION
	FS_ArchiveResource resource = {0};

	if(R_SUCCEEDED(FSUSER_GetArchiveResource(&resource, SYSTEM_MEDIATYPE_SD))) {
		u64 size = (u64) resource.freeClusters * (u64) resource.clusterSize;
		u64 totalSize = (u64) resource.totalClusters * (u64) resource.clusterSize;

		u64 usedSize = totalSize - size;

		//m_progressBarSD.setSize(cpp3ds::Vector2f((usedSize * 320) / totalSize, 26));
		if (!m_isProgressSDTransitioning) {
			m_isProgressSDTransitioning = true;
			TweenEngine::Tween::to(m_progressBarSD, util3ds::TweenRectangleShape::SIZE, 0.2f)
				.target((usedSize * 320) / totalSize, 26.f)
				.setCallback(TweenEngine::TweenCallback::COMPLETE, [=](TweenEngine::BaseTween* source) {
					m_isProgressSDTransitioning = false;
				})
				.start(m_tweenManager);
			}

			if (usedSize > 1024 * 1024 * 1024 || totalSize > 1024 * 1024 * 1024)
				m_textSDStorage.setString(_("%.1f/%.1f GB", static_cast<float>(size) / 1024.f / 1024.f / 1024.f, static_cast<float>(totalSize) / 1024.f / 1024.f / 1024.f));
			else if (usedSize > 1024 * 1024 || totalSize > 1024 * 1024)
				m_textSDStorage.setString(_("%.1f/%.1f MB", static_cast<float>(size) / 1024.f / 1024.f, static_cast<float>(totalSize) / 1024.f / 1024.f));
			else
				m_textSDStorage.setString(_("%d/%d KB", size / 1024, totalSize / 1024));

			if ((usedSize * 320) / totalSize >= 304) {
				m_textSD.setString(_("SD - Storage almost full"));
				m_textSD.setStyle(cpp3ds::Text::Italic);
			} else {
				m_textSD.setString(_("SD"));
				m_textSD.setStyle(cpp3ds::Text::Regular);
			}
		} else {
			m_textSDStorage.setString("No SD Card detected");
	}

	if(R_SUCCEEDED(FSUSER_GetArchiveResource(&resource, SYSTEM_MEDIATYPE_TWL_NAND))) {
		u64 size = (u64) resource.freeClusters * (u64) resource.clusterSize;
		u64 totalSize = (u64) resource.totalClusters * (u64) resource.clusterSize;

		u64 usedSize = totalSize - size;

		//m_progressBarNAND.setSize(cpp3ds::Vector2f((usedSize * 320) / totalSize, 26));
		if (!m_isProgressNANDTransitioning) {
			m_isProgressNANDTransitioning = true;
			TweenEngine::Tween::to(m_progressBarNAND, util3ds::TweenRectangleShape::SIZE, 0.2f)
				.target((usedSize * 320) / totalSize, 26.f)
				.setCallback(TweenEngine::TweenCallback::COMPLETE, [=](TweenEngine::BaseTween* source) {
					m_isProgressNANDTransitioning = false;
				})
				.start(m_tweenManager);
		}

		if (usedSize > 1024 * 1024 * 1024 || totalSize > 1024 * 1024 * 1024)
			m_textNANDStorage.setString(_("%.1f/%.1f GB", static_cast<float>(size) / 1024.f / 1024.f / 1024.f, static_cast<float>(totalSize) / 1024.f / 1024.f / 1024.f));
		else if (usedSize > 1024 * 1024 || totalSize > 1024 * 1024)
			m_textNANDStorage.setString(_("%.1f/%.1f MB", static_cast<float>(size) / 1024.f / 1024.f, static_cast<float>(totalSize) / 1024.f / 1024.f));
		else
			m_textNANDStorage.setString(_("%d/%d KB", size / 1024, totalSize / 1024));

		if ((usedSize * 320) / totalSize >= 304) {
			m_textNAND.setString(_("TWL NAND - Storage almost full"));
			m_textNAND.setStyle(cpp3ds::Text::Italic);
		} else {
			m_textNAND.setString(_("TWL NAND"));
			m_textNAND.setStyle(cpp3ds::Text::Regular);
		}
	} else {
		m_textNANDStorage.setString("No TWL NAND detected... Wait what ?!");
	}
	#else
	m_textSDStorage.setString("16/32 GB");
	m_progressBarSD.setSize(cpp3ds::Vector2f(320, 26));

	m_textNANDStorage.setString("200/400 MB");
	m_progressBarNAND.setSize(cpp3ds::Vector2f(160, 26));
	#endif

	// Update the sleep downloads text
	#ifndef EMULATION
	// Init vars
	u32 pendingTitleCountSD = 0;
	u32 pendingTitleCountNAND = 0;
	u32 pendingTitleCountTotal = 0;
	std::vector<cpp3ds::Uint64> pendingTitleIds;

	// Get pending title count for SD (3ds Titles) and NAND (TWL Titles)
	AM_GetPendingTitleCount(&pendingTitleCountSD, MEDIATYPE_SD, AM_STATUS_MASK_INSTALLING | AM_STATUS_MASK_AWAITING_FINALIZATION);
	AM_GetPendingTitleCount(&pendingTitleCountNAND, MEDIATYPE_NAND, AM_STATUS_MASK_INSTALLING | AM_STATUS_MASK_AWAITING_FINALIZATION);

	// Get the total count
	pendingTitleCountTotal = pendingTitleCountSD + pendingTitleCountNAND;

	// Get title IDs of pending titles
	pendingTitleIds.resize(pendingTitleCountTotal);
	AM_GetPendingTitleList(nullptr, pendingTitleCountSD, MEDIATYPE_SD, AM_STATUS_MASK_INSTALLING | AM_STATUS_MASK_AWAITING_FINALIZATION, &pendingTitleIds[0]);
	AM_GetPendingTitleList(nullptr, pendingTitleCountNAND, MEDIATYPE_NAND, AM_STATUS_MASK_INSTALLING | AM_STATUS_MASK_AWAITING_FINALIZATION, &pendingTitleIds[pendingTitleIds.size() - pendingTitleCountNAND]);

	// Get pending title count for titles that are on the encTitleKey.bin file
	pendingTitleCountTotal = 0;
	for (auto& titleId : pendingTitleIds)
		if (TitleKeys::get(titleId))
			pendingTitleCountTotal++;

	if (pendingTitleCountTotal > 0)
		m_textSleepDownloads.setString(_("%i sleep download pending\nClose the software with the Start button and close\nthe lid of your console.", pendingTitleCountTotal));
	else
		m_textSleepDownloads.setString(_(""));
	#else
	m_textSleepDownloads.setString(_("%i sleep download pending\nClose the software with the Start button and close\nthe lid of your console.", rand() % 11));
	#endif
}

} // namespace FreeShop
