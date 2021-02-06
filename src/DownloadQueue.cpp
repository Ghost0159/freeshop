#include <iostream>
#include <string>
#include <cpp3ds/System/I18n.hpp>
#include <TweenEngine/Tween.h>
#include <cpp3ds/System/Sleep.hpp>
#include <cpp3ds/System/Err.hpp>
#include <cpp3ds/System/FileSystem.hpp>
#include <fstream>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <cpp3ds/System/FileInputStream.hpp>
#include <cpp3ds/System/Lock.hpp>
#include <cmath>
#include <ctime>
#include "DownloadQueue.hpp"
#include "Notification.hpp"
#include "AppList.hpp"
#include "TitleKeys.hpp"
#include "InstalledList.hpp"
#include "Config.hpp"
#include "Util.hpp"
#include "Theme.hpp"
#include "States/SleepState.hpp"
#include <inttypes.h>
#ifndef EMULATION
#include <3ds.h>
#include "MCU/Mcu.hpp"
#endif

namespace FreeShop {


DownloadItem::DownloadItem(std::shared_ptr<AppItem> appItem, Download *download, Installer *installer)
: appItem(appItem)
, download(download)
, installer(installer)
{

}

DownloadItem::~DownloadItem()
{
	if (installer)
		delete installer;
	if (download)
		delete download;
}


DownloadQueue::DownloadQueue()
: m_threadRefresh(&DownloadQueue::refresh, this)
, m_refreshEnd(false)
, m_isSleepInstalling(false)
, m_size(320.f, 0.f)
{
	if (Theme::isSoundChimeThemed)
		m_soundBufferFinish.loadFromFile(FREESHOP_DIR "/theme/sounds/chime.ogg");
	else
		m_soundBufferFinish.loadFromFile("sounds/chime.ogg");

	m_soundFinish.setBuffer(m_soundBufferFinish);

	load();
	m_threadRefresh.launch();
}

DownloadQueue::~DownloadQueue()
{
	m_refreshEnd = true;
	cpp3ds::Lock lock(m_mutexRefresh);
	m_downloads.clear();
}

void DownloadQueue::addDownload(std::shared_ptr<AppItem> app, cpp3ds::Uint64 titleId, DownloadCompleteCallback callback, int contentIndex, float progress)
{
	cpp3ds::Lock lock(m_mutexRefresh);

	cpp3ds::Uint32 type = titleId >> 32;
	if (cpp3ds::Keyboard::isKeyDown(cpp3ds::Keyboard::Select)) {
		app->queueForSleepInstall(false, type == TitleKeys::Demo);
		return;
	}

	if (titleId == 0)
	{
		if (app->isInstalled()) // Don't allow reinstalling without deleting
			return;
		titleId = app->getTitleId();
	}

#ifndef EMULATION
	//Check if the title is not a system app (because it pass the verification for some reason)
	size_t titleType = app->getTitleId() >> 32;
	if (titleType != TitleKeys::SystemApplet && titleType != TitleKeys::SystemApplication) {
		bool isRegistered;
		NIMS_IsTaskRegistered(app->getTitleId(), &isRegistered);

		if (isRegistered || app->isSleepBusy()) {
			Notification::spawn(_("Registered for sleep download: \n%s", app->getTitle().toAnsiString().c_str()));
			return;
		}
	}
#endif

	cpp3ds::String title = app->getTitle();
	if (type != TitleKeys::Game)
	{
		if (type == TitleKeys::Update)
			title = _("[Update] %s", title.toAnsiString().c_str());
		else if (type == TitleKeys::Demo)
			title = _("[Demo] %s", title.toAnsiString().c_str());
		else if (type == TitleKeys::DLC)
			title = _("[DLC] %s", title.toAnsiString().c_str());
	}

	std::string url = _("http://ccs.cdn.c.shop.nintendowifi.net/ccs/download/%016llX/tmd", titleId);
	Download* download = new Download(url);
	download->fillFromAppItem(app);
	download->m_textTitle.setString(title);
	download->setPosition(0.f, 240.f);
	download->setRetryCount(INT_MAX);
	download->setTimeout(cpp3ds::seconds(Config::get(Config::DownloadTimeout).GetFloat()));
	download->setBufferSize(1024 * Config::get(Config::DownloadBufferSize).GetUint());
	download->setSendTopCallback([this](Download *d){
		sendTop(d);
	});

	std::vector<char> buf; // Store fetched files (only used for TMD atm)
	cpp3ds::Clock clock;
	float count = 0;
	int fileIndex = 0;   // Keep track of the multiple files, first is usually the TMD with rest being contents
	size_t fileSize = 0; // Size of each file (as obtain by http Content-Length)

	Installer *installer = new Installer(titleId, contentIndex);
	cpp3ds::Uint64 titleFileSize = 0;
	cpp3ds::Uint64 totalProcessed;

	// Is resuming from saved queue
	bool isResuming = contentIndex >= 0;

	// TMD values
	cpp3ds::Uint16 contentCount;
	cpp3ds::Uint16 titleVersion;
	std::vector<cpp3ds::Uint16> contentIndices;

	download->setDataCallback([=](const void* data, size_t len, size_t processed, const cpp3ds::Http::Response& response) mutable
	{
		// This condition should be true when download first starts.
		if (len == processed) {
			std::string length = response.getField("Content-Length");
			if (!length.empty())
				fileSize = strtoul(length.c_str(), 0, 10);
		}

		const char *bufdata = reinterpret_cast<const char*>(data);

		if (fileIndex == 0)
		{
			buf.insert(buf.end(), bufdata, bufdata + len);

			if (processed == fileSize && fileSize != 0)
			{
				static int dataOffsets[6] = {0x240, 0x140, 0x80, 0x240, 0x140, 0x80};
				char sigType = buf[0x3];

				titleVersion = *(cpp3ds::Uint16*)&buf[dataOffsets[sigType] + 0x9C];
				contentCount = __builtin_bswap16(*(cpp3ds::Uint16*)&buf[dataOffsets[sigType] + 0x9E]);

				if (isResuming)
					installer->resume();

				bool foundIndex = false; // For resuming via contentIndex arg
				for (int i = 0; i < contentCount; ++i)
				{
					char *contentChunk = &buf[dataOffsets[sigType] + 0x9C4 + (i * 0x30)];
					cpp3ds::Uint32 contentId = __builtin_bswap32(*(cpp3ds::Uint32*)&contentChunk[0]);
					cpp3ds::Uint16 contentIdx = __builtin_bswap16(*(cpp3ds::Uint16*)&contentChunk[4]);
					cpp3ds::Uint64 contentSize = __builtin_bswap64(*(cpp3ds::Uint64*)&contentChunk[8]);
					titleFileSize += contentSize;
					contentIndices.push_back(contentIdx);

					if (contentIdx == contentIndex)
					{
						foundIndex = true;
						fileIndex = i + 1;
					}
					if (!isResuming || foundIndex)
						download->pushUrl(_("http://ccs.cdn.c.shop.nintendowifi.net/ccs/download/%016llX/%08lX", titleId, contentId), (contentIdx == contentIndex) ? installer->getCurrentContentPosition() : 0);
				}

				totalProcessed = progress * titleFileSize;

				if (!isResuming)
				{
					// Check for cancel at each stage in case it changes
					if (!download->isCanceled())
					{
						download->setProgressMessage(_("Installing ticket..."));
						if (!installer->installTicket(titleVersion))
							return false;
					}
					cpp3ds::Uint32 type = titleId >> 32;
					if (!download->isCanceled() && type == TitleKeys::Game && !app->getSeed().empty())
					{
						download->setProgressMessage(_("Installing seed..."));
						if (!installer->installSeed(&app->getSeed()[0]))
							return false;
					}
					if (!download->isCanceled())
					{
						if (!installer->start(true))
							return false;
						download->setProgressMessage(_("Installing TMD..."));
						if (!installer->installTmd(&buf[0], dataOffsets[sigType] + 0x9C4 + (contentCount * 0x30)))
							return false;
						if (!installer->finalizeTmd())
							return false;
					}
				}

				buf.clear();
				if (!isResuming)
					fileIndex++;
			}
		}
		else // is a Content file
		{
			// Conditions indicate download issue (e.g. internet is down)
			// with either an empty length or one not 64-byte aligned
#ifdef _3DS
			if (len == 0 || len % 64 > 0)
			{
				cpp3ds::Lock lock(m_mutexRefresh);
				download->suspend();
				installer->suspend();
				return true;
			}
#endif

			int oldIndex = installer->getCurrentContentIndex();
			if (!installer->installContent(data, len, contentIndices[fileIndex-1]))
			{
				if (download->getStatus() == Download::Suspended)
					return true;
				return false;
			}

			// Save index change to help recover queue from crash
			if (oldIndex != installer->getCurrentContentIndex())
				save();

			totalProcessed += len;
			download->setProgress(static_cast<double>(totalProcessed) / titleFileSize);

			if (processed == fileSize && fileSize != 0)
			{
				if (!installer->finalizeContent())
					return false;
				if (fileIndex == 1 && type == TitleKeys::DLC)
				{
					download->setProgressMessage(_("Importing content..."));
					if (!installer->importContents(contentIndices.size() - 1, &contentIndices[1]))
						return false;
				}
				fileIndex++;
			}
		}

		// Handle status message and counters
		count += len;
		float speed = count / clock.getElapsedTime().asSeconds() / 1024.f;
		int secsRemaining = (titleFileSize - totalProcessed) / 1024 / speed;
		if (fileIndex <= contentCount)
			download->setProgressMessage(_("Installing %d/%d... %.1f%% (%.0f KB/s) %dm %02ds",
		                                   fileIndex, contentCount,
		                                   download->getProgress() * 100.f,
		                                   speed, secsRemaining / 60, secsRemaining % 60));
		if (clock.getElapsedTime() > cpp3ds::seconds(5.f))
		{
			count = 0;
			clock.restart();
		}

		return true;
	});

	download->setFinishCallback([=](bool canceled, bool failed) mutable
	{
		bool succeeded = false;

		switch (download->getStatus())
		{
			case Download::Suspended:
				download->setProgressMessage(_("Suspended"));
				if (download->getLastResponse().getStatus() == cpp3ds::Http::Response::NotFound)
					download->m_status = Download::Failed;
				else
					return;
			case Download::Finished:
				if (installer->commit())
				{
					Notification::spawn(_("Completed: %s", app->getTitle().toAnsiString().c_str()));
					download->setProgressMessage(_("Installed"));
					succeeded = true;
#ifndef EMULATION
					if (Config::get(Config::NEWSDownloadFinished).GetBool()) {
						//Get the time to print it on Notification
						time_t t = time(NULL);
						struct tm * timeinfo;
						timeinfo = localtime(&t);
						char timeTextFmt[20];
						strftime(timeTextFmt, 20, "%d/%m/%Y %H:%M:%S", timeinfo);

						//Get the formatted game size
						std::string gameSize;
						if (app->getFilesize() > 1024 * 1024 * 1024)
							gameSize = _("%.1f GB", static_cast<float>(app->getFilesize()) / 1024.f / 1024.f / 1024.f).toAnsiString();
						else if (app->getFilesize() > 1024 * 1024)
							gameSize = _("%.1f MB", static_cast<float>(app->getFilesize()) / 1024.f / 1024.f).toAnsiString();
						else
							gameSize = _("%.1f KB", static_cast<float>(app->getFilesize()) / 1024).toAnsiString();

						//No blink functionnality
						bool canBlink = true;
						if (Config::get(Config::NEWSNoLED).GetBool() || Config::get(Config::LEDDownloadFinished).GetBool())
							canBlink = false;

						Notification::sendNews(_("%s has been installed", app->getTitle().toAnsiString().c_str()), _("The game %s (%s) has been installed on \n%s", app->getTitle().toAnsiString().c_str(), gameSize, timeTextFmt));

						if (!canBlink)
							MCU::getInstance().ledReset();
					}

					if (Config::get(Config::LEDDownloadFinished).GetBool())
						MCU::getInstance().ledBlinkOnce(0xFFA419);
#endif
					break;
				}
				download->m_status = Download::Failed;
				// Fall through
			case Download::Failed:
				Notification::spawn(_("Failed: %s", app->getTitle().toAnsiString().c_str()));
				installer->abort();

				switch (installer->getErrorCode()) {
					case 0: // Failed due to internet problem and not from Installer
						download->setProgressMessage(_("Failed: Internet problem"));
						break;
					case 0xC8A08035: download->setProgressMessage(_("Not enough space on NAND")); break;
					case 0xC86044D2: download->setProgressMessage(_("Not enough space on SD")); break;
					case 0xD8E0806A: download->setProgressMessage(_("Wrong title key")); break;
					case 0xD8A08004: download->setProgressMessage(_("Incomplete installation")); break;
					default:
						download->setProgressMessage(installer->getErrorString());
				}
				break;
			case Download::Canceled:
				break;
		}

		download->setProgress(1.f);

		// Reset CanSendTop state
		for (auto& download : m_downloads)
			download->download->m_canSendTop = true;

		// Refresh installed list to add recent install
		if (succeeded)
			InstalledList::getInstance().refresh();

		// Play sound if queue is finished
		if (getActiveCount() == 0 && !canceled)
			if ((Config::get(Config::PlaySoundAfterDownload).GetBool() && !SleepState::isSleeping) || (SleepState::isSleeping && Config::get(Config::PlaySoundAfterDownload).GetBool() && Config::get(Config::SoundOnInactivity).GetBool()))
				m_soundFinish.play();

		if (callback)
			callback(succeeded);
	});

	if (progress > 0.f)
	{
		download->setProgress(progress);
		download->setProgressMessage(_("Suspended"));
		download->m_status = Download::Suspended;
	}
	else
		download->setProgressMessage(_("Queued"));

	std::unique_ptr<DownloadItem> downloadItem(new DownloadItem(app, download, installer));
	m_downloads.emplace_back(std::move(downloadItem));
	realign();
}

void DownloadQueue::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	states.transform *= getTransform();
	states.scissor = cpp3ds::UintRect(0, 30, 320, 210);

