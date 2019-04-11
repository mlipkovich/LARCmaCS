#include "fieldScene.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#define _USE_MATH_DEFINES
#include <math.h>
#include <QTextStream>

FieldScene::FieldScene(QObject *parent) :
	QGraphicsScene(parent)
{
	connect(this, SIGNAL(reDrawScene()), this, SLOT(update()));
	setBackgroundBrush (QBrush(QColor(0, 0x91, 0x19, 255), Qt::SolidPattern));
	shutdownSoccerView = false;
	ksize = 10;
	LoadFieldGeometry();
	ConstructField();
	fieldBrush = new QBrush(Qt::NoBrush);
	fieldLinePen = new QPen();
	fieldLinePen->setColor(Qt::white);
	fieldLinePen->setWidth(2);
	fieldLinePen->setJoinStyle(Qt::MiterJoin);
	fieldItem = this->addPath(*field, *fieldLinePen, *fieldBrush);
	fieldItem->setZValue(0);
	mDrawTimer.setInterval(33);
	connect(&mDrawTimer, SIGNAL(timeout()), this, SLOT(updateFrame()));
}

FieldScene::~FieldScene()
{
	mDrawTimer.stop();
}

void FieldScene::updateFrame()
{
	QSharedPointer<pair<QSharedPointer<QVector<QSharedPointer<SSL_WrapperPacket> > >, QSharedPointer<SSL_WrapperPacket> > > tmp = mReceiver->getVisionData();
	UpdateField(tmp->first, tmp->second);
}

void FieldScene::start()
{
	mDrawTimer.start();
}

void FieldScene::setReceiver(Receiver * receiver)
{
	mReceiver = receiver;
}

void FieldScene::AddRobot(Robot *robot)
{
	robots.append(robot);
	this->addItem(robot);
}

#ifndef OLD_SSL_PROTO
void FieldScene::UpdateFieldGeometry(QSharedPointer<SSL_WrapperPacket> packet) {
	LoadFieldGeometry(packet->geometry().field());
}
#endif

void FieldScene::UpdateField(QSharedPointer<QVector<QSharedPointer<SSL_WrapperPacket> > > detection, QSharedPointer<SSL_WrapperPacket> geometry)
{
	if (!geometry.isNull()) {
#ifndef OLD_SSL_PROTO
		UpdateFieldGeometry(geometry);
#else
		UpdateGeometry(geometry->geometry().field());
#endif
	}
	for (int i = 0; i < detection->size(); i++) {
		if (!detection->at(i).isNull()) {
			if (detection->at(i)->has_geometry()) {
#ifndef OLD_SSL_PROTO
				UpdateFieldGeometry(detection->at(i));
#else
				UpdateGeometry(detection->at(i)->geometry().field());
#endif
			}
			UpdateRobots(detection->at(i));
		}
	}
	emit reDrawScene();
}

void FieldScene::updateRobot(const SSL_DetectionRobot & robot, int team, unsigned int camID)
{
	int id, robotNum;
	double x, y, orientation, conf;

	conf = robot.confidence();
	x = robot.x();
	y = -robot.y();

	orientation = Robot::NAOrientation;
	if (robot.has_orientation()) {
		orientation = robot.orientation() * 180.0 / M_PI;
	}

	QString label;
	robotNum = Robot::robotNotFound;
	id = Robot::NA;
	if (robot.has_robot_id()) {
		id = robot.robot_id();
		label.setNum (id + 1, 10);
		for (int j = 0; j < robots.size(); j++) {
			if (robots[j]->getTeamID() == team && robots[j]->getRobotID() == id) {
				robotNum = j;
				robots[robotNum]->SetPose(x, -y, orientation, conf);
				break;
			}
		}
	} else {
		label = "?";
	}

	if (robotNum == Robot::robotNotFound) {
		AddRobot(new Robot(x, y, orientation, team, id, camID, conf));
		robotNum = robots.size() - 1;
	}

	label = label.toUpper();
	robots[robotNum]->setRobotLabel(label);
}

