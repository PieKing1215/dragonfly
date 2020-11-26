/**
	World manager.
	@file WorldManager.cpp
	@author Dennis James Stelmach (dstelmach2@wpi.edu)
*/

#include "WorldManager.h"

// Engine includes.
#include "DisplayManager.h"
#include "LogManager.h"
#include "EventCollision.h"
#include "EventStep.h"
#include "EventOut.h"
#include "ViewObject.h"
#include "utility.h"

df::WorldManager::WorldManager() {
	setType("WorldManager");
	df::Vector v = df::pixelsToSpaces(df::Vector(DM.getHorizontalPixels(), DM.getVerticalPixels())), w(DM.getHorizontal(), DM.getVertical());
	if (w.getX() < v.getX()) {
		w.setX(v.getX());
	}
	if (w.getY() < v.getY()) {
		w.setY(v.getY());
	}
	boundary = df::Box(df::Vector() - ((w - v) / 2.0), w.getX(), w.getY());
	view = df::Box(df::Vector(), v.getX(), v.getY());
	p_view_following = NULL;
	view_slack = df::Vector();
	m_deletions;
	scene_graph = df::SceneGraph();
}

df::WorldManager::WorldManager(WorldManager const&) {
	setType("WorldManager");
	df::Vector w(DM.getHorizontal(), DM.getVertical()), v = df::pixelsToSpaces(df::Vector(DM.getHorizontalPixels(), DM.getVerticalPixels()));
	boundary = df::Box(df::Vector(), w.getX(), w.getY());
	view = df::Box((w - v) / 2.0, v.getX(), v.getY());
	p_view_following = NULL;
	view_slack = df::Vector();
	m_deletions;
	scene_graph = df::SceneGraph();
}

void df::WorldManager::operator=(WorldManager const&) {

}

df::WorldManager& df::WorldManager::getInstance() {
	static WorldManager worldManager;
	return worldManager;
}

int df::WorldManager::startUp() {
	if (!LM.isStarted()) {
		return -1;
	}
	if (!DM.isStarted()) {
		writeLog("ERROR", "Error starting WorldManager. DisplayManager not started yet.");
		return -1;
	}
	df::Manager::startUp();
	writeLog("", "Size of world: %f x %f.", getBoundary().getHorizontal(), getBoundary().getVertical());
	writeLog("", "Size of viewable area: %f x %f.", getView().getHorizontal(), getView().getVertical());
	writeLog("", "Top-left corner of world: %s.", df::toString(getBoundary().getCorner()).c_str());
	return 0;
}

void df::WorldManager::shutDown() {
	// Destroy all objects in the scene graph.
	writeLog("", "Killing all Objects.");
	m_deletions = scene_graph.activeObjects() + scene_graph.inactiveObjects();
	df::ObjectListIterator li(&m_deletions);
	li.first();
	while (!li.isDone()) {
		delete li.currentObject();
		li.next();
	}
	m_deletions.clear();
	// Clear object lists.
	scene_graph.activeObjects().clear();
	scene_graph.inactiveObjects().clear();
	scene_graph.solidObjects().clear();
	for (int i = 0; i < df::MAX_ALTITUDE + 1; i++) {
		scene_graph.visibleObjects(i).clear();
	}
	Manager::shutDown();
}

int df::WorldManager::insertObject(df::Object* p_o) {
	// Check object is not inserted already
	df::ObjectList ol = getAllObjects();
	df::ObjectListIterator li(&ol);
	li.first();
	while (!li.isDone() && li.currentObject()) {
		if (li.currentObject() == p_o) {
			return 0;
		}
		li.next();
	}
	// If there are collisions on spawn.
	if (getCollisions(p_o).getCount() > 0) {
		df::Vector v;
		switch (p_o->getOverlapHandle()) {
		// Object should die.
		case df::OverlapHandle::DO_NOT_SPAWN:
			writeLog("ERROR", "Error inserting object '%s %d'. Spawn location overlaps with another object. Ignoring.", p_o->getType().c_str(), p_o->getId());
			return -1;
		// Object should try to find a location with 10 units nearby.
		case df::OverlapHandle::ATTEMPT_TO_MOVE:
			writeLog("ALERT", "Error inserting object '%s %d'. Collision on change. Attempting to move.", p_o->getType().c_str(), p_o->getId());
			if (p_o->tryToMove() || getCollisions(p_o).getCount() > 0) {
				writeLog("ERROR", "No nearby locations found. Ignoring.");
				return -1;
			}
			writeLog("", "Location nearby found. Moved to %s.", df::toString(p_o->getPosition()).c_str());
			break;
		// Object should spawn anyways.
		case df::OverlapHandle::SPAWN_ALWAYS:
			writeLog("WARN!", "Error inserting object '%s %d'. Collision on change. Inserting anyways. Prepare for a probable crash.", p_o->getType().c_str(), p_o->getId());
		}
	}
	return scene_graph.insertObject(p_o);
}

