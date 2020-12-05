/**
	The player.
	@file Player.cpp
	@author David Mahany (djmahany@wpi.edu)
*/

#include "Player.h"

// Engine includes.
#include "dragonfly/GameManager.h"
#include "dragonfly/ResourceManager.h"
#include "dragonfly/InputManager.h"
#include "dragonfly/WorldManager.h"
#include "dragonfly/EventStep.h"
#include "dragonfly/EventCollision.h"
#include "dragonfly/EventKeyboard.h"
#include "dragonfly/utility.h"

// Game includes.
#include "EnemyMaster.h"
#include "GameOver.h"

Player::Player() {
	setType("Player");

	// TODO: we shouldn't need to set boxes manually. setSprite does it for us
	auto b = getBox();
	b = df::Box(df::Vector(b.getCorner().getX() + 0.25f, b.getCorner().getY() + 0.5f), b.getHorizontal() - 0.5f, b.getVertical() - 0.5f);
	//setBox(b);

	hasGravity(true);
	//registerInterest(df::STEP_EVENT);
	registerInterest(df::COLLISION_EVENT);
	registerInterest(df::KEYBOARD_EVENT);

	setSprite("player-bounds");
}

int Player::eventHandler(const df::Event* p_e) {

	if(p_e->getType() == df::STEP_EVENT) {
		tickMovement();
		return 1;
	} else if(p_e->getType() == df::KEYBOARD_EVENT) {
		df::EventKeyboard* ke = (df::EventKeyboard*)p_e;

		if (ke->getKey() == df::Keyboard::Key::SPACE) {
			if (ke->getKeyboardAction() == df::EventKeyboardAction::KEY_PRESSED) {
				jump();
				return 1;
			}
			else if (ke->getKeyboardAction() == df::EventKeyboardAction::KEY_RELEASED) {
				endJump();
				return 1;
			}
		}
		if (ke->getKey() == df::Keyboard::Key::Q) {
			if (ke->getKeyboardAction() == df::EventKeyboardAction::KEY_PRESSED) {
				GM.setGameOver();
				return 1;
			}
		}
		

	} else if(p_e->getType() == df::COLLISION_EVENT) {
		df::EventCollision* ce = (df::EventCollision*)p_e;
		//printf("%s %s\n", ce->getObject1()->getType().c_str(), ce->getObject2()->getType().c_str());
		bool enemy = dynamic_cast<EnemyMaster*>(ce->getObject1()) || dynamic_cast<EnemyMaster*>(ce->getObject2());
		if(enemy) {
			setVelocity({0, 0});
			WM.markForDelete(this);
			new GameOver;
		}
	}

	return 0;
}

int Player::draw() {

	// since we the player's legs and body animate separately, we need to use a combination of multiple sprites
	// the Object's "real" sprite (using setSprite) is "player-bounds" which is a 2x3, completely empty sprite only used for collisions

	// TODO: attacking animations, etc
	RM.getSprite("player-idle-body")->draw(0, this->getPosition() + df::Vector(-1, -1));


	// decide which frame for the legs
	// 0 = "/ \"
	// 1 = " | "
	int legFrame = walkCycle > 0.5f ? 1 : 0;

	if(!isGrounded()) {
		// always use "/ \" in the air
		legFrame = 0;
	} else {
		if(abs(getVelocity().getX()) < 0.1) {
			// always use " | " if on the ground and standing still
			legFrame = 1;
			walkCycle = 0;
		} else {
			// cycle between frames based on x velocity
			walkCycle += abs(getVelocity().getX()) / 3.0f;
			while(walkCycle > 1.0f) walkCycle -= 1.0f;
		}
	}

	RM.getSprite("player-walk")->draw(legFrame, this->getPosition() + df::Vector(-1, 0.2f + 1));

	return Object::draw();
}

void Player::tickMovement() {
	float vx = getVelocity().getX();
	float vy = getVelocity().getY();

	// state of the relevant inputs
	bool aDown = IM.isKeyDown(df::Keyboard::Key::A);
	bool dDown = IM.isKeyDown(df::Keyboard::Key::D);

	bool grounded = isGrounded();

	// -X if holding A, +X if holding D
	float xMove = (aDown ? -0.02f : 0.0f) + (dDown ? 0.02f : 0.0f);

	// when not pressing left or right
	if(!aDown && !dDown) {
		// slow down 
		float factor = grounded ? 0.85f : 0.98f;
		vx *= factor;
	}

	// when pressing the opposite direction as your current velocity
	if((vx > 0 && xMove < 0) || (vx < 0 && xMove > 0)) {
		// give a boost to movement power (helps when "turning around")
		float factor = grounded ? 3.0f : 2.0f;
		xMove *= factor;
	}

	// when on the ground and pressing the same direction as your current velocity
	if((vx < 0 && xMove < 0) || (vx > 0 && xMove > 0)) {
		// help push towards max speed
		float maxSpeed = 1.0f;
		float thru = maxSpeed - abs(vx);

		int sign = (vx < 0 ? -1 : 1);
		float factor = grounded ? 20.0f : 40.0f;
		xMove += thru / factor * sign;
	}

	// add the movement velocity
	vx += xMove;

	// dampening
	vx *= 0.98f;
	vy *= 0.99f;

	setVelocity({vx, vy});
}

void Player::jump() {
	if(isGrounded()) setVelocity({getVelocity().getX(), -0.6f});
}

void Player::endJump() {
	// allows "cancelling" so you can do short jumps by releasing the key early and high jumps by holding the key until the max height
	if(getVelocity().getY() < 0) {
		setVelocity({getVelocity().getX(), getVelocity().getY() * 0.5f});
	}
}