	for (auto& item : m_downloads)
	{
		static float top = 30.f - item->download->getSize().y;
		float posY = item->download->getPosition().y + m_scrollPos;
		if (posY > top && posY < 240.f)
			target.draw(*item->download, states);
	}
}


void DownloadQueue::restartDownload(Download *download)
{
	for (auto& item : m_downloads)
		if (item->download == download && item->download->getStatus() == Download::Failed)
		{
			item->download->m_markedForDelete = true;
			addDownload(item->appItem, item->installer->getTitleId());
			break;
		}
}


bool DownloadQueue::isDownloading(std::shared_ptr<AppItem> app)
{
	for (auto& item : m_downloads)
		if (item->appItem == app)
		{
			auto status = item->download->getStatus();
			if (status == Download::Queued || status == Download::Downloading || status == Download::Suspended)
				return true;
		}

	return false;
}


bool DownloadQueue::isDownloading(cpp3ds::Uint64 titleId)
{
	for (auto& item : m_downloads)
		if (item->installer->getTitleId() == titleId)
		{
			auto status = item->download->getStatus();
			if (status == Download::Queued || status == Download::Downloading || status == Download::Suspended)
				return true;
		}
	return false;
}


bool DownloadQueue::processEvent(const cpp3ds::Event &event)
{
	for (auto& item : m_downloads)
	{
		static float top = 30.f - item->download->getSize().y;
		float posY = item->download->getPosition().y + m_scrollPos;
		if (posY > top && posY < 240.f)
			item->download->processEvent(event);
	}
	return true;
}