int df::WorldManager::removeObject(df::Object* p_o) {
	return scene_graph.removeObject(p_o);
}

df::ObjectList df::WorldManager::getAllObjects() const {
	return scene_graph.activeObjects() + scene_graph.inactiveObjects();
}

df::ObjectList df::WorldManager::objectsOfType(std::string type) const {
	df::ObjectList list, ol = scene_graph.activeObjects();
	df::ObjectListIterator li(&ol);
	li.first();
	while (!li.isDone() && li.currentObject()) {
		if (li.currentObject()->getType() == type) {
			list.insert(li.currentObject());
		}
	}
	return list;
}

void df::WorldManager::update(int step_count) {
	// Delete pending objects.
	df::ObjectListIterator toDelete(&m_deletions);
	toDelete.first();
	while (!toDelete.isDone()) {
		scene_graph.removeObject(toDelete.currentObject());
		delete toDelete.currentObject();
		toDelete.next();
	}
	m_deletions.clear();
	// Move and dispatch step event for all active objects.
	df::ObjectList ol = scene_graph.activeObjects();
	df::ObjectListIterator toUpdate(&ol);
	toUpdate.first();
	df::EventStep e_step(step_count);
	std::string x = "";
	while (!toUpdate.isDone() && toUpdate.currentObject()) {
		df::Vector new_pos = toUpdate.currentObject()->predictPosition();
		if (new_pos != toUpdate.currentObject()->getPosition()) {
			moveObject(toUpdate.currentObject(), new_pos);
		}
		toUpdate.currentObject()->eventHandler(&e_step);
		toUpdate.next();
	}
}

int df::WorldManager::markForDelete(Object* p_o) {
	// Check object is not already marked for deletion.
	df::ObjectListIterator li(&m_deletions);
	li.first();
	while (!li.isDone() && li.currentObject()) {
		if (li.currentObject() == p_o) {
			return 0;
		}
		li.next();
	}
	m_deletions.insert(p_o);
	return 0;
}

void df::WorldManager::draw() {
	// Draw objects in increasing altitude to keep z-index consistent.
	for (int alt = 0; alt < df::MAX_ALTITUDE; alt++) {
		df::ObjectList ol = scene_graph.activeObjects();
		df::ObjectListIterator li(&ol);
		li.first();
		while (!li.isDone() && li.currentObject()) {
			// Only draw if on current layer and is (in bounds or a is view object)
			if (li.currentObject()->getAltitude() == alt && (df::boxIntersectsBox(df::getWorldBox(li.currentObject()), view) || dynamic_cast<df::ViewObject*>(li.currentObject()))) {
				li.currentObject()->draw();
			}
			li.next();
		}
	}
}

int df::WorldManager::moveObject(df::Object* p_o, Vector where) {
	if (!p_o) { // Can't move what doesn't exist.
		return -1;
	}
	// Check for collision that will impede movement.
	if (p_o->isSolid()) {
		df::ObjectList list = getCollisions(p_o, where);
		if (!list.isEmpty()) {
			bool do_move = true;
			df::ObjectListIterator li(&list);
			li.first();
			while (!li.isDone() && li.currentObject()) {
				df::Object* p_temp_o = li.currentObject();
				df::EventCollision c(p_o, p_temp_o, where);
				// Dispatch collision event to both objects.
				p_o->eventHandler(&c);
				p_temp_o->eventHandler(&c);
				if ((p_o->getSolidness() == df::Solidness::HARD && p_temp_o->getSolidness() == df::Solidness::HARD) || (p_o->getNoSoft() && p_temp_o->getSolidness() == df::Solidness::SOFT)) {
					do_move = false; // Do not allow movement if both objects are HARD or the main object is impeded by SOFT and the other object is SOFT.
				}
				li.next();
			}
			if (!do_move) {
				return -1;
			}
		}
	}
	df::Box box0 = df::getWorldBox(p_o);
	p_o->setPosition(where);
	if (p_view_following == p_o) {
		float view_center_x = view.getCorner().getX() + view.getHorizontal() / 2;
		float view_center_y = view.getCorner().getY() + view.getVertical() / 2;

		// Compute inner "slack" box edges.
		float left = view_center_x - view.getHorizontal() * view_slack.getX() / 2;
		float right = view_center_x + view.getHorizontal() * view_slack.getX() / 2;
		float top = view_center_y - view.getVertical() * view_slack.getY() / 2;
		float bottom = view_center_y + view.getVertical() * view_slack.getY() / 2;

		df::Vector new_pos = p_o->getPosition();

		// Move view right/left?
		if (new_pos.getX() < left) {
			view_center_x -= left - new_pos.getX();
		}
		else if (new_pos.getX() > right) {
			view_center_x += new_pos.getX() - right;
		}

		// Move up/down?
		if (new_pos.getY() < top) {
			view_center_y -= top - new_pos.getY();
		}
		else if (new_pos.getY() > bottom) {
			view_center_y += new_pos.getY() - bottom;
		}

		// Set new view position.
		setViewPosition(Vector(view_center_x, view_center_y));
	}
	df::Box box1 = df::getWorldBox(p_o);
	// Dispatch out event if object has moved off world or screen. Hard out event has priority as it implies a soft out event.
	if (df::boxIntersectsBox(box0, boundary) && !df::boxIntersectsBox(box1, boundary)) {
		const EventOut ov(true);
		p_o->eventHandler(&ov);
	}
	else if (df::boxIntersectsBox(box0, view) && !df::boxIntersectsBox(box1, view)) {
		const EventOut ov(false);
		p_o->eventHandler(&ov);
	}
	return 0;
}

