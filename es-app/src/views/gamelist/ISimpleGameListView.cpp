#include "views/gamelist/ISimpleGameListView.h"

#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "Settings.h"
#include "Sound.h"
#include "SystemData.h"
#include "SystemConf.h"
#include "guis/GuiMsgBox.h"
#include "Window.h"
#include "LocaleES.h"
#include "guis/GuiSettings.h"
#include <set>
#include "components/SwitchComponent.h"
#include "ApiSystem.h"
#include "animations/LambdaAnimation.h"
#include "guis/GuiGameOptions.h"
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "SaveStateRepository.h"
#include "guis/GuiSaveState.h"
#include "guis/GuiGamelistOptions.h"
#include "BasicGameListView.h"
#include "utils/Randomizer.h"
#include "views/Binding.h"
#include "guis/GuiImageViewer.h"
#include "guis/GuiGameAchievements.h"

ISimpleGameListView::ISimpleGameListView(Window* window, FolderData* root, bool temporary) : IGameListView(window, root),
	mHeaderText(window), mHeaderImage(window), mBackground(window), mFolderPath(window), mOnExitPopup(nullptr),
	mYButton("y"), mXButton("x"), mOKButton("OK"), mSelectButton("select")
{
	mExtraMode = ThemeData::ExtraImportType::ALL_EXTRAS;

	mHeaderText.setText("Logo Text");
	mHeaderText.setSize(mSize.x(), 0);
	mHeaderText.setPosition(0, 0);
	mHeaderText.setHorizontalAlignment(ALIGN_CENTER);
	mHeaderText.setDefaultZIndex(50);
	
	mHeaderImage.setResize(0, mSize.y() * 0.185f);
	mHeaderImage.setOrigin(0.5f, 0.0f);
	mHeaderImage.setPosition(mSize.x() / 2, 0);
	mHeaderImage.setDefaultZIndex(50);

	mBackground.setResize(mSize.x(), mSize.y());
	mBackground.setDefaultZIndex(0);

	mFolderPath.setHorizontalAlignment(ALIGN_CENTER);
	mFolderPath.setDefaultZIndex(55);
	mFolderPath.setVisible(false);

	addChild(&mHeaderText);
	addChild(&mBackground);
	addChild(&mFolderPath);

}


ISimpleGameListView::~ISimpleGameListView()
{
	for (auto extra : mThemeExtras)
		delete extra;
}

void ISimpleGameListView::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
	using namespace ThemeFlags;
	mBackground.applyTheme(theme, getName(), "background", ALL);
	mHeaderImage.applyTheme(theme, getName(), "logo", ALL);
	mHeaderText.applyTheme(theme, getName(), "logoText", ALL);
	mFolderPath.applyTheme(theme, getName(), "folderpath", ALL);

	// Remove old theme extras
	for (auto extra : mThemeExtras)
	{
		removeChild(extra);
		delete extra;
	}
	mThemeExtras.clear();

	// Add new theme extras
	mThemeExtras = ThemeData::makeExtras(theme, getName(), mWindow, false, mExtraMode);

	for (auto extra : mThemeExtras)
		addChild(extra);

	if(mHeaderImage.hasImage())
	{
		removeChild(&mHeaderText);
		addChild(&mHeaderImage);
	}
	else
	{
		addChild(&mHeaderText);
		removeChild(&mHeaderImage);
	}
}

void ISimpleGameListView::onFileChanged(FileData* /*file*/, FileChangeType /*change*/)
{
	// we could be tricky here to be efficient;
	// but this shouldn't happen very often so we'll just always repopulate
	FileData* cursor = getCursor();
	if (!cursor->isPlaceHolder()) 
	{
		populateList(cursor->getParent()->getChildrenListToDisplay());
		setCursor(cursor);
	}
	else
	{
		while (mCursorStack.size())
			mCursorStack.pop();

		populateList(mRoot->getChildrenListToDisplay());
		setCursor(cursor);
	}
}

void ISimpleGameListView::moveToFolder(FolderData* folder)
{
	if (folder == nullptr || folder->getChildren().size() == 0)
		return;
	
	mCursorStack.push(folder);
	populateList(folder->getChildrenListToDisplay());
	
	FileData* cursor = getCursor();
	if (cursor != nullptr)
		setCursor(cursor);	
}

