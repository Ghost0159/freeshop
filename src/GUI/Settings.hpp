#ifndef FREESHOP_SETTINGS_HPP
#define FREESHOP_SETTINGS_HPP

#include <Gwen/Controls.h>
#include <Gwen/Controls/RadioButtonController.h>
#include <Gwen/Input/cpp3ds.h>
#include <Gwen/Skins/TexturedBase.h>
#include <TweenEngine/Tweenable.h>
#include <cpp3ds/System/Vector2.hpp>
#include "../States/State.hpp"
#include "../Config.hpp"


namespace FreeShop
{
	namespace GUI
	{
		class Settings: public Gwen::Event::Handler, public TweenEngine::Tweenable
		{
		public:
			static const int POSITION_XY = 1;

			Settings(Gwen::Skin::TexturedBase *skin, State *state);
			~Settings();

			bool processEvent(const cpp3ds::Event &event);
			bool update(float delta);
			void render();

			void setPosition(const cpp3ds::Vector2f &position);
			const cpp3ds::Vector2f &getPosition() const;

			void saveToConfig();
			void loadConfig();

			virtual int getValues(int tweenType, float *returnValues);
			virtual void setValues(int tweenType, float *newValues);

			void playMusic(); // For recovery from Sleep state

		private:
			void saveFilter(Config::Key key, std::vector<Gwen::Controls::CheckBoxWithLabel*> &checkboxArray);
			void loadFilter(Config::Key key, std::vector<Gwen::Controls::CheckBoxWithLabel*> &checkboxArray);
			void saveSort(Config::Key key, std::vector<Gwen::Controls::LabeledRadioButton*> &radioArray);
			void loadSort(Config::Key key, std::vector<Gwen::Controls::LabeledRadioButton*> &radioArray);
			void updateEnabledStates();

			Gwen::Controls::ScrollControl *addFilterPage(const std::string &name);
			void fillFilterRegions(Gwen::Controls::Base *parent);
			void fillFilterGenres(Gwen::Controls::Base *parent);
			void fillFilterPlatforms(Gwen::Controls::Base *parent);
			void fillFilterLanguages(Gwen::Controls::Base *parent);
			void fillFilterFeature(Gwen::Controls::Base *parent);
			void fillFilterPublisher(Gwen::Controls::Base *parent);

			Gwen::Controls::ScrollControl *addSortPage(const std::string &name);
			void fillSortGameList(Gwen::Controls::Base *parent);
			void fillSortInstalledList(Gwen::Controls::Base *parent);

			void fillSortPage(Gwen::Controls::Base *page);
			void fillUpdatePage(Gwen::Controls::Base *page);
			void fillDownloadPage(Gwen::Controls::Base *page);
			void fillMusicPage(Gwen::Controls::Base *page);
			void fillLocalesPage(Gwen::Controls::Base *page);
			void fillThemeInfoPage(Gwen::Controls::Base *page);
			void fillThemeSettingsPage(Gwen::Controls::Base *page);
			void ifThemeSettingsIsPressed(Gwen::Controls::Base *page);
			void fillNotifiersPage(Gwen::Controls::Base *page);
			void fillInactivityPage(Gwen::Controls::Base *page);
			void fillOtherPage(Gwen::Controls::Base *page);

			// Event Callback functions
			void selectAll(Gwen::Controls::Base* control);
			void selectNone(Gwen::Controls::Base* control);

			bool m_ignoreCheckboxChange;
			void filterCheckboxChanged(Gwen::Controls::Base* control);
			void filterRegionCheckboxChanged(Gwen::Controls::Base* control);
			void filterSaveClicked(Gwen::Controls::Base *base);
			void filterClearClicked(Gwen::Controls::Base *base);

			void updateUrlSelected(Gwen::Controls::Base *combobox);
			void updateQrClicked(Gwen::Controls::Base *button);
			void updateKeyboardClicked(Gwen::Controls::Base *button);
			void updateUrlDeleteClicked(Gwen::Controls::Base *button);
			void updateCheckUpdateClicked(Gwen::Controls::Base *button);
			void updateRefreshCacheClicked(Gwen::Controls::Base *button);

			void downloadTimeoutChanged(Gwen::Controls::Base *base);
			void downloadBufferSizeChanged(Gwen::Controls::Base *base);
			void downloadUseDefaultsClicked(Gwen::Controls::Base *base);
			void downloadPowerOffClicked(Gwen::Controls::Base *base);

			void updateEnabledState(Gwen::Controls::Base* control);
			void updateSorting(Gwen::Controls::Base* control);
			void sortSaveClicked(Gwen::Controls::Base *base);
			void sortClearClicked(Gwen::Controls::Base *base);

			void musicComboChanged(Gwen::Controls::Base *combobox);
			void musicFileChanged(Gwen::Controls::Base* base);
			void selectRandomMusicTrack();
			void playSelectedMusic();
			void resetEshopMusicClicked(Gwen::Controls::Base *button);

			void languageChange(Gwen::Controls::Base* base);
			void keyboardChange(Gwen::Controls::Base* base);
			void systemKeyboardChange(Gwen::Controls::Base* base);

			void inactivityTimeChanged(Gwen::Controls::Base* base);

			void showNews(Gwen::Controls::Base* base);

			void askUserToRestartApp();
			void darkThemeRestart();

		private:
			cpp3ds::Vector2f m_position;

			State *m_state;
			Gwen::Input::cpp3dsInput m_input;
			Gwen::Controls::Canvas *m_canvas;
			Gwen::Controls::TabControl *m_tabControl;
			std::string m_countryCode;

			// Filter
			Gwen::Controls::TabControl *m_filterTabControl;
			std::vector<Gwen::Controls::CheckBoxWithLabel*> m_filterGenreCheckboxes;
			std::vector<Gwen::Controls::CheckBoxWithLabel*> m_filterPlatformCheckboxes;
			std::vector<Gwen::Controls::CheckBoxWithLabel*> m_filterRegionCheckboxes;
			std::vector<Gwen::Controls::CheckBoxWithLabel*> m_filterLanguageCheckboxes;
			std::vector<Gwen::Controls::CheckBoxWithLabel*> m_filterFeatureCheckboxes;
			std::vector<Gwen::Controls::CheckBoxWithLabel*> m_filterPublisherCheckboxes;
			Gwen::Controls::Button *m_buttonFilterSave;
			Gwen::Controls::Button *m_buttonFilterSaveClear;

			// Sort
			Gwen::Controls::TabControl *m_sortTabControl;
			Gwen::Controls::RadioButtonController *m_radioSortType;
			Gwen::Controls::RadioButtonController *m_radioSortDirection;
			Gwen::Controls::RadioButtonController *m_radioSortTypeInstalled;
			Gwen::Controls::RadioButtonController *m_radioSortDirectionInstalled;
			std::vector<Gwen::Controls::LabeledRadioButton*> m_sortGameListRadioButtons;
			std::vector<Gwen::Controls::LabeledRadioButton*> m_sortInstalledListRadioButtons;
			std::vector<Gwen::Controls::LabeledRadioButton*> m_sortGameListRadioButtonsDirection;
			std::vector<Gwen::Controls::LabeledRadioButton*> m_sortInstalledListRadioButtonsDirection;
			Gwen::Controls::Button *m_buttonFilterSaveSort;
			Gwen::Controls::Button *m_buttonFilterSaveClearSort;

			// Update
			Gwen::Controls::CheckBoxWithLabel *m_checkboxAutoUpdate;
			Gwen::Controls::CheckBoxWithLabel *m_checkboxDownloadKeys;
			Gwen::Controls::ComboBox *m_comboBoxUrls;
			Gwen::Controls::Button *m_buttonUrlQr, *m_buttonUrlKeyboard, *m_buttonUrlDelete;
			Gwen::Controls::Button *m_buttonUpdate;
			Gwen::Controls::Label *m_labelLastUpdated;
			Gwen::Controls::CheckBoxWithLabel *m_checkboxDownloadMultipleKeys;

			// Download
			Gwen::Controls::Label *m_labelTimeout;
			Gwen::Controls::Label *m_labelDownloadBufferSize;
			Gwen::Controls::HorizontalSlider *m_sliderTimeout;
			Gwen::Controls::HorizontalSlider *m_sliderDownloadBufferSize;
			Gwen::Controls::CheckBoxWithLabel *m_checkboxPlaySound;
			Gwen::Controls::CheckBoxWithLabel *m_checkboxPowerOff;
			Gwen::Controls::ComboBox *m_comboPowerOffTime;

			// Music
			Gwen::Controls::ComboBox *m_comboMusicMode;
			Gwen::Controls::ListBox *m_listboxMusicFiles;
			Gwen::Controls::Label *m_labelAudioState;
			Gwen::Controls::CheckBoxWithLabel *m_checkboxTurnOffMusicSlider;

			// Locales
			Gwen::Controls::ListBox *m_listboxLanguages;
			Gwen::Controls::ListBox *m_listboxKeyboards;
			Gwen::Controls::CheckBoxWithLabel *m_checkboxSystemKeyboard;

			// Theme
			Gwen::Controls::Label *m_labelThemeName;
			Gwen::Controls::Label *m_labelThemeVersion;
			Gwen::Controls::Label *m_labelThemeDescription;
			Gwen::Controls::CheckBoxWithLabel *m_checkboxDarkTheme;

			// Notifiers
			Gwen::Controls::CheckBoxWithLabel *m_checkboxLEDStartup;
			Gwen::Controls::CheckBoxWithLabel *m_checkboxLEDDownloadFinished;
			Gwen::Controls::CheckBoxWithLabel *m_checkboxLEDDownloadErrored;
			Gwen::Controls::CheckBoxWithLabel *m_checkboxNEWSDownloadFinished;
			Gwen::Controls::CheckBoxWithLabel *m_checkboxNEWSNoLED;

			// Inactivity
			Gwen::Controls::CheckBoxWithLabel *m_checkboxSleep;
			Gwen::Controls::CheckBoxWithLabel *m_checkboxSleepBottom;
			Gwen::Controls::CheckBoxWithLabel *m_checkboxDimLEDs;
			Gwen::Controls::CheckBoxWithLabel *m_checkboxInactivitySoundAllowed;
			Gwen::Controls::CheckBoxWithLabel *m_checkboxInactivityMusicAllowed;
			Gwen::Controls::Label *m_labelInactivityTime;
			Gwen::Controls::HorizontalSlider *m_sliderInactivityTime;

			// Other
			Gwen::Controls::CheckBoxWithLabel *m_checkboxTitleID;
			Gwen::Controls::CheckBoxWithLabel *m_checkboxBatteryPercent;
			Gwen::Controls::CheckBoxWithLabel *m_checkboxGameCounter;
			Gwen::Controls::CheckBoxWithLabel *m_checkboxGameDescription;
			Gwen::Controls::Button *m_buttonResetEshopMusic;
			Gwen::Controls::CheckBoxWithLabel *m_checkboxRestartFix;
		};
	}
}


#endif //FREESHOP_SETTINGS_HPP
