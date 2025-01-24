#include "gamescreen.h"

#include <array>
#include <prism/numberpopuphandler.h>

#include "bookscreen.h"

static struct 
{
    CollisionListData* mMermaidCollisionList;
    CollisionListData* mBubbleMermaidCollisionList;
    CollisionListData* mBubbleFishCollisionList;
    CollisionListData* mFishCollisionList;
    int mLevel = 0;
} gGameScreenData;

class GameScreen
{
    public:
    GameScreen() {
        instantiateActor(getPrismNumberPopupHandler());
        load();
        streamMusicFile("game/GAME.ogg");
        //activateCollisionHandlerDebugMode();
    }

    MugenSpriteFile mSprites;
    MugenAnimations mAnimations;
    MugenSounds mSounds;

    void loadFiles() {
        mSprites = loadMugenSpriteFileWithoutPalette("game/GAME.sff");
        mAnimations = loadMugenAnimationFile("game/GAME.air");
        mSounds = loadMugenSoundFile("game/GAME.snd");
    }

    void load() {
        loadFiles();
        loadGame();
    }
    
    void loadCollisionLists() {
        gGameScreenData.mMermaidCollisionList = addCollisionListToHandler();
        gGameScreenData.mBubbleMermaidCollisionList = addCollisionListToHandler();
        gGameScreenData.mBubbleFishCollisionList = addCollisionListToHandler();
        gGameScreenData.mFishCollisionList = addCollisionListToHandler();
        addCollisionHandlerCheck(gGameScreenData.mMermaidCollisionList, gGameScreenData.mBubbleMermaidCollisionList);
        addCollisionHandlerCheck(gGameScreenData.mFishCollisionList, gGameScreenData.mBubbleFishCollisionList);
    }

    void loadGame() {
        loadCollisionLists();
        loadUI();
        loadBG();
        loadTriton();
    }


    int mLifeCount = 5;
    int mMermaidSpawnsLeft = 25;
    std::array<int, 5> mHearts;
    int mScoreTextPreEntityID;
    int mScoreTextID;
    int mScore = 0;
    void loadUI() {
        for(int i = 0; i < mLifeCount; i++) {
            mHearts[i] = addBlitzEntity(Vector3D(300 - i * 20, 10, 80));
            addBlitzMugenAnimationComponent(mHearts[i], &mSprites, &mAnimations, 61);
        }

        mScoreTextPreEntityID = addBlitzEntity(Vector3D(45, 10, 80));
        addBlitzMugenAnimationComponent(mScoreTextPreEntityID, &mSprites, &mAnimations, 50);
        mScoreTextID = addMugenTextMugenStyle("0", Vector3D(45, 12, 80), Vector3DI(1, 5, 1));
    }

    void updateScoreText() {
        changeMugenText(mScoreTextID, std::to_string(mScore).c_str());
    }

    int bgEntityID;
    void loadBG() {
        bgEntityID = addBlitzEntity(Vector3D(0, 0, 1));
        addBlitzMugenAnimationComponent(bgEntityID, &mSprites, &mAnimations, 1);
    }

    int tritonEntityID;
    void loadTriton() {
        tritonEntityID = addBlitzEntity(Vector3D(160, 230, 10));
        addBlitzMugenAnimationComponent(tritonEntityID, &mSprites, &mAnimations, 10);
    }

    void update() {
        if (!isGameOver)
        {
            updateTriton();
            updateBubbles();
            updateMermaids();
            updateFishes();
            updateWinning();
        }
        updateLosing();
    }

    int isGameOver = 0;
    int gameOverEntityID;
    void updateLosing() {
        if(!isGameOver) {
            if(mLifeCount <= 0) {
                isGameOver = 1;
                gameOverEntityID = addBlitzEntity(Vector3D(0, 0, 70));
                addBlitzMugenAnimationComponent(gameOverEntityID, &mSprites, &mAnimations, 70);
                tryPlayMugenSoundAdvanced(&mSounds, 2, 3, 0.1);
            }
        }
        else
        {
            if(hasPressedStartFlank()) {
                resetGame();
                setNewScreen(getGameScreen());
            }
        }
    }

    void updateWinning() {
        if(!mMermaidSpawnsLeft && mMermaids.empty()) {
            setBookName("OUTRO");
            setNewScreen(getBookScreen());
        }
    }

    enum class FishState {
        MOVING_LEFT,
        MOVING_RIGHT,
        IN_BUBBLE,
    };

    struct Fish {
        int entityID;
        FishState state;
        bool toBeRemoved;
        int time = 0;
        int collisionID;
    };

    std::map<int, Fish> mFishes;

    void updateFishes() {
        updateAddingFishes();
        updateActiveFishes();
    }

    void updateAddingFishes() {
        if(randfromInteger(0, 100) < 1) {
            addFish();
        }
    }