void ISimpleGameListView::toggleFavoritesFilter()
{
	FileData* cursor = getCursor();
	if (cursor == nullptr)
		return;

	auto system = cursor->getSystem();

	if (system->isGroupChildSystem())
		system = system->getParentGroupSystem();

	auto index = system->getIndex(true);
	if (index == nullptr)
		return;

	static std::vector<std::string> trueFilter = { "TRUE" };

	if (index->getFilter(FilterIndexType::FAVORITES_FILTER) == nullptr)
		index->setFilter(FilterIndexType::FAVORITES_FILTER, &trueFilter);
	else
		index->setFilter(FilterIndexType::FAVORITES_FILTER, nullptr);

	ViewController::get()->reloadGameListView(system);
}

FolderData* ISimpleGameListView::getCurrentFolder()
{
	if (mCursorStack.size())
	{
		auto top = mCursorStack.top();
		if (top->getType() == FOLDER)
			return (FolderData*)top;
	}

	return nullptr;
}

void ISimpleGameListView::update(const int deltaTime)
{
	GuiComponent::update(deltaTime);

	if (mOKButton.isLongPressed(deltaTime))
	{
		if (Settings::getInstance()->getBool("GameOptionsAtNorth"))
			showSelectedGameSaveSnapshots();
		else
			showSelectedGameOptions();

		return;
	}

	if (mSelectButton.isLongPressed(deltaTime))
	{
		toggleFavoritesFilter();
		return;
	}

	if (mYButton.isLongPressed(deltaTime))
	{
		showQuickSearch();
		return;
	}
	
	if (mXButton.isLongPressed(deltaTime))
	{
		if (UIModeController::getInstance()->isUIModeKid() && cursorHasSaveStatesEnabled())
		{
			if (Settings::getInstance()->getBool("GameOptionsAtNorth"))
				showSelectedGameOptions();
			else
				showSelectedGameSaveSnapshots();
		}
		else if (!UIModeController::getInstance()->isUIModeKid() && (mRoot->getSystem()->isGameSystem() || mRoot->getSystem()->isGroupSystem()))
			CollectionSystemManager::get()->toggleGameInCollection(getCursor(), "Favorites");
	}
}

void ISimpleGameListView::goBack()
{
	if (mCursorStack.size())
	{
		auto top = mCursorStack.top();
		mCursorStack.pop();

		FolderData* folder = top->getParent();
		if (folder == nullptr && getCursor()->getSystem()->getParentGroupSystem() != nullptr)
			folder = getCursor()->getSystem()->getParentGroupSystem()->getRootFolder();

		if (folder != nullptr)
		{
			populateList(folder->getChildrenListToDisplay());
			setCursor(top);
			Sound::getFromTheme(getTheme(), getName(), "back")->play();
		}
	}
	else if (mPopupSelfReference)
	{
		ViewController::get()->setActiveView(mPopupParentView);
		mPopupParentView->updateHelpPrompts();
		closePopupContext();	
	}
	else
	{
		onFocusLost();
		SystemData* systemToView = getCursor()->getSystem();

		if (systemToView->isGroupChildSystem())
			systemToView = systemToView->getParentGroupSystem();
		else if (systemToView->isCollection())
			systemToView = CollectionSystemManager::get()->getSystemToView(systemToView);

		ViewController::get()->goToSystemView(systemToView);
	}	
}

bool ISimpleGameListView::input(InputConfig* config, Input input)
{
	if (mOKButton.isShortPressed(config, input))
	{
		launchSelectedGame();
		return true;
	}

	if (mYButton.isShortPressed(config, input))
	{
		moveToRandomGame();
		return true;
	}

	if (mXButton.isShortPressed(config, input))
	{
		if (Settings::getInstance()->getBool("GameOptionsAtNorth"))
			showSelectedGameOptions();
		else
			showSelectedGameSaveSnapshots();

		return true;
	}	

	if (!UIModeController::getInstance()->isUIModeKid() && mSelectButton.isShortPressed(config, input))
	{
		showGamelistOptions();
		return true;
	}

	if (config->isMappedTo(BUTTON_OK, input) || config->isMappedTo("x", input) || config->isMappedTo("y", input) || config->isMappedTo("select", input))
		return true;

	if (input.value == 0)
		return IGameListView::input(config, input);

	if (config->isMappedTo("l3", input))
	{
		showSelectedGameSaveSnapshots();
		return true;
	}
		
	if (config->isMappedTo(BUTTON_BACK, input))
	{
		goBack();
		return true;
	}
	else if ((Settings::getInstance()->getBool("QuickSystemSelect") && config->isMappedLike(getQuickSystemSelectRightButton(), input)) || config->isMappedLike("r2", input))
	{
		if (!mPopupSelfReference)
		{
			onFocusLost();
			ViewController::get()->goToNextGameList();
		}

		return true;
	}
	else if ((Settings::getInstance()->getBool("QuickSystemSelect") && config->isMappedLike(getQuickSystemSelectLeftButton(), input)) || config->isMappedLike("l2", input))
	{
		if (!mPopupSelfReference)
		{
			onFocusLost();
			ViewController::get()->goToPrevGameList();
		}

		return true;
	}

	return IGameListView::input(config, input);
}

