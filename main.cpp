/*
This file deals with
*/

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

#include <SDL2/SDL.h>

#include "action.hpp"
#include "utils.hpp"
#include "world.hpp"

static bool activateKey(
    SDL_Scancode scancode,
    const Uint8 *keyboardState,
    const Uint8 *lastKeyboardState,
    bool holdKey
) {
    return
        keyboardState[scancode] &&
        (
            holdKey ||
            !lastKeyboardState[scancode]
        );
}

static bool activateKeyActionModified(
    SDL_Scancode scancode,
    const Uint8 *keyboardState,
    const Uint8 *lastKeyboardState,
    bool holdKey,
    bool& currentActionModified
) {
    bool ret = activateKey(scancode, keyboardState, lastKeyboardState, holdKey);
    currentActionModified = currentActionModified || ret;
    return ret;
}

// Controls that may not act immediately.
static bool holdControls(
    const Uint8* keyboardState,
    const Uint8* lastKeyboardState,
    bool holdKey,
    Action& action
) {
    bool currentActionModified = false;
    if (activateKeyActionModified(SDL_SCANCODE_LEFT, keyboardState, lastKeyboardState, holdKey, currentActionModified)) {
        action.setMoveX(Action::MoveX::LEFT);
    }
    if (activateKeyActionModified(SDL_SCANCODE_RIGHT, keyboardState, lastKeyboardState, holdKey, currentActionModified)) {
        action.setMoveX(Action::MoveX::RIGHT);
    }
    if (activateKeyActionModified(SDL_SCANCODE_UP, keyboardState, lastKeyboardState, holdKey, currentActionModified)) {
        action.setMoveY(Action::MoveY::UP);
    }
    if (activateKeyActionModified(SDL_SCANCODE_DOWN, keyboardState, lastKeyboardState, holdKey, currentActionModified)) {
        action.setMoveY(Action::MoveY::DOWN);
    }
    if (activateKeyActionModified(SDL_SCANCODE_SPACE, keyboardState, lastKeyboardState, holdKey, currentActionModified)) {
    }
    return currentActionModified;
}