    void addFish() {
        int isMovingLeft = randfromInteger(0, 1);
        
        auto entityID = addBlitzEntity(Vector3D(isMovingLeft ? 340 : -20, randfromInteger(58, 220), 20));
        addBlitzMugenAnimationComponent(entityID, &mSprites, &mAnimations, 30);
        setBlitzMugenAnimationFaceDirection(entityID, isMovingLeft);
        addBlitzCollisionComponent(entityID);
        auto collisionID = addBlitzCollisionCirc(entityID, gGameScreenData.mFishCollisionList, CollisionCirc{Vector2D(0, 0), 10});
        mFishes[entityID] = {entityID, isMovingLeft ? FishState::MOVING_LEFT : FishState::MOVING_RIGHT, false, 0, collisionID };
    }

    void updateActiveFishes() {
        for(auto& fish : mFishes) {
            updateFish(fish.second);
        }

        removeFishes();
    }

    void updateFish(Fish& e) {
        switch(e.state) {
            case FishState::MOVING_LEFT:
            case FishState::MOVING_RIGHT:
                updateFishMoving(e);
                updateFishBlockingShot(e);
                break;
            case FishState::IN_BUBBLE:
                updateFishInBubble(e);
                break;
        }
    }

    void updateFishInBubble(Fish& e) {
        e.time++;
        if(e.time >= 120) {
            e.toBeRemoved = true;
        }
    }

    void updateFishBlockingShot(Fish& e) {
        if(hasBlitzCollidedThisFrame(e.entityID, e.collisionID)) {
            tryPlayMugenSoundAdvanced(&mSounds, 2, 2, 0.1);
            e.state = FishState::IN_BUBBLE;
            e.time = 0;        
        }
    }

    void updateFishMoving(Fish& e) {
        auto pos = getBlitzEntityPosition(e.entityID);
        if(e.state == FishState::MOVING_LEFT) {
            pos.x -= 2;
        } else {
            pos.x += 2;
        }
        setBlitzEntityPosition(e.entityID, pos);
        if(pos.x <= -20 || pos.x >= 340) {
            e.toBeRemoved = true;
        }
    }

    void removeFishes() {
        auto it = mFishes.begin();
        while(it != mFishes.end()) {
            if(it->second.toBeRemoved) {
                removeBlitzEntity(it->second.entityID);
                it = mFishes.erase(it);
            } else {
                it++;
            }
        }
    }

    void updateTriton() {
        updateTritonMovement();
        updateTritonShootingBubbles();
    }

    void updateTritonMovement() {
        auto pos = getBlitzEntityPosition(tritonEntityID);
        if(hasPressedLeft()) {
            pos.x -= 2;
        } else if(hasPressedRight()) {
            pos.x += 2;
        }
        pos = clampPositionToGeoRectangle(pos, GeoRectangle2D(0, 0, 320, 240));
        setBlitzEntityPosition(tritonEntityID, pos);
    }

    void updateTritonShootingBubbles() {
        if(hasPressedAFlank()) {
            tryPlayMugenSoundAdvanced(&mSounds, 2, 1, 0.1);
            shootBubble();
        }
    }

    void shootBubble() {
        auto pos = getBlitzEntityPosition(tritonEntityID);
        pos.y -= 20;
        auto entityID = addBlitzEntity(pos);
        addBlitzMugenAnimationComponent(entityID, &mSprites, &mAnimations, 40);
        addBlitzCollisionComponent(entityID);
        auto collisionIDMermaid = addBlitzCollisionCirc(entityID, gGameScreenData.mBubbleMermaidCollisionList, CollisionCirc{Vector2D(0, 0), 10});
        auto collisionIDFish = addBlitzCollisionCirc(entityID, gGameScreenData.mBubbleFishCollisionList, CollisionCirc{Vector2D(0, 0), 10});
        mBubbles[entityID] = {entityID, BubbleState::GOING_UP, false, collisionIDMermaid, collisionIDFish};
    }

    enum class BubbleState {
        GOING_UP,
    };

    struct Bubble {
        int entityID;
        BubbleState state;
        bool toBeRemoved;
        int collisionIDMermaid;
        int collisionIDFish;
    };

    std::map<int, Bubble> mBubbles;
    void updateBubbles() {
        updateActiveBubbles();
    }

    void updateActiveBubbles() {
        for(auto& bubble : mBubbles) {
            updateBubble(bubble.second);
        }

        removeBubbles();
    }

    void updateBubble(Bubble& e) {
        switch(e.state) {
            case BubbleState::GOING_UP:
                updateBubbleGoingUp(e);
                break;
        }
    }

    void updateBubbleGoingUp(Bubble& e) {
        auto pos = getBlitzEntityPosition(e.entityID);
        pos.y -= 2;
        setBlitzEntityPosition(e.entityID, pos);
        if(pos.y <= -20) {
            tryPlayMugenSoundAdvanced(&mSounds, 2, 0, 0.1);
            e.toBeRemoved = true;
        }

        if(hasBlitzCollidedThisFrame(e.entityID, e.collisionIDMermaid) || hasBlitzCollidedThisFrame(e.entityID, e.collisionIDFish)) {
            e.toBeRemoved = true;
        }
    }