void FieldScene::UpdateRobots(QSharedPointer<SSL_WrapperPacket> packet)
{
	SSL_DetectionFrame detection = packet->detection();
	int robots_blue_n = detection.robots_blue_size();
	int robots_yellow_n = detection.robots_yellow_size();
	unsigned int camID = detection.camera_id();

	SSL_DetectionRobot robot;

	while (robots.size() > Constants::maxNumOfRobots) {
		this->removeItem(robots.last());
		robots.removeLast();
	}

	for (int j = 0; j < robots.size(); j++) {
		if (robots[j]->getCamID() == detection.camera_id()) {
			robots[j]->setRobotConfidence(0);
		}
	}

	for (int i = 0; i < robots_blue_n; i++) {
		robot = detection.robots_blue(i);
		updateRobot(robot, Robot::teamBlue, camID);
	}

	for (int i = 0; i < robots_yellow_n; i++) {
		robot = detection.robots_yellow(i);
		updateRobot(robot, Robot::teamYellow, camID);
	}

	//update balls

	QVector<QGraphicsEllipseItem*> tmp;
	while(camID + 1 > ballItems.size())
		ballItems.append(tmp);
	if (ballItems[camID].size() < detection.balls_size()) {
		//need to allocate some space for the new balls
		QPen pen(QColor(0xcd,0x59,0x00,0xff));
		pen.setWidth (2);
		QBrush brush (QColor(0xff, 0x81, 0x00, 0xff), Qt::SolidPattern);
		while (detection.balls_size() > ballItems[camID].size()) {
			ballItems[camID].append(this->addEllipse(0, 0, 12, 12, pen, brush));
			ballItems[camID][ballItems[camID].size() - 1]->setZValue(2);
		}
	} else if (ballItems[camID].size() > detection.balls_size()) {
	//need to delete some balls
		while(ballItems[camID].size() > detection.balls_size()) {
			this->removeItem(ballItems[camID][0]);
			ballItems[camID].remove(0);
		}
	}
	for (int i = 0; i < detection.balls_size(); i++) {
		ballItems[camID][i]->setPos(detection.balls(i).x()/ksize-6, -detection.balls(i).y()/ksize-6);
	}
}

void FieldScene::UpdateGeometry(SSL_GeometryFieldSize fieldSize) {
	LoadFieldGeometry(fieldSize);
}

void FieldScene::ClearField()
{
	this->clear();
	LoadFieldGeometry();
	field_arcs.clear();
	field_lines.clear();
	ConstructField();
	fieldItem = this->addPath(*field, *fieldLinePen, *fieldBrush);
	robots.clear();
	ballItems.clear();
	emit reDrawScene();
}