void DownloadQueue::realign()
{
	bool processedFirstItem = false;
	m_size.y = 0.f;
	for (int i = 0; i < m_downloads.size(); ++i)
	{
		Download *download = m_downloads[i]->download;
		Download::Status status = download->getStatus();
		if (download->markedForDelete())
			continue;
		if (processedFirstItem)
		{
			download->m_canSendTop = (status == Download::Queued || status == Download::Suspended || status == Download::Downloading);
		}
		else if (status == Download::Queued || status == Download::Suspended || status == Download::Downloading)
		{
			processedFirstItem = true;
			download->m_canSendTop = false;
		}

		TweenEngine::Tween::to(*download, util3ds::TweenSprite::POSITION_Y, 0.2f)
			.target(33.f + i * download->getSize().y)
			.start(m_tweenManager);

		m_size.y += download->getSize().y;
	}
	updateScrollSize();
	save();
}

void DownloadQueue::update(float delta)
{
	// Remove downloads marked for delete
	{
		bool changed = false;
		cpp3ds::Lock lock(m_mutexRefresh);
		for (auto it = m_downloads.begin(); it != m_downloads.end();)
		{
			Download *download = it->get()->download;
			if (download->markedForDelete())
			{
				if (download->getStatus() == Download::Suspended)
					it->get()->installer->abort();
				m_downloads.erase(it);
				changed = true;
			}
			else
				++it;

		if (!download->markedForDelete())
			download->update(delta);
		}
		if (changed)
			realign();
	}

	m_tweenManager.update(delta);
}