    void removeBubbles() {
        auto it = mBubbles.begin();
        while(it != mBubbles.end()) {
            if(it->second.toBeRemoved) {
                removeBlitzEntity(it->second.entityID);
                it = mBubbles.erase(it);
            } else {
                it++;
            }
        }
    }

    enum class MermaidState {
        DISAPPEARING,
        IN_BUBBLE,
        MOVING_AWAY,
        DYING,
    };

    struct Mermaid {
        int entityID;
        MermaidState state;
        bool toBeRemoved;
        int mTime = 0;
        int collisionID;
    };
    std::map<int, Mermaid> mMermaids;

    void updateMermaids() {
        updateAddingMermaids();
        updateActiveMermaids();
    }

    void updateAddingMermaids() {
        if(randfromInteger(0, 100) < 1 && mMermaids.size() < 5 && mMermaidSpawnsLeft) {
            addMermaid();
            mMermaidSpawnsLeft--;
        }
    }

    void addMermaid() {
        auto entityID = addBlitzEntity(Vector3D(randfromInteger(10, 310), 48, 20));
        addBlitzMugenAnimationComponent(entityID, &mSprites, &mAnimations, 20);
        addBlitzCollisionComponent(entityID);
        auto collisionID = addBlitzCollisionCirc(entityID, gGameScreenData.mMermaidCollisionList, CollisionCirc{Vector2D(0, 0), 10});
        mMermaids[entityID] = {entityID, MermaidState::DISAPPEARING, false, 0, collisionID };
        tryPlayMugenSoundAdvanced(&mSounds, 2, 4, 0.1);
    }

    void updateActiveMermaids() {
        for(auto& mermaid : mMermaids) {
            updateMermaid(mermaid.second);
        }

        removeMermaids();
    }

    void updateMermaid(Mermaid& e) {
        switch(e.state) {
            case MermaidState::DISAPPEARING:
                updateMermaidDisappearing(e);
                break;
            case MermaidState::IN_BUBBLE:
                updateMermaidInBubble(e);
                break;
            case MermaidState::MOVING_AWAY:
                updateMermaidMovingAway(e);
                break;
            case MermaidState::DYING:
                updateMermaidDying(e);
                break;
        }
    }

    void updateMermaidDisappearing(Mermaid& e) {
        e.mTime++;
        if(e.mTime >= 240) {
            addPrismNumberPopup(-1, getBlitzEntityPosition(e.entityID), 0, Vector3D(0, -1, 0), 1, 1, 60);
            removeLife();
            changeBlitzMugenAnimation(e.entityID, 23);
            e.state = MermaidState::DYING;
            e.mTime = 0;
        }
        else
        {
            updateMermaidBeingSaved(e);
        }
    }

    void removeLife() {
        if (!mLifeCount) return;
        mLifeCount--;
        changeBlitzMugenAnimation(mHearts[mLifeCount], 60);
    }
    
    void updateMermaidBeingSaved(Mermaid& e) {
        if(hasBlitzCollidedThisFrame(e.entityID, e.collisionID)) {
            addPrismNumberPopup(100, getBlitzEntityPosition(e.entityID), 0, Vector3D(0, -1, 0), 1, 5, 60);
            mScore += 100;
            tryPlayMugenSoundAdvanced(&mSounds, 2, 5, 0.1);
            updateScoreText();
            changeBlitzMugenAnimation(e.entityID, 21);
            e.state = MermaidState::IN_BUBBLE;
            e.mTime = 0;
        }
    }



    void updateMermaidInBubble(Mermaid& e) {
        e.mTime++;
        if(e.mTime >= 120) {
            changeBlitzMugenAnimation(e.entityID, 22);
            e.state = MermaidState::MOVING_AWAY;
            e.mTime = 0;
        }
    }

    void updateMermaidMovingAway(Mermaid& e) {
        auto pos = getBlitzEntityPosition(e.entityID);
        pos.x -= 2;
        setBlitzEntityPosition(e.entityID, pos);
        if(pos.x <= -20) {
            e.toBeRemoved = true;
        }
    }

    void updateMermaidDying(Mermaid& e) {
        auto pos = getBlitzEntityPosition(e.entityID);
        pos.x += 2;
        setBlitzEntityPosition(e.entityID, pos);
        if(pos.x >= 340) {
            e.toBeRemoved = true;
        }
    }

    void removeMermaids() {
        auto it = mMermaids.begin();
        while(it != mMermaids.end()) {
            if(it->second.toBeRemoved) {
                removeBlitzEntity(it->second.entityID);
                it = mMermaids.erase(it);
            } else {
                it++;
            }
        }
    }
};

EXPORT_SCREEN_CLASS(GameScreen);

void resetGame()
{
    gGameScreenData.mLevel = 0;
}