void ISimpleGameListView::showGamelistOptions()
{
	FileData* cursor = getCursor();
	if (cursor == nullptr)
		return;

	auto idx = mRoot->getSystem()->getIndex(false);
	if (idx != nullptr && idx->hasRelevency())
		return;

	Sound::getFromTheme(mTheme, getName(), "menuOpen")->play();
	mWindow->pushGui(new GuiGamelistOptions(mWindow, this, this->mRoot->getSystem()));	
}


void ISimpleGameListView::showSelectedGameOptions()
{
	FileData* cursor = getCursor();
	if (cursor == nullptr)
		return;

	Sound::getFromTheme(mTheme, getName(), "menuOpen")->play();
	mWindow->pushGui(new GuiGameOptions(mWindow, cursor));
}

bool ISimpleGameListView::cursorHasSaveStatesEnabled()
{
	FileData* cursor = getCursor();
	if (cursor == nullptr || cursor->getType() != GAME)
		return false;

	return SaveStateRepository::isEnabled(cursor);
}

void ISimpleGameListView::showSelectedGameSaveSnapshots()
{
	FileData* cursor = getCursor();
	if (cursor == nullptr || cursor->getType() != GAME)
		return;

	if (SaveStateRepository::isEnabled(cursor))
	{
		Sound::getFromTheme(mTheme, getName(), "menuOpen")->play();

		mWindow->pushGui(new GuiSaveState(mWindow, cursor, [this, cursor](SaveState state)
		{
			Sound::getFromTheme(getTheme(), getName(), "launch")->play();

			LaunchGameOptions options;
			options.saveStateInfo = state;
			ViewController::get()->launch(cursor, options);
		}
		));
#ifdef _ENABLEEMUELEC
		guiSaveStateLoad(mWindow, cursor);
#endif
	}
}

void ISimpleGameListView::launchSelectedGame()
{
	// Don't launch game if transition is still running
	if (ViewController::get()->isAnimationPlaying(0))
		return;

	FileData* cursor = getCursor();
	FolderData* folder = NULL;

	if (mCursorStack.size() && cursor->getType() == PLACEHOLDER && cursor->getPath() == "..")
	{
		auto top = mCursorStack.top();
		mCursorStack.pop();

		FolderData* folder = top->getParent();
		if (folder == nullptr)
			folder = getCursor()->getSystem()->getParentGroupSystem()->getRootFolder();

		populateList(folder->getChildrenListToDisplay());
		setCursor(top);
		Sound::getFromTheme(getTheme(), getName(), "back")->play();
	}
	else
	{
		if (cursor->getType() == GAME)
		{
			if (SaveStateRepository::isEnabled(cursor) &&
				(cursor->getCurrentGameSetting("savestates") == "1" || (cursor->getCurrentGameSetting("savestates") == "2" && cursor->getSourceFileData()->getSystem()->getSaveStateRepository()->hasSaveStates(cursor))))
			{
				mWindow->pushGui(new GuiSaveState(mWindow, cursor, [this, cursor](SaveState state)
				{
					Sound::getFromTheme(getTheme(), getName(), "launch")->play();

					LaunchGameOptions options;
					options.saveStateInfo = state;
					ViewController::get()->launch(cursor, options);
				}
				));
#ifdef _ENABLEEMUELEC
				guiSaveStateLoad(mWindow, cursor);
#endif
			}
			else
			{
				Sound::getFromTheme(getTheme(), getName(), "launch")->play();
				launch(cursor);
			}
		}
		else if (cursor->getType() == FOLDER)
			moveToFolder((FolderData*)cursor);
	}
}


void ISimpleGameListView::showQuickSearch()
{
	std::string searchText;

	auto idx = mRoot->getSystem()->getIndex(false);
	if (idx != nullptr)
		searchText = idx->getTextFilter();

	auto updateVal = [this](const std::string& newVal)
	{
		auto index = mRoot->getSystem()->getIndex(!newVal.empty());
		if (index != nullptr)
		{
			index->setTextFilter(newVal);
			if (!index->isFiltered())
				mRoot->getSystem()->deleteIndex();
		}

		if (mRoot->getSystem()->isCollection())
			CollectionSystemManager::get()->reloadCollection(mRoot->getSystem()->getName());
		else
			ViewController::get()->reloadGameListView(mRoot->getSystem());
	};

	if (Settings::getInstance()->getBool("UseOSK"))
		mWindow->pushGui(new GuiTextEditPopupKeyboard(mWindow, _("FILTER GAMES BY TEXT"), searchText, updateVal, false));
	else
		mWindow->pushGui(new GuiTextEditPopup(mWindow, _("FILTER GAMES BY TEXT"), searchText, updateVal, false));
}

void ISimpleGameListView::moveToRandomGame()
{
	auto list = getFileDataEntries();

	unsigned int total = (int)list.size();
	if (total == 0)
		return;

	int target = Randomizer::random(total); // (int)Math::round((std::rand() / (float)RAND_MAX) * (total - 1));
	if (target >= 0 && target < total)
		setCursor(list.at(target));
}

std::vector<std::string> ISimpleGameListView::getEntriesLetters()
{	
	std::set<std::string> setOfLetters;

	for (auto file : getFileDataEntries()) 
		if (file->getType() == GAME)
			setOfLetters.insert(std::string(1, toupper(file->getName()[0])));

	std::vector<std::string> letters;

	for (const auto letter : setOfLetters)
		letters.push_back(letter);

	std::sort(letters.begin(), letters.end());
	return letters;
}

void ISimpleGameListView::updateHeaderLogoAndText()
{
	SystemData* system = mCursorStack.size() && (!mRoot->getSystem()->isGameSystem() || mRoot->getSystem()->isGroupSystem()) ? mCursorStack.top()->getSystem() : mRoot->getSystem();

	auto groupTheme = system->getTheme();
	if (groupTheme && mHeaderImage.hasImage())
	{
		const ThemeData::ThemeElement* logoElem = groupTheme->getElement(getName(), "logo", "image");
		if (logoElem && logoElem->has("path") && Utils::FileSystem::exists(logoElem->get<std::string>("path")))
			mHeaderImage.setImage(logoElem->get<std::string>("path"), false, mHeaderImage.getMaxSizeInfo());
	}

	mHeaderText.setText(system->getFullName());
}

void ISimpleGameListView::updateFolderPath()
{
	if (mCursorStack.size())
	{
		auto top = mCursorStack.top();
		mFolderPath.setText(top->getBreadCrumbPath());
	}
	else 
		mFolderPath.setText("");
}

void ISimpleGameListView::repopulate()
{
	FolderData* folder = mRoot;

	if (mCursorStack.size())
	{
		auto top = mCursorStack.top();
		if (top->getType() == FOLDER)
			folder = (FolderData*)top;
	}

	populateList(folder->getChildrenListToDisplay());
}

void ISimpleGameListView::setPopupContext(std::shared_ptr<IGameListView> pThis, std::shared_ptr<GuiComponent> parentView, const std::string label, const std::function<void()>& onExitTemporary)
{ 
	mPopupSelfReference = pThis;
	mPopupParentView = parentView;
	mOnExitPopup = onExitTemporary;

	if (mHeaderImage.hasImage())
	{
		mHeaderText.setText(_("Games similar to") + " " + label); // 

		mHeaderImage.setImage("");
		addChild(&mHeaderText);
		removeChild(&mHeaderImage);
	}
}

void ISimpleGameListView::closePopupContext()
{
	if (!mPopupSelfReference)
		return;

	auto exitPopup = mOnExitPopup;

	mPopupParentView.reset();	
	mPopupSelfReference.reset();

	if (exitPopup != nullptr)
		exitPopup();
}