size_t DownloadQueue::getCount()
{
	return m_downloads.size();
}

size_t DownloadQueue::getActiveCount()
{
	size_t count = 0;
	for (auto& item : m_downloads)
		if (item->download->getProgress() < 1.f)
			++count;
	return count;
}

DownloadQueue &DownloadQueue::getInstance()
{
	static DownloadQueue downloadQueue;
	return downloadQueue;
}

void DownloadQueue::sendTop(Download *download)
{
	cpp3ds::Lock lock(m_mutexRefresh);
	auto iterTopDownload = m_downloads.end();
	auto iterBottomDownload = m_downloads.end();
	for (auto it = m_downloads.begin(); it != m_downloads.end(); ++it)
	{
		if (iterTopDownload == m_downloads.end())
		{
			Download::Status status = it->get()->download->getStatus();
			if (status == Download::Downloading || status == Download::Queued || status == Download::Suspended)
				iterTopDownload = it;
		}
		else if (iterBottomDownload == m_downloads.end())
		{
			if (it->get()->download == download)
				iterBottomDownload = it;
		}
	}

	if (iterTopDownload != m_downloads.end() && iterBottomDownload != m_downloads.end())
	{
		std::rotate(iterTopDownload, iterBottomDownload, iterBottomDownload + 1);
		realign();
		m_clockRefresh.restart();
	}
}