static void printHelp() {
    std::cerr <<
        "# CLI Options\n"
        "\n"
        "## Interactive play options\n"
        "\n"
        "- `-b`:          (Block) don't block on player input.\n"
        "\n"
        "                 If given, if he player does not give any input until\n"
        "                 before the current frame is over, and empty input is used,\n"
        "                 and the simulation progresses.\n"
        "\n"
        "                 Not setting this option makes the game more Rogue-like.\n"
        "\n"
        "- `-d`:          (Display) turn display off. Might make simulation faster\n"
        "                 or runnable in device without display.\n"
        "\n"
        "                 User input is only available with display.\n"
        "\n"
        "                 If not present, simulation runs as faster as possible.\n"
        "\n"
        "                 Helps humans visualize non-interactive simulations\n"
        "                 that are too fast. E.g. `-f 2.0` limits simulation to 2 FPS.\n"
        "\n"
        "                 Forces -p = 0, since it is currently impossible to control\n"
        "                 players without a display.\n"
        "\n"
        "- `-f <double>`: (Fps) limit FPS to <double> FPS. Default: 1.0.\n"
        "\n"
        "                 You likely don't want this for interactive simulations that\n"
        "                 block on user input (Rogue-like), as this becomes lag.\n"
        "\n"
        "- `-H`:          (Hold key) actions are taken if the player is holding\n"
        "                 at the end of a frame, a click is not needed.\n"
        "\n"
        "                 The simulation advances automatically if the player is hodling any key\n"
        "                 at the end of the frame.\n"
        "\n"
        "                 If a click action (-i) was taken during the previous frame,\n"
        "                 it gets overridden if any key is held at the en of the frame.\n"
        "\n"
        "                 Holding SPACE makes the simulation advance with an empty action.\n"
        "\n"
        "                 Makes the game more interactive, and controls less precise.\n"
        "\n"
        "                 By hardware limitation, some combinations of keys may be impossible,\n"
        "                 while similar ones are possible, e.g. Right + Up + Space vs Left + Up + Space\n"
        "                 on Lenovo keyboards:\n"
        "                 http://unix.stackexchange.com/questions/268850/leftupspace-keys-not-working-on-thinkpad-x201\n"
        "\n"
        "                 Only the first human player is affected by this option, since it is impossible\n"
        "                 to give different controls to different players (the only two design\n"
        "                 choices are: affect only first or affect all equally, and we chose N.1).\n"
        "\n"
        "- `-i`:          (Immediate) mode. Create action immediately whenever the user presses any key,\n"
        "                 without waiting for `SPACE` to be pressed.\n"
        "\n"
        "                 Makes game more interactive, and less precisely controllable.\n"
        "\n"
        "                 In particular, it becomes difficult to press multiple simultaneous\n"
        "                 keys consistently.\n"
        "\n"
        "- `-s <string>`: (Scenario) choose a named pre-built world scenario. TODO way to show\n"
        "                 scenario list here. For now read source\n"
        "\n"
        "- `-v <int>`:    (View player) Only show what the int-th player is able to observe.\n"
        "\n"
        "                 This notably limits the field of view of the player,\n"
        "                 but also includes for which player the HUD is about (e.g. score).\n"
        "\n"
        "                 You are forced to use this if the world is so large that\n"
        "                 each tile would be less than one pixel wide.\n"
        "\n"
        "                 You do not need to be controlling the observed player: in particular\n"
        "                 if there are 0 players to be controlled by keyboard, you can just watch\n"
        "                 the action unroll by itself.\n"
        "                 You can also control one player while observing another,\n"
        "                 but you will likely go nuts.\n"
        "\n"
        "- `-W <int>`:    (Width) window width in pixels. Square windows only for now.\n"
        "                 Must be divisible by the width of the world. Default: 500.\n"
        "\n"
        "## World state options\n"
        "\n"
        "- `-p`:          now many human players should be added to the map. Default: 1.\n"
        "\n"
        "                 With 0 you watch computer bots fight.\n"
        "\n"
        "- `-r`:          (Random) set a fixed random seed.\n"
        "\n"
        "                 If given, it is always used across restarts. Otherwise, a new\n"
        "                 seed is chosen for every restart.\n"
        "\n"
        "                 This is the only source of randomness in the whole engine.\n"
        "                 Fixing it to a given value gives reproducible games.\n"
        "\n"
        "- `-w <int>`:    (Width) world width in tiles\n"
        "\n"
        "## Debug options"
        "- `-h`:          (help) show this help\n"
        "\n"
        "- `-V`:          (Verbose) Show debug and performance information.\n"
        "\n"
        "# Controls\n"
        "\n"
        "- `q`: quit\n"
        "- `r`: restart from initial state\n"
        "- `UP` / `DOWN` / `LEFT` / `RIGHT` arrow keys: move\n"
        "- `SPACE`: step simulation if neither or -i or \"-b -H\" are given\n"
        "\n"
        "# Examples\n"
        "\n"
        "## Rogue-like TAS mode\n"
        "\n"
        "    ./prog\n"
        "\n"
        "If a player controller is present,\n"
        "then the world blocks until player makes a move (`SPACE`).\n"
        "\n"
        "By TAS, we mean that each input can be carefully constructed,\n"
        "by parts, and the world only advances when `SPACE` is pressed.\n"
        "in similar fashion to how Tool Assisted Speedruns are developed\n"
        "\n"
        "This mode may be too slow to be fun, but it allows for precise controls.\n"
        "\n"
        "E.g., to move diagonally right up, you do:\n"
        "\n"
        "- `RIGHT` (horizontal direction)\n"
        "- `UP` (vertical direction)\n"
        "- `SPACE`\n"
        "\n"
        "The default for each type of direction is to do nothing.\n"
        "E.g., to just move up:\n"
        "\n"
        "- `UP` (vertical direction)\n"
        "- `SPACE`\n"
        "\n"
        "Since there was no horizontal input, the default of not\n"
        "moving horizontally is used.\n"
        "\n"
        "You can fix some controls half-way. E.g. the following will move up:\n"
        ""
        "\n"
        "- `DOWN`\n"
        "- `UP`, overrides the previous `DOWN`\n"
        "- `SPACE`\n"
        "\n"
        "## Rogue mode\n"
        "\n"
        "    ./prog -i\n"
        "\n"
        "When the player clicks any key, the world updates.\n"
        "\n"
        "## Crypt of the NecroDancer mode\n"
        "\n"
        "   ./prog -H -b -f 2.0 -i\n"
        "\n"
        "World updates even if user does nothing, but only at 2FPS.\n"
        "\n"
        "## Continuous action game\n"
        "\n"
        "   ./prog -H -b -f 20.0 -i -w 100\n"
        "\n"
        "Same as Crypt of the NecroDancer, but with wide screen, and faster FPS.\n"
        "\n"
        "Most engines will implement this mode with floating point positions,\n"
        "but it could also be done with integers. However, we map states 1-to-1\n"
        "with screen while using floating point means that multiple states map to a\n"
        "single screen after rounding.\n"
        "\n"
        "## Non-interactive simulation\n"
        "\n"
        "    ./prog -t 10 -p 0 -d\n"
        "\n"
        "Let your AIs battle it out for 10 ticks, and at the end see all scores.\n"
        "No human intervention or GUI, everything happens as fast as possible.\n"
        "\n"
    ;
}

