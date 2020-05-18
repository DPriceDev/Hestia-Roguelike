#ifndef HESTIA_ROGUE_GAME_ENVIRONMENT_H
#define HESTIA_ROGUE_GAME_ENVIRONMENT_H

#include <framework/game_envrionment.h>

#include "player/RoguePlayer.h"
#include "world/Wall.h"
#include "world/map_grid.h"

class RogueGameEnvironment : public HGE::GameEnvironment {

    public:
    RogueGameEnvironment() = default;
    ~RogueGameEnvironment() override = default;

    void beginGame() override {
        createObject<MapGrid>();
    }

    void gameLoop(const double & deltaTime) override { }

    void endGame() override {

    }
};

#endif