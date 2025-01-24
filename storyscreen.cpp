#include "storyscreen.h"

#include <assert.h>

#include <prism/blitz.h>
#include <prism/stlutil.h>

#include <prism/soundeffect.h>

using namespace std;
using namespace prism;

struct {
	MugenDefScript mScript;
	MugenDefScriptGroup* mCurrentGroup;
	MugenSpriteFile mSprites;
	MugenSounds mSounds;

	MugenSounds mVoiceSounds;
	int mNextVoice = 0;

	MugenAnimation* mOldAnimation;
	MugenAnimation* mAnimation;
	MugenAnimationHandlerElement* mAnimationID;
	MugenAnimationHandlerElement* mOldAnimationID;

	Position mOldAnimationBasePosition;
	Position mAnimationBasePosition;

	int mSpeakerID;
	int mTextID;

	int mIsStoryOver;

	char mDefinitionPath[1024];
	int mTrack;
} gStoryScreenData;

int isImageGroup() {
	string name = gStoryScreenData.mCurrentGroup->mName;
	char firstW[100];
	sscanf(name.data(), "%s", firstW);

	return !strcmp("image", firstW);
}

void increaseGroup() {
	gStoryScreenData.mCurrentGroup = gStoryScreenData.mCurrentGroup->mNext;
}

void loadImageGroup() {
	if (gStoryScreenData.mOldAnimationID != nullptr) {
		removeMugenAnimation(gStoryScreenData.mOldAnimationID);
		destroyMugenAnimation(gStoryScreenData.mOldAnimation);
	}

	if (gStoryScreenData.mAnimationID != nullptr) {
		setMugenAnimationBasePosition(gStoryScreenData.mAnimationID, &gStoryScreenData.mOldAnimationBasePosition);
	}

	gStoryScreenData.mOldAnimationID = gStoryScreenData.mAnimationID;
	gStoryScreenData.mOldAnimation = gStoryScreenData.mAnimation;


	int group = getMugenDefNumberVariableAsGroup(gStoryScreenData.mCurrentGroup, "group");
	int item = getMugenDefNumberVariableAsGroup(gStoryScreenData.mCurrentGroup, "item");
	gStoryScreenData.mAnimation = createOneFrameMugenAnimationForSprite(group, item);
	gStoryScreenData.mAnimationID = addMugenAnimation(gStoryScreenData.mAnimation, &gStoryScreenData.mSprites, Vector3D(0, 0, 0));
	setMugenAnimationBasePosition(gStoryScreenData.mAnimationID, &gStoryScreenData.mAnimationBasePosition);

	increaseGroup();
}


int isTextGroup() {
	string name = gStoryScreenData.mCurrentGroup->mName;
	char firstW[100];
	sscanf(name.data(), "%s", firstW);

	return !strcmp("text", firstW);
}

void loadTextGroup() {
	if (gStoryScreenData.mTextID != -1) {
		removeMugenText(gStoryScreenData.mTextID);
		removeMugenText(gStoryScreenData.mSpeakerID);
	}

	char* speaker = getAllocatedMugenDefStringVariableAsGroup(gStoryScreenData.mCurrentGroup, "speaker");
	char* text = getAllocatedMugenDefStringVariableAsGroup(gStoryScreenData.mCurrentGroup, "text");

	gStoryScreenData.mSpeakerID = addMugenText(speaker, Vector3D(30 / 2, 348 / 2, 3), 1);

	gStoryScreenData.mTextID = addMugenText(text, Vector3D(30 / 2, 380 / 2, 3), 1);
	setMugenTextBuildup(gStoryScreenData.mTextID, 1);
	setMugenTextTextBoxWidth(gStoryScreenData.mTextID, 560 / 2);
	setMugenTextColor(gStoryScreenData.mTextID, COLOR_WHITE);

	stopAllSoundEffects();
	tryPlayMugenSound(&gStoryScreenData.mVoiceSounds, 1, gStoryScreenData.mNextVoice);
	gStoryScreenData.mNextVoice++;

	freeMemory(speaker);
	freeMemory(text);

	increaseGroup();
}

int isTitleGroup() {
	string name = gStoryScreenData.mCurrentGroup->mName;
	char firstW[100];
	sscanf(name.data(), "%s", firstW);

	return !strcmp("title", firstW);
}