void DownloadQueue::refresh()
{
	while (!m_refreshEnd)
	{
		{
			cpp3ds::Lock lock(m_mutexRefresh);
			DownloadItem *activeDownload = nullptr;
			DownloadItem *firstQueued = nullptr;
			bool activeNeedsSuspension = false;
			for (auto& item : m_downloads)
			{
				Download::Status status = item->download->getStatus();
				if (!firstQueued && (status == Download::Queued || status == Download::Suspended))
				{
					firstQueued = item.get();
				}
				else if (!activeDownload && status == Download::Downloading)
				{
					activeDownload = item.get();
					if (firstQueued)
						activeNeedsSuspension = true;
				}
			}

			if ((!activeDownload && firstQueued) || activeNeedsSuspension)
			{
				if (activeNeedsSuspension)
				{
					activeDownload->download->suspend();
					activeDownload->installer->suspend();
				}

				firstQueued->installer->resume();
				firstQueued->download->resume();
				realign();
			}
		}

		m_clockRefresh.restart();
		while (m_clockRefresh.getElapsedTime() < cpp3ds::seconds(1.f))
			cpp3ds::sleep(cpp3ds::milliseconds(200));
	}
}

void DownloadQueue::suspend()
{
	cpp3ds::Lock lock(m_mutexRefresh);
	m_refreshEnd = true;
	if (m_isSleepInstalling)
		m_sleepInstaller->suspend();
	for (auto& item : m_downloads)
	{
		item->download->suspend();
		item->installer->suspend();
	}
}

void DownloadQueue::resume()
{
	m_refreshEnd = false;
	if (m_isSleepInstalling)
		m_sleepInstaller->resume();
	m_threadRefresh.launch();
}

void DownloadQueue::save()
{
	rapidjson::Document json;
	std::string filepath = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/queue.json");
	std::ofstream file(filepath);
	rapidjson::OStreamWrapper osw(file);
	rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
	std::vector<std::string> subTitleIds; // Strings need to be in memory for json allocator write

	json.SetArray();
	for (auto& item : m_downloads)
	{
		Download::Status status = item->download->getStatus();

		if (status != Download::Downloading && status != Download::Queued && status != Download::Suspended)
			continue;

		rapidjson::Value obj(rapidjson::kObjectType);
		cpp3ds::Uint32 titleType = item->installer->getTitleId() >> 32;
		subTitleIds.push_back(_("%016llX", item->installer->getTitleId()).toAnsiString());

		obj.AddMember("title_id", rapidjson::StringRef(item->appItem->getTitleIdStr().c_str()), json.GetAllocator());
		obj.AddMember("subtitle_id", rapidjson::StringRef(subTitleIds.back().c_str()), json.GetAllocator());

		// TODO: Figure out how to properly resume DLC installation
		if (titleType == TitleKeys::DLC) {
			obj.AddMember("content_index", rapidjson::Value().SetInt(-1), json.GetAllocator());
			obj.AddMember("progress", rapidjson::Value().SetFloat(0.f), json.GetAllocator());
		} else {
			obj.AddMember("content_index", rapidjson::Value().SetInt(status == Download::Queued ? -1 : item->installer->getCurrentContentIndex()), json.GetAllocator());
			obj.AddMember("progress", rapidjson::Value().SetFloat(item->download->getProgress()), json.GetAllocator());
		}
		json.PushBack(obj, json.GetAllocator());
	}

	json.Accept(writer);
}