void FieldScene::ConstructField()
{
	field = new QPainterPath();

	QFont qfont;
	qfont.setPixelSize(20);
	field->addText(QPoint(field_length/(2*ksize) + 100 / ksize, field_width/(2*ksize)+200/ksize), qfont , "(" + QString::number(field_length / 2) + ", " + QString::number(-field_width / 2) + ")");
	field->addText(QPoint(-field_length/(2*ksize)-1400/ksize, field_width/(2*ksize)+200/ksize), qfont , "(" + QString::number(-field_length / 2) + ", " + QString::number(-field_width / 2) + ")");
	field->addText(QPoint(-field_length/(2*ksize)-1300/ksize, -field_width/(2*ksize)-100/ksize), qfont , "(" + QString::number(-field_length / 2) + ", " + QString::number(field_width / 2) + ")");
	field->addText(QPoint(field_length/(2*ksize)+100/ksize, -field_width/(2*ksize)-100/ksize), qfont , "(" + QString::number(field_length / 2) + ", " + QString::number(field_width / 2) + ")");

#ifdef OLD_SSL_PROTO
	field->moveTo(0, -field_width / (2 * ksize));
	field->lineTo(0, field_width / (2 * ksize));

	field->addEllipse(-2 * center_circle_radius / (2 * ksize), -center_circle_radius / ksize,
						4 * center_circle_radius / (2 * ksize), 2 * center_circle_radius / ksize);

	field->moveTo(field_length / (2 * ksize), -field_width / (2 * ksize));
	field->lineTo(field_length / (2 * ksize), field_width / (2 * ksize));

	field->moveTo(-field_length / (2 * ksize), -field_width / (2 * ksize));
	field->lineTo(-field_length / (2 * ksize), field_width / (2 * ksize));

	field->moveTo(-field_length / (2 * ksize), -field_width / (2 * ksize));
	field->lineTo(field_length / (2 * ksize), -field_width / (2 * ksize));

	field->moveTo(-field_length / (2 * ksize), field_width / (2 * ksize));
	field->lineTo(field_length / (2 * ksize), field_width / (2 * ksize));

	field->moveTo(field_length / (2 * ksize), goal_width / (2 * ksize));
	field->lineTo((field_length / (2 * ksize) + goal_depth/ksize), goal_width / (2 * ksize));
	field->lineTo((field_length / (2 * ksize) + goal_depth / ksize), -goal_width / (2 * ksize));
	field->lineTo(field_length / (2 * ksize), -goal_width / (2 * ksize));
	field->moveTo((field_length / (2 * ksize) - defense_radius / ksize), defense_stretch / (2 * ksize));
	field->lineTo((field_length / (2 * ksize) - defense_radius / ksize), -defense_stretch / (2 * ksize));
	field->moveTo((field_length / (2 * ksize) - defense_radius / ksize), defense_stretch / (2 * ksize));
	field->arcTo((field_length / (2 * ksize) - defense_radius / ksize), -(defense_radius / ksize - defense_stretch / (2 * ksize)), 2 * defense_radius / ksize, 2 * defense_radius / ksize, 180, 90);
	field->moveTo((field_length / (2 * ksize) - defense_radius / ksize), -defense_stretch / (2 * ksize));
	field->arcTo((field_length / (2 * ksize) - defense_radius / ksize), -(defense_radius / ksize + defense_stretch / (2 * ksize)), 2 * defense_radius / ksize, 2 * defense_radius / ksize, 180, -90);

	field->moveTo( -field_length/(2*ksize),goal_width/(2*ksize) );
	field->lineTo( - ( field_length/(2*ksize)+goal_depth/ksize ),goal_width/(2*ksize) );
	field->lineTo( - ( field_length/(2*ksize)+goal_depth/ksize ),-goal_width/(2*ksize) );
	field->lineTo( -field_length/(2*ksize),-goal_width/(2*ksize) );
	field->moveTo( - ( field_length/(2*ksize)-defense_radius/ksize ),defense_stretch/(2*ksize) );
	field->lineTo( - ( field_length/(2*ksize)-defense_radius/ksize ),-defense_stretch/(2*ksize) );
	field->moveTo( - ( field_length/(2*ksize)-defense_radius/ksize ),defense_stretch/(2*ksize) );
	field->arcTo( - ( field_length/(2*ksize)+defense_radius/ksize ),- (defense_radius/ksize-defense_stretch/(2*ksize) ),2*defense_radius/ksize,2*defense_radius/ksize,0,-90 );
	field->moveTo( - ( field_length/(2*ksize)-defense_radius/ksize ),-defense_stretch/(2*ksize) );
	field->arcTo( - ( field_length/(2*ksize)+defense_radius/ksize ),- ( defense_radius/ksize+defense_stretch/(2*ksize) ),2*defense_radius/ksize,2*defense_radius/ksize,0,90 );
#else
	if (field_lines.empty() || field_arcs.empty()) {
		field->moveTo ( 0,-field_width/(2*ksize) );
		field->lineTo ( 0,field_width/(2*ksize) );

		field->addEllipse ( -2*center_circle_radius/(2*ksize),-center_circle_radius/ksize, 4*center_circle_radius/(2*ksize),2*center_circle_radius/ksize );

		field->moveTo ( field_length/(2*ksize),-field_width/(2*ksize) );
		field->lineTo ( field_length/(2*ksize),field_width/(2*ksize) );

		field->moveTo ( -field_length/(2*ksize),-field_width/(2*ksize) );
		field->lineTo ( -field_length/(2*ksize),field_width/(2*ksize) );

		field->moveTo ( -field_length/(2*ksize),-field_width/(2*ksize) );
		field->lineTo ( field_length/(2*ksize),-field_width/(2*ksize) );

		field->moveTo ( -field_length/(2*ksize),field_width/(2*ksize) );
		field->lineTo ( field_length/(2*ksize),field_width/(2*ksize) );

		field->moveTo ( field_length/(2*ksize),goal_width/(2*ksize) );
		field->lineTo ( ( field_length/(2*ksize)+goal_depth/ksize ),goal_width/(2*ksize) );
		field->lineTo ( ( field_length/(2*ksize)+goal_depth/ksize ),-goal_width/(2*ksize) );
		field->lineTo ( field_length/(2*ksize),-goal_width/(2*ksize) );
		field->moveTo (( field_length/(2*ksize)-penalty_area_depth/ksize ),penalty_area_width/(2*ksize) );
		field->lineTo (( field_length/(2*ksize)-penalty_area_depth/ksize ),-penalty_area_width/(2*ksize) );
		field->moveTo((field_length/(2*ksize)), penalty_area_width/(2*ksize));
		field->lineTo (( field_length/(2*ksize)-penalty_area_depth/ksize ),penalty_area_width/(2*ksize) );
		field->moveTo((field_length/(2*ksize)), -penalty_area_width/(2*ksize));
		field->lineTo (( field_length/(2*ksize)-penalty_area_depth/ksize ),-penalty_area_width/(2*ksize) );

		field->moveTo ( -field_length/(2*ksize),goal_width/(2*ksize) );
		field->lineTo ( - ( field_length/(2*ksize)+goal_depth/ksize ),goal_width/(2*ksize) );
		field->lineTo ( - ( field_length/(2*ksize)+goal_depth/ksize ),-goal_width/(2*ksize) );
		field->lineTo ( -field_length/(2*ksize),-goal_width/(2*ksize) );
		field->moveTo ( - ( field_length/(2*ksize)-penalty_area_depth/ksize ),penalty_area_width/(2*ksize) );
		field->lineTo ( - ( field_length/(2*ksize)-penalty_area_depth/ksize ),-penalty_area_width/(2*ksize) );
		field->moveTo(- (field_length/(2*ksize)), penalty_area_width/(2*ksize));
		field->lineTo ( - ( field_length/(2*ksize)-penalty_area_depth/ksize ),penalty_area_width/(2*ksize) );
		field->moveTo(- (field_length/(2*ksize)), -penalty_area_width/(2*ksize));
		field->lineTo ( - ( field_length/(2*ksize)-penalty_area_depth/ksize ),-penalty_area_width/(2*ksize) );
	} else {
		for (int i = 0; i < field_lines.size(); i++) {
			field->moveTo(field_lines[i].p1().x() / ksize, field_lines[i].p1().y() / ksize);
			field->lineTo(field_lines[i].p2().x() / ksize, field_lines[i].p2().y() / ksize);
		}
		for (int i = 0; i < field_arcs.size(); i++) {
			double tmp;
			if (field_arcs[i].a2() < field_arcs[i].a1()) {
				tmp = 2 * M_PI + field_arcs[i].a2() - field_arcs[i].a1();
			} else {
				tmp = field_arcs[i].a2() - field_arcs[i].a1();
			}
			field->moveTo(field_arcs[i].center().x() / ksize + field_arcs[i].radius() / ksize, field_arcs[i].center().y() / ksize);
			field->arcTo(field_arcs[i].center().x() / ksize - field_arcs[i].radius() / ksize, field_arcs[i].center().y() / ksize - field_arcs[i].radius() / ksize, 2 * field_arcs[i].radius() / ksize, 2 * field_arcs[i].radius() / ksize, field_arcs[i].a1(), tmp/0.0175);
		}
	}
#endif
}

void FieldScene::LoadFieldGeometry()
{
#ifdef OLD_SSL_PROTO
	this->line_width = FieldConstantsRoboCup2009::line_width;
	this->field_length = FieldConstantsRoboCup2009::field_length;
	this->field_width = FieldConstantsRoboCup2009::field_width;
	this->boundary_width = FieldConstantsRoboCup2009::boundary_width;
	this->referee_width = FieldConstantsRoboCup2009::referee_width;
	this->goal_width = FieldConstantsRoboCup2009::goal_width;
	this->goal_depth = FieldConstantsRoboCup2009::goal_depth;
	this->goal_wall_width = FieldConstantsRoboCup2009::goal_wall_width;
	this->center_circle_radius = FieldConstantsRoboCup2009::center_circle_radius;
	this->defense_radius = FieldConstantsRoboCup2009::defense_radius;
	this->defense_stretch = FieldConstantsRoboCup2009::defense_stretch;
	this->free_kick_from_defense_dist = FieldConstantsRoboCup2009::free_kick_from_defense_dist;
	this->penalty_spot_from_field_line_dist = FieldConstantsRoboCup2009::penalty_spot_from_field_line_dist;
	this->penalty_line_from_spot_dist = FieldConstantsRoboCup2009::penalty_line_from_spot_dist;
#else
	this->line_width = FieldConstantsRoboCup2018A::kLineThickness;
	this->field_length = FieldConstantsRoboCup2018A::kFieldLength;
	this->field_width = FieldConstantsRoboCup2018A::kFieldWidth;
	this->boundary_width = FieldConstantsRoboCup2018A::kBoundaryWidth;
	this->goal_width = FieldConstantsRoboCup2018A::kGoalWidth;
	this->goal_depth = FieldConstantsRoboCup2018A::kGoalDepth;
	this->center_circle_radius = FieldConstantsRoboCup2018A::kCenterCircleRadius;
	this->penalty_area_depth = FieldConstantsRoboCup2018A::kPenaltyAreaDepth;
	this->penalty_area_width = FieldConstantsRoboCup2018A::kPenaltyAreaWidth;
#endif
}

void FieldScene::LoadFieldGeometry(const SSL_GeometryFieldSize & fieldSize)
{
#ifdef OLD_SSL_PROTO
	this->line_width = fieldSize.line_width();
	this->referee_width = fieldSize.referee_width();
	this->goal_wall_width = fieldSize.goal_wall_width();
	this->center_circle_radius = fieldSize.center_circle_radius();
	this->defense_radius = fieldSize.defense_radius();
	this->defense_stretch = fieldSize.defense_stretch();
	this->free_kick_from_defense_dist = fieldSize.free_kick_from_defense_dist();
	this->penalty_spot_from_field_line_dist = fieldSize.penalty_spot_from_field_line_dist();
	this->penalty_line_from_spot_dist = fieldSize.penalty_line_from_spot_dist();
#else
	this->line_width = FieldConstantsRoboCup2018A::kLineThickness;//fieldSize.line_width();
	field_lines.clear();
	field_arcs.clear();
	for (int i = 0; i < fieldSize.field_lines_size(); i++) {
		field_lines.append(fieldSize.field_lines(i));
	}
	for (int i = 0; i < fieldSize.field_arcs_size(); i++) {
		field_arcs.append(fieldSize.field_arcs(i));
	}

	this->penalty_area_depth = FieldConstantsRoboCup2018A::kPenaltyAreaDepth;
	this->penalty_area_width = FieldConstantsRoboCup2018A::kPenaltyAreaWidth;
	this->center_circle_radius = FieldConstantsRoboCup2018A::kCenterCircleRadius;
#endif

	this->field_length = fieldSize.field_length();
	this->field_width = fieldSize.field_width();
	this->boundary_width = fieldSize.boundary_width();
	this->goal_width = fieldSize.goal_width();
	this->goal_depth = fieldSize.goal_depth();

	this->removeItem ( fieldItem );
	ConstructField();
	fieldItem = this->addPath (*field, *fieldLinePen, *fieldBrush);
}