void goToTitle(void* tCaller) {
	(void)tCaller;
	//setNewScreen(getTitleScreen());
}

void loadTitleGroup() {
	gStoryScreenData.mIsStoryOver = 1;

	addFadeOut(30, goToTitle, NULL);
}

void loadNextStoryGroup() {
	int isRunning = 1;
	while (isRunning) {
		if (isImageGroup()) {
			loadImageGroup();
		}
		else if (isTextGroup()) {
			loadTextGroup();
			break;
		}
		else if (isTitleGroup()) {
			loadTitleGroup();
			break;
		}
		else {
			logError("Unidentified group type.");
			//logErrorString(gStoryScreenData.mCurrentGroup->mName);
			abortSystem();
		}
	}
}

void findStartOfStoryBoard() {
	gStoryScreenData.mCurrentGroup = gStoryScreenData.mScript.mFirstGroup;

	while (gStoryScreenData.mCurrentGroup && "storystart" != gStoryScreenData.mCurrentGroup->mName) {
		gStoryScreenData.mCurrentGroup = gStoryScreenData.mCurrentGroup->mNext;
	}

	assert(gStoryScreenData.mCurrentGroup);
	gStoryScreenData.mCurrentGroup = gStoryScreenData.mCurrentGroup->mNext;
	assert(gStoryScreenData.mCurrentGroup);

	gStoryScreenData.mAnimationID = nullptr;
	gStoryScreenData.mOldAnimationID = nullptr;
	gStoryScreenData.mTextID = -1;

	gStoryScreenData.mOldAnimationBasePosition = Vector3D(0, 0, 1);
	gStoryScreenData.mAnimationBasePosition = Vector3D(0, 0, 2);

	loadNextStoryGroup();
}



void loadStoryScreen() {
	gStoryScreenData.mNextVoice = 0;
	gStoryScreenData.mIsStoryOver = 0;

	gStoryScreenData.mSounds = loadMugenSoundFile("game/GAME.snd");
	gStoryScreenData.mVoiceSounds = loadMugenSoundFile("game/INTRO.snd");

	loadMugenDefScript(&gStoryScreenData.mScript, gStoryScreenData.mDefinitionPath);

	if (isMugenDefStringVariable(&gStoryScreenData.mScript, "header", "sprites")) {
		char* spritePath = getAllocatedMugenDefStringVariable(&gStoryScreenData.mScript, "header", "sprites");
		gStoryScreenData.mSprites = loadMugenSpriteFileWithoutPalette(spritePath);
		freeMemory(spritePath);
	}
	else {
		char path[1024];
		char folder[1024];
		strcpy(folder, gStoryScreenData.mDefinitionPath);
		char* dot = strrchr(folder, '.');
		*dot = '\0';
		sprintf(path, "%s.sff", folder);
		gStoryScreenData.mSprites = loadMugenSpriteFileWithoutPalette(path);

	}

	findStartOfStoryBoard();

	streamMusicFile("game/STORY.ogg");
	//playTrack(gStoryScreenData.mTrack);
}


void updateText() {
	if (gStoryScreenData.mIsStoryOver) return;
	if (gStoryScreenData.mTextID == -1) return;

	if (hasPressedAFlankSingle(0) || hasPressedAFlankSingle(1) || hasPressedKeyboardKeyFlank(KEYBOARD_SPACE_PRISM) || hasPressedStartFlank()) {
		tryPlayMugenSound(&gStoryScreenData.mSounds, 1, 2);
		if (isMugenTextBuiltUp(gStoryScreenData.mTextID)) {
			loadNextStoryGroup();
		}
		else {
			setMugenTextBuiltUp(gStoryScreenData.mTextID);
		}
	}
}

void updateStoryScreen() {

	updateText();
}


Screen gStoryScreen;

Screen* getStoryScreen() {
	gStoryScreen = makeScreen(loadStoryScreen, updateStoryScreen);
	return &gStoryScreen;
}

void setCurrentStoryDefinitionFile(char* tPath, int tTrack) {
	strcpy(gStoryScreenData.mDefinitionPath, tPath);
	gStoryScreenData.mTrack = tTrack;
}