std::vector<HelpPrompt> ISimpleGameListView::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;

	if (Renderer::getScreenProportion() > 1.4)
	{
		if (mPopupSelfReference == nullptr && Settings::getInstance()->getBool("QuickSystemSelect") && getQuickSystemSelectLeftButton() == "left")
			prompts.push_back(HelpPrompt("left/right", _("SYSTEM")));

		prompts.push_back(HelpPrompt("up/down", _("CHOOSE")));
	}

	bool invertNorthButton = Settings::getInstance()->getBool("GameOptionsAtNorth");

	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK"), [&] { goBack(); }));

	if (invertNorthButton)
		prompts.push_back(HelpPrompt(BUTTON_OK, _("SAVE STATES (HOLD)"), [&] { showSelectedGameSaveSnapshots(); }));
	else 
		prompts.push_back(HelpPrompt(BUTTON_OK, _("GAME OPTIONS (HOLD)"), [&] { showSelectedGameOptions(); }));

	if (!UIModeController::getInstance()->isUIModeKid())
		prompts.push_back(HelpPrompt("select", _("OPTIONS"), [&] { showGamelistOptions(); }));

	if (cursorHasSaveStatesEnabled())
	{
		if (invertNorthButton)
		{
			if (UIModeController::getInstance()->isUIModeKid())
				prompts.push_back(HelpPrompt("x", _("GAME OPTIONS"), [&] { showSelectedGameOptions(); }));
			else
				prompts.push_back(HelpPrompt("x", _("GAME OPTIONS") + std::string("/") + _("FAVORITE"), [&] { showSelectedGameOptions(); }));
		}
		else
		{
			if (UIModeController::getInstance()->isUIModeKid())
				prompts.push_back(HelpPrompt("x", _("SAVE STATES"), [&] { showSelectedGameSaveSnapshots(); }));
			else
				prompts.push_back(HelpPrompt("x", _("SAVE STATES") + std::string("/") + _("FAVORITE"), [&] { showSelectedGameSaveSnapshots(); }));
		}
	}
	else if (!UIModeController::getInstance()->isUIModeKid())
		prompts.push_back(HelpPrompt("x", _("FAVORITE")));

	prompts.push_back(HelpPrompt("y", _("SEARCH") + std::string("/") + _("RANDOM"), [&] { showQuickSearch(); }));

	return prompts;
}

void ISimpleGameListView::updateThemeExtrasBindings()
{
	FileData* file = getCursor();
	if (file == nullptr)
		return;

	auto system = file->getSystem();

	for (auto extra : mThemeExtras)
	{
		TextComponent* text = dynamic_cast<TextComponent*>(extra);
		if (text != nullptr)
			Binding::updateBindings(text, system);

		Binding::updateBindings(extra, file);
	}
}

bool ISimpleGameListView::onAction(const std::string& action)
{
	if (action == "back")
	{
		goBack();
		return true;
	}

	if (action == "options")
	{
		showGamelistOptions();
		return true;
	}

	if (action == "gameoptions")
	{
		showSelectedGameOptions();
		return true;
	}

	if (action == "launch")
	{
		launchSelectedGame();
		return true;
	}

	if (action == "search")
	{
		showQuickSearch();
		return true;
	}

	if (action == "savestates")
	{
		showSelectedGameSaveSnapshots();
		return true;
	}

	if (action == "favorite")
	{
		toggleFavoritesFilter();
		return true;
	}
	
	if (action == "random")
	{
		moveToRandomGame();
		return true;
	}
	
	if (action == "video")
	{
		FileData* game = getCursor();
		if (game != nullptr)
		{
			auto path = game->getMetadata(MetaDataId::Video);
			if (!path.empty())
				GuiVideoViewer::playVideo(mWindow, path);
		}
		return true;
	}

	if (action == "manual")
	{
		FileData* game = getCursor();
		if (game != nullptr)
		{
			auto path = game->getMetadata(MetaDataId::Manual);
			if (!path.empty())
				GuiImageViewer::showPdf(mWindow, path);
		}
		return true;
	}

	if (action == "map")
	{
		FileData* game = getCursor();
		if (game != nullptr)
		{
			auto path = game->getMetadata(MetaDataId::Map);
			if (!path.empty())
				GuiImageViewer::showImage(mWindow, path, Utils::String::toLower(Utils::FileSystem::getExtension(path)) != ".pdf");
		}
		return true;
	}

	if (action == "medias")
	{
		FileData* game = getCursor();
		if (game != nullptr)
		{
			auto imageList = game->getSourceFileData()->getFileMedias();
			if (imageList.size())
				GuiImageViewer::showImages(mWindow, imageList);
		}

		return true;
	}

	if (action == "cheevos")
	{
		FileData* game = getCursor();
		if (game != nullptr)
		{
			auto path = Utils::String::toInteger(game->getMetadata(MetaDataId::CheevosId));
			if (path != 0)
				GuiGameAchievements::show(mWindow, path);
		}

		return true;
	}

	
	return false;
}
