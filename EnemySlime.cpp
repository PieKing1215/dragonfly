/**
	Basic ground enemy.
	@file EnemySlime.cpp
	@author David Mahany (djmahany@wpi.edu)
*/

#include "EnemySlime.h"

// Engine includes.
#include "dragonfly/WorldManager.h"
#include "dragonfly/ResourceManager.h"
#include "dragonfly/EventStep.h"

EnemySlime::EnemySlime() {
    setType("Slime");
	hasGravity(true);
    setSprite("slime");

    // make the hitbox smaller so it can't hit the player from as far away
    auto b = getBox();
    b = df::Box(df::Vector(b.getCorner().getX() + 0.5f, b.getCorner().getY() + 1.25f), b.getHorizontal() - 1.0f, b.getVertical() - 1.25f);
    setBox(b);
}

EnemySlime::~EnemySlime() {
    RM.getSound("slime_die")->getSound()->setVolume(50);
    RM.getSound("slime_die")->play();
}

int EnemySlime::eventHandler(const df::Event* ev) {

    if(ev->getType() == df::STEP_EVENT) {
        bool ground = isGrounded();

        // only scan for the player once in a while (helps with performance)
        scanCooldown--;
        if(scanCooldown <= 0) {
            nearPlayer = false;
            hDir = 0.0f;
            scanCooldown = 15;

            df::ObjectList ol = WM.getSceneGraph().activeObjects();
            df::ObjectListIterator toUpdate(&ol);
            toUpdate.first();
            std::string x = "";
            while(!toUpdate.isDone() && toUpdate.currentObject()) {
                if(toUpdate.currentObject()->getType() == "Player") {
                    float dist = (getPosition() - toUpdate.currentObject()->getPosition()).getMagnitude();
                    // if in range
                    if(dist < 20) {
                        nearPlayer = true;
                    }

                    // determine if player is to the left or right
                    float dx = getPosition().getX() - toUpdate.currentObject()->getPosition().getX();
                    hDir = dx > 0 ? -0.5f : 0.5f;
                }
                toUpdate.next();
            }
        }

        // if doesnt see the player
        if(!nearPlayer) {
            resetJumpCooldown();

            // occasionally do small hops
            if(ground && rand() % 1000 == 0) {
                hDir = ((rand() % 100) / 100.0f - 0.5f) * 0.4f;
                float jumpStrength = 0.4f;
                jumpStrength += (rand() % 100) / 100.0f * 0.05f;
                setVelocity({getVelocity().getX() + hDir, -jumpStrength});
            }
        }

        // friction with ground
        if(ground) {
            setVelocity(getVelocity() * 0.8f);
        }

        // only warm up to jump when on the ground and if sees player
        if(ground && nearPlayer) jumpCooldown--;

        if(jumpCooldown <= 0) {
            // jump towards player
            RM.getSound("slime_jump")->getSound()->setVolume(50);
            RM.getSound("slime_jump")->play();

            float jumpStrength = 0.5f;
            jumpStrength += (rand() % 100) / 100.0f * 0.15f;
            setVelocity({getVelocity().getX() + hDir, -jumpStrength});

            resetJumpCooldown();
        }

        if(!ground) {
            // always use first frame when in the air
            getAnimation()->setIndex(0);
            getAnimation()->setSlowdownCount(-1);
        } else {
            // reset animation after landing
            if(getAnimation()->getSlowdownCount() == -1) {
                getAnimation()->setIndex(1);
                getAnimation()->setSlowdownCount(16);
            }

            // speed up when about to jump
            if(jumpCooldown < 60) {
                // skip every 2nd and 3rd frame (effective slowdown of 9)
                getAnimation()->setSlowdownCount(getAnimation()->getSlowdownCount() + 1 + getAnimation()->getSlowdownCount() % 2);
            }

            // speed up slightly when player nearby
            if(nearPlayer) {
                // skip every 3rd frame (effective slowdown of 16)
                getAnimation()->setSlowdownCount(getAnimation()->getSlowdownCount() + getAnimation()->getSlowdownCount() % 2);
            }
        }
    }

    return 0;
}

void EnemySlime::resetJumpCooldown() {
    jumpCooldown = 90 + rand() % 120;
}