void DownloadQueue::load()
{
	uint32_t pendingTitleCountSD = 0;
	uint32_t pendingTitleCountNAND = 0;
	std::vector<uint64_t> pendingTitleIds;
#ifndef EMULATION
	Result res = 0;
	if (R_SUCCEEDED(res = AM_GetPendingTitleCount(&pendingTitleCountSD, MEDIATYPE_SD, AM_STATUS_MASK_INSTALLING)))
		if (R_SUCCEEDED(res = AM_GetPendingTitleCount(&pendingTitleCountNAND, MEDIATYPE_NAND, AM_STATUS_MASK_INSTALLING)))
		{
			pendingTitleIds.resize(pendingTitleCountSD + pendingTitleCountNAND);
			res = AM_GetPendingTitleList(nullptr, pendingTitleCountSD, MEDIATYPE_SD, AM_STATUS_MASK_INSTALLING, &pendingTitleIds[0]);
			res = AM_GetPendingTitleList(nullptr, pendingTitleCountNAND, MEDIATYPE_NAND, AM_STATUS_MASK_INSTALLING, &pendingTitleIds[pendingTitleCountSD]);
		}
#endif
	cpp3ds::FileInputStream file;
	if (file.open(FREESHOP_DIR "/queue.json"))
	{
		rapidjson::Document json;
		std::string jsonStr;
		jsonStr.resize(file.getSize());
		file.read(&jsonStr[0], jsonStr.size());
		json.Parse(jsonStr.c_str());
		if (!json.HasParseError())
		{
			auto &list = AppList::getInstance().getList();
			for (auto it = json.Begin(); it != json.End(); ++it)
			{
				auto item = it->GetObject();
				std::string strTitleId = item["title_id"].GetString();
				std::string strSubTitleId = item["subtitle_id"].GetString();
				int contentIndex = item["content_index"].GetInt();
				float progress = item["progress"].GetFloat();

				uint64_t titleId = strtoull(strTitleId.c_str(), 0, 16);
				uint64_t subTitleId = strtoull(strSubTitleId.c_str(), 0, 16);
				size_t parentType = titleId >> 32;
#ifdef _3DS
				for (auto pendingTitleId : pendingTitleIds)
					if (contentIndex == -1 || pendingTitleId == titleId || pendingTitleId == subTitleId)
					{
#endif
						// For system titles, the relevant AppItem won't be in AppList
						if (parentType == TitleKeys::SystemApplet || parentType == TitleKeys::SystemApplication)
							for (auto &app : InstalledList::getInstance().getList())
							{
								if (app->getTitleId() == titleId)
									addDownload(app->getAppItem(), subTitleId, nullptr, contentIndex, progress);
							}
						else
							for (auto &app : list)
								if (app->getAppItem()->getTitleId() == titleId)
									addDownload(app->getAppItem(), subTitleId, nullptr, contentIndex, progress);
#ifdef _3DS
						break;
					}
#endif
			}
		}
	}
}

