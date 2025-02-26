#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/CCDirector.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

bool enableMod;
bool enableLogging;
bool gameStarted = false;

void performJumpscare(std::string reason, std::string modSetting, bool isTest = false) { // it didnt work without taking another parameter :(
	const std::string& imagePath = Mod::get()->getSettingValue<std::filesystem::path>("jumpscareImage").string();
	if (enableLogging) log::debug("The path for the image is: {}", imagePath);
	CCSprite* jumpscareImage = CCSprite::create(imagePath.c_str());

	if (enableLogging) log::debug("\"reason\" is: {} \"modSetting\" is: {}", reason, modSetting);

	if (!jumpscareImage) {
		if (enableLogging) log::debug("Jumpscare on {} failed - invalid image.", reason);
		if (isTest) FLAlertLayer::create("Fail", "Error getting image.", "OK")->show();
		return;
	}
	if (!enableMod) {
		if (enableLogging) log::debug("Jumpscare on {} was not performed - mod is disabled.", reason);
		return;
    }
	jumpscareImage->setID("jumpscare-image"_spr);

	CCScene* currentScene = CCScene::get();
	currentScene->addChild(jumpscareImage);

	jumpscareImage->setOpacity(0);
	jumpscareImage->setZOrder(currentScene->getHighestChildZ() + 1000);

	CCSize winSize = CCDirector::sharedDirector()->getWinSize();
	jumpscareImage->setPosition(winSize / 2.f);

	CCArray* actionsArray = CCArray::create();
	actionsArray->addObject(CCFadeTo::create(Mod::get()->getSettingValue<double>("FadeIn"), Mod::get()->getSettingValue<double>("MaxOpacity") * 255));
	actionsArray->addObject(CCFadeTo::create(Mod::get()->getSettingValue<double>("FadeOut"), 0));
	actionsArray->addObject(CallFuncExt::create([currentScene, jumpscareImage]{
		currentScene->removeChild(jumpscareImage);
	}));

	CCSize jumpscareSize = jumpscareImage->getContentSize();
	jumpscareImage->setScale(winSize.height / jumpscareSize.height);

	double randomNumber = rand() / (RAND_MAX + 1.0);

	if (isTest || (randomNumber < Mod::get()->getSettingValue<double>(fmt::format("ChanceOn{}", modSetting)) / 100 && Mod::get()->getSettingValue<bool>(fmt::format("EnableJumpscareOn{}", modSetting)))) {
		jumpscareImage->runAction(CCSequence::create(actionsArray));
		if (enableLogging) log::debug("Jumpscare on {} was performed.", reason);
		return;
	}
	if (enableLogging) log::debug("Jumpscare on {} was not performed, unlucky.", reason);
}

class $modify(MyMenuLayer, MenuLayer) {
	bool init() {
		if (!MenuLayer::init()) return false;
		if (gameStarted) return true;
		gameStarted = true;
		return true;
	}
};

class $modify(MyPlayLayer, PlayLayer) {
	void destroyPlayer(PlayerObject* player, GameObject* object) {
		PlayLayer::destroyPlayer(player, object);	
		if (!player || !object || object == m_anticheatSpike) return;
		performJumpscare("death", "Death");
	}
};

class $modify(MyPauseLayer, PauseLayer) {
	void onQuit(CCObject* sender) {
		PauseLayer::onQuit(sender);
		performJumpscare("level exit", "LevelExit");
	}
};

class $modify(MyCCDirector, CCDirector) {
	void willSwitchToScene(CCScene* pScene) {
		CCDirector::willSwitchToScene(pScene);
		performJumpscare("scene transition", "SceneTransition");
	}
};

$on_mod(Loaded) {
	(void) Mod::get()->registerCustomSettingType("test-jumpscare", &TestJumpscareSettingV3::parse);
	listenForSettingChanges("EnableMod", [](bool value) {
		enableLogging = value;
	});
	listenForSettingChanges("EnableLogging", [](bool value) {
		enableLogging = enableMod ? value : false;
	});
}

#include "TestJumpscareSetting.hpp" //TODO: why must this be at the end of the cpp file?