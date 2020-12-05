/**
	Basic ground enemy.
	@file EnemySlime.cpp
	@author David Mahany (djmahany@wpi.edu)
*/

#include "EnemySlime.h"

// Engine includes.
#include "dragonfly/WorldManager.h"
#include "dragonfly/EventStep.h"

EnemySlime::EnemySlime() {
    setType("Slime");
	hasGravity(true);
    setSprite("slime");
}

int EnemySlime::eventHandler(const df::Event* ev) {

    if(ev->getType() == df::STEP_EVENT) {
        bool ground = isGrounded();

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
                    if(dist < 20) {
                        nearPlayer = true;
                    }
                    float dx = getPosition().getX() - toUpdate.currentObject()->getPosition().getX();
                    hDir = dx > 0 ? -0.5f : 0.5f;
                }
                toUpdate.next();
            }
        }

        if(ground && nearPlayer) jumpCooldown--;
        if(!nearPlayer) {
            resetJumpCooldown();

            if(ground && rand() % 1000 == 0) {
                hDir = ((rand() % 100) / 100.0f - 0.5f) * 0.4f;
                float jumpStrength = 0.4f;
                jumpStrength += (rand() % 100) / 100.0f * 0.05f;
                setVelocity({getVelocity().getX() + hDir, -jumpStrength});
            }
        }

        if(ground) {
            setVelocity(getVelocity() * 0.8f);
        }

        if(jumpCooldown <= 0) {
            float jumpStrength = 0.5f;
            jumpStrength += (rand() % 100) / 100.0f * 0.15f;
            setVelocity({getVelocity().getX() + hDir, -jumpStrength});

            resetJumpCooldown();
        }

        if(!ground) {
            getAnimation()->setIndex(0);
            getAnimation()->setSlowdownCount(-1);
        } else {
            if(getAnimation()->getSlowdownCount() == -1) {
                getAnimation()->setIndex(1);
                getAnimation()->setSlowdownCount(16);
            }
            if(jumpCooldown < 60) {
                // skip every 2nd and 3rd frame (effective slowdown of 9)
                getAnimation()->setSlowdownCount(getAnimation()->getSlowdownCount() + 1 + getAnimation()->getSlowdownCount() % 2);
            }
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