int main(int argc, char **argv) {
    SDL_Event event;
    std::unique_ptr<World> world;
    bool
        blockOnPlayer = true,
        display = true,
        fixedRandomSeed = false,
        holdKey = false,
        immediateAction = false,
        limitFps = false,
        showFov = false,
        verbose = false
    ;
    std::string scenario;
    double
        targetFps = 1.0,
        last_time;
    ;
    unsigned int
        nHumanPlayers = 1,
        randomSeed,
        showPlayerId = 0,
        timeLimit = 100,
        width = 100,
        windowWidthPix = 500
    ;

    // Treat CLI arguments.
    for (decltype(argc) i = 0; i < argc; ++i) {
        if (argv[i][0] == '-') {

            // Interactive play options.
            if (std::strcmp(argv[i], "-b") == 0) {
                blockOnPlayer = !blockOnPlayer;
            } else if (std::strcmp(argv[i], "-d") == 0) {
                display = !display;
            } else if (std::strcmp(argv[i], "-f") == 0) {
                limitFps = !limitFps;
                targetFps = std::strtod(argv[i + 1], NULL);
            } else if (std::strcmp(argv[i], "-H") == 0) {
                holdKey = !holdKey;
            } else if (std::strcmp(argv[i], "-i") == 0) {
                immediateAction = !immediateAction;
            } else if (std::strcmp(argv[i], "-v") == 0) {
                showPlayerId = std::strtol(argv[i + 1], NULL, 10);
                showFov = true;
            } else if (std::strcmp(argv[i], "-W") == 0) {
                windowWidthPix = std::strtol(argv[i + 1], NULL, 10);

            // World state options.
            } else if (std::strcmp(argv[i], "-p") == 0) {
                nHumanPlayers = std::strtol(argv[i + 1], NULL, 10);
            } else if (std::strcmp(argv[i], "-r") == 0) {
                randomSeed = std::strtol(argv[i + 1], NULL, 10);
                fixedRandomSeed = true;
            } else if (std::strcmp(argv[i], "-s") == 0) {
                scenario = argv[i + 1];
            } else if (std::strcmp(argv[i], "-t") == 0) {
                timeLimit = std::strtol(argv[i + 1], NULL, 10);
            } else if (std::strcmp(argv[i], "-w") == 0) {
                width = std::strtol(argv[i + 1], NULL, 10);

            // Debug options.
            } else if (std::strcmp(argv[i], "-h") == 0) {
                printHelp();
                std::exit(EXIT_SUCCESS);
            } else if (std::strcmp(argv[i], "-V") == 0) {
                verbose = !verbose;
            } else {
                printHelp();
                std::exit(EXIT_FAILURE);
            }
        }
    }
    auto windowHeightPix = windowWidthPix;
    auto targetSpf = 1.0 / targetFps;
    auto height = width;
    if (!display) {
        nHumanPlayers = 0;
    }

    world = std::make_unique<World>(
        width,
        height,
        display,
        windowWidthPix,
        windowHeightPix,
        showPlayerId,
        showFov,
        fixedRandomSeed,
        randomSeed,
        nHumanPlayers,
        std::move(scenario),
        timeLimit,
        verbose
    );
main_loop:
    last_time = utils::get_secs();

    // Keyboard state.
    int numkeys;
    const Uint8 * keyboardState = SDL_GetKeyboardState(&numkeys);
    size_t keyboardStateSize = numkeys * sizeof(*keyboardState);
    auto lastKeyboardState = std::make_unique<Uint8[]>(keyboardStateSize);

    // Human actions.
    std::vector<std::unique_ptr<Action>> humanActions;
    for (decltype(world->getNHumanActions()) i = 0; i < world->getNHumanActions(); ++i) {
        humanActions.push_back(std::make_unique<Action>());
    }

    while (!world->isGameOver()) {
        world->draw();
        double nextTarget = last_time + targetSpf;
        decltype(humanActions.size()) currentActionIdx = 0;
        bool needMoreHumanActions = currentActionIdx < world->getNHumanActions();
        for (auto &action : humanActions) {
            action->reset();
        }
        // Action being currently built, possibly with multiple keypresses.
        Action currentAction, holdAction;
        bool loop = true;
        do {
            double slack = nextTarget - utils::get_secs();
            bool
                slackOver = (slack < 0.0),
                currentActionModified = false,
                holdActionDone = false
            ;
            if (display) {
                std::memcpy(lastKeyboardState.get(), keyboardState, keyboardStateSize);
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_QUIT) {
                        goto quit;
                    }
                }

                // Immediate action controls that don't wait for FPS limiting.
                if (keyboardState[SDL_SCANCODE_Q]) {
                    goto quit;
                }
                if (keyboardState[SDL_SCANCODE_R]) {
                    world->reset();
                    goto main_loop;
                }

                // Actions that may be built with multiple key-presses.
                if (needMoreHumanActions) {
                    currentActionModified = holdControls(
                        keyboardState,
                        lastKeyboardState.get(),
                        false,
                        currentAction
                    );
                    if (
                        activateKey(SDL_SCANCODE_SPACE, keyboardState, lastKeyboardState.get(), false)
                        || (currentActionModified && immediateAction)
                    ) {
                        *humanActions[currentActionIdx] = currentAction;
                        currentActionIdx++;
                        needMoreHumanActions = currentActionIdx < world->getNHumanActions();
                        currentAction.reset();
                        currentActionModified = false;
                    }
                }
                if (slackOver && holdKey && world->getNHumanActions() > 0) {
                    holdAction.reset();
                    bool holdActionModified = holdControls(
                        keyboardState,
                        lastKeyboardState.get(),
                        true,
                        currentAction
                    );
                    if (holdActionModified) {
                        *humanActions[0] = currentAction;
                        holdActionDone = true;
                    }
                }
            }
            loop =
                !holdActionDone
                &&
                (
                    (blockOnPlayer && needMoreHumanActions)
                    ||
                    (limitFps && !slackOver)
                )
            ;
        } while (loop);
        last_time = utils::get_secs();
        world->update(humanActions);
    }
    world->printScores();
quit:
    return EXIT_SUCCESS;
}