void DownloadQueue::addSleepDownload(std::shared_ptr<AppItem> app, cpp3ds::Uint64 titleId, cpp3ds::String title)
{
#ifndef EMULATION
	//Lock the thread so if user wants to install another app while an app is processed, this doesn't fu** up everything (and make ARM9 crashes (true story))
	cpp3ds::Lock lock(m_mutexSleepInstall);

	//A sleep installation is processed
	m_isSleepInstalling = true;

	//Get title type
	cpp3ds::Uint32 type = titleId >> 32;

	// Check if enough space to install the title
	FS_ArchiveResource resource = {0};
	Result ret;

	if (type == TitleKeys::DSiWare)
		ret = FSUSER_GetArchiveResource(&resource, SYSTEM_MEDIATYPE_TWL_NAND);
	else
		ret = FSUSER_GetArchiveResource(&resource, SYSTEM_MEDIATYPE_SD);

	if(R_SUCCEEDED(ret)) {
		u64 size = (u64) resource.freeClusters * (u64) resource.clusterSize;

		if (app->getFilesize() > size) {
			if (Config::get(Config::LEDDownloadError).GetBool())
				MCU::getInstance().ledBlinkThrice(0x1A25FF);

			if (type == TitleKeys::DSiWare)
				Notification::spawn(_("Not enough space on NAND to install: \n%s", app->getTitle().toAnsiString().c_str()));
			else
				Notification::spawn(_("Not enough space on SD card to install: \n%s", app->getTitle().toAnsiString().c_str()));

			return;
		}
	}

	if (titleId == 0)
	{
		if (app->isInstalled()) { // Don't allow reinstalling without deleting
			m_isSleepInstalling = false;
			return;
		}
		titleId = app->getTitleId();
	}

	bool isRegistered;
	NIMS_IsTaskRegistered(titleId, &isRegistered);

	//Get title name
	cpp3ds::String appTitle;
	if (title == "")
		appTitle = app->getTitle();
	else
		appTitle = title;

	//Add prefix to the game
	if (type == TitleKeys::Update)
		appTitle = _("[Update] %s", appTitle.toAnsiString().c_str());
	else if (type == TitleKeys::Demo)
		appTitle = _("[Demo] %s", appTitle.toAnsiString().c_str());
	else if (type == TitleKeys::DLC)
		appTitle = _("[DLC] %s", appTitle.toAnsiString().c_str());
	std::string stdAppTitle = appTitle.toAnsiString();

	if (isRegistered) {
		Notification::spawn(_("Ready for sleep installation: \n%s", app->getTitle().toAnsiString().c_str()));
		m_isSleepInstalling = false;
		return;
	}

	Notification::spawn(_("Processing sleep installation: \n%s", stdAppTitle.c_str()));

	//Some vars initialisation
	uint32_t res;
	NIM_TitleConfig tc;
	m_sleepInstaller = new Installer(titleId, 0);
	FS_MediaType mediaType = ((titleId >> 32) == TitleKeys::DSiWare) ? MEDIATYPE_NAND : MEDIATYPE_SD;
	cpp3ds::Uint32 titleType = titleId >> 32;

	//Get the title version (and not ticket version) via the title ID
	res = getTicketVersion(titleId);
	printf("%" PRIu32 "\n", res);

	//Check at each step that the user didn't went to HOME Menu
	if (m_refreshEnd) {
		while (m_refreshEnd)
			cpp3ds::sleep(cpp3ds::milliseconds(100));
	}

	//Install app ticket
	if (!m_sleepInstaller->installTicket(res)) {
		Notification::spawn(_("Can't install ticket: \n%s", stdAppTitle.c_str()));
		m_isSleepInstalling = false;
		return;
	}

	//Check at each step that the user didn't went to HOME Menu
	if (m_refreshEnd) {
		while (m_refreshEnd)
			cpp3ds::sleep(cpp3ds::milliseconds(100));
	}

	//Install app seed
	if (titleType == TitleKeys::Game && !app->getSeed().empty()) {
		if (!m_sleepInstaller->installSeed(&app->getSeed()[0])) {
			Notification::spawn(_("Can't install seed: \n%s", stdAppTitle.c_str()));
			m_isSleepInstalling = false;
			return;
		}
	}

	//Check at each step that the user didn't went to HOME Menu
	if (m_refreshEnd) {
		while (m_refreshEnd)
			cpp3ds::sleep(cpp3ds::milliseconds(100));
	}

	//The code that register the sleep download
	NIMS_MakeTitleConfig(&tc, titleId, res, 0, mediaType);

	if (R_SUCCEEDED(res = NIMS_RegisterTask(&tc, stdAppTitle.c_str(), "freeShop download"))) {
		Notification::spawn(_("Ready for sleep installation: \n%s", stdAppTitle.c_str()));

		if (Config::get(Config::LEDDownloadFinished).GetBool())
			MCU::getInstance().ledBlinkOnce(0x8EEBA0);
	} else {
		Notification::spawn(_("Sleep installation failed: \n%s", stdAppTitle.c_str()));

		if (Config::get(Config::LEDDownloadError).GetBool())
			MCU::getInstance().ledBlinkThrice(0x1A25FF);
	}

	/*The clear and simple explanation of this (LOOOONNNGGGGGGG) condition:
	If the "Play sound after downoad queue finishes" option IS enabled AND the freeShop is NOT on it's sleeping state, the sound can be played
	If the "Play sound after downoad queue finishes" option IS enabled AND the freeShop IS on it's sleeping state AND the "Allow sounds on inactivity mode" option IS enabled, the sound can be played*/
	if ((Config::get(Config::PlaySoundAfterDownload).GetBool() && !SleepState::isSleeping) || (SleepState::isSleeping && Config::get(Config::PlaySoundAfterDownload).GetBool() && Config::get(Config::SoundOnInactivity).GetBool()))
		m_soundFinish.play();

	m_isSleepInstalling = false;
#endif
}

void DownloadQueue::setScroll(float position)
{
	m_scrollPos = position;
	setPosition(0.f, std::round(position));
}

float DownloadQueue::getScroll()
{
	return m_scrollPos;
}

const cpp3ds::Vector2f &DownloadQueue::getScrollSize()
{
	return m_size;
}

} // namespace FreeShop