df::ObjectList df::WorldManager::getCollisions(const df::Object* p_o) const {
	return getCollisions(p_o, p_o->getPosition());
}

df::ObjectList df::WorldManager::getCollisions(const df::Object* p_o, df::Vector where) const {
	df::ObjectList collList;
	if (!p_o) {
		return collList;
	}
	if (!p_o->isSolid()) {
		return collList;
	}
	const df::ObjectList ol = scene_graph.activeObjects();
	df::ObjectListIterator li(&ol);
	li.first();
	while (!li.isDone() && li.currentObject()) {
		df::Object* p_temp = li.currentObject();
		if (p_temp && p_o != p_temp && df::boxIntersectsBox(df::getWorldBox(p_o, where), df::getWorldBox(p_temp)) && p_temp->isSolid()) {
			collList.insert(p_temp); // Add item to collision list if overlaps with p_o at where.
		}
		li.next();
	}
	return collList;
}

int df::WorldManager::onEvent(const Event* p_event) const {
	if (!p_event) {
		return -1;
	}
	int count = 0;
	df::ObjectList ol = getAllObjects();
	df::ObjectListIterator li(&ol);
	li.first();
	while (!li.isDone() && li.currentObject()) {
		li.currentObject()->eventHandler(p_event);
		li.next();
		count++;
	}
	return count;
}

void df::WorldManager::setBoundary(df::Box new_boundary) {
	boundary = new_boundary;
}

df::Box df::WorldManager::getBoundary() const {
	return boundary;
}

void df::WorldManager::setView(df::Box new_view) {
	view = new_view;
}

df::Box df::WorldManager::getView() const {
	return view;
}

void df::WorldManager::setViewPosition(df::Vector view_pos) {
	float x = view_pos.getX() - view.getHorizontal() / 2;
	if (x + view.getHorizontal() > boundary.getHorizontal()) {
		x = boundary.getHorizontal() - view.getHorizontal();
	}
	if (x < 0) {
		x = 0;
	}
	float y = view_pos.getY() - view.getVertical() / 2;
	if (y + view.getVertical() > boundary.getVertical()) {
		y = boundary.getVertical() - view.getVertical();
	}
	if (y < 0) {
		y = 0;
	}
	df::Vector new_corner(x, y);
	view.setCorner(new_corner);
}

int df::WorldManager::setViewFollowing(df::Object* p_new_view_following) {
	if (!p_new_view_following) {
		p_view_following = NULL;
		return 0;
	}
	df::ObjectList ol = getAllObjects();
	df::ObjectListIterator li(&ol);
	li.first();
	bool found = false;
	while (!li.isDone() && li.currentObject()) {
		if (li.currentObject() == p_new_view_following) {
			found = true;
			break;
		}
		li.next();
	}
	if (found) {
		p_view_following = p_new_view_following;
		setViewPosition(p_view_following->getPosition());
		return 0;
	}
	return -1;
}

void df::WorldManager::setViewSlack(df::Vector new_view_slack) {
	view_slack = new_view_slack;
}

df::Vector df::WorldManager::getViewSlack() const {
	return view_slack;
}

df::SceneGraph df::WorldManager::getSceneGraph() const {
	return scene_graph;
}

df::Vector df::WorldManager::attemptMove(df::Object* p_o, int max_out) {
	df::Vector pos = p_o->getPosition();
	for (int i = 1; i <= max_out; i++) {
		df::Vector v = attemptMoveInRadius(p_o, i);
		if (v != pos) {
			return v;
		}
	}
	return pos;
}

df::Vector df::WorldManager::attemptMoveInRadius(Object* p_o, int radius) {
	df::Vector v;
	for (int i = -1; i <= 1; i += 2) {
		for (int x = -1.0 * radius; x <= radius; x++) {
			v = p_o->getPosition() - df::Vector(radius + x, radius * i);
			if (getCollisions(p_o, v).getCount() == 0) {
				return v;
			}
		}
	}
	for (int i = -1; i <= 1; i += 2) {
		for (int y = -1.0 * radius; y <= radius; y++) {
			v = p_o->getPosition() - df::Vector(radius * i, radius + y);
			if (getCollisions(p_o, v).getCount() == 0) {
				return v;
			}
		}
	}
	return p_o->getPosition();
}