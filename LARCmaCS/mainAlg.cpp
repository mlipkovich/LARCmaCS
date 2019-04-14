// Copyright 2019 Dmitrii Iarosh

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

// http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mainAlg.h"
#include <iostream>
#include <windows.h>
#include <QtWidgets/QApplication>

using namespace std;

#include "message.h"

MainAlg::MainAlg()
{
	mWorker = new MainAlgWorker();
	mThread = new QThread();
}

MainAlg::~MainAlg()
{
	stop();
}

void MainAlg::init(Receiver * receiver)
{
	mReceiver = receiver;
	mPacketSSL = QSharedPointer<PacketSSL>(new PacketSSL());
	mWorker->moveToThread(mThread);
	cout << "Init mainAlg ok" << endl;
	connect(mThread, SIGNAL(started()), mWorker, SLOT(start()));
	connect(this, SIGNAL(wstop()), mWorker, SLOT(stop()));
	connect(mWorker, SIGNAL(finished()), mWorker, SLOT(deleteLater()));
	connect(mThread, SIGNAL(finished()), mThread, SLOT(deleteLater()));
	connect(mWorker, SIGNAL(finished()), mThread, SLOT(quit()));
	connect(this, SIGNAL(MLEvalString(const QString &)), mWorker, SLOT(EvalString(const QString &)));
	connect(mWorker, SIGNAL(getDataFromReceiver()), this, SLOT(loadVisionData()), Qt::DirectConnection);
	connect(mWorker, SIGNAL(newPauseState(const QString &)), this, SLOT(receivePauseState(const QString &)));
	connect(mWorker, SIGNAL(sendStatistics(const QString &)), this, SLOT(sendStatistics(const QString &)));
	connect(mWorker, SIGNAL(sendToConnector(int, const QByteArray &)), this, SLOT(moveToConnector(int, const QByteArray &)));
	connect(mWorker, SIGNAL(sendToSimConnector(const QByteArray &)), this, SLOT(moveToSimConnector(const QByteArray &)));
	connect(this, SIGNAL(updateEnableSimFlag(bool)), mWorker, SLOT(setEnableSimFlag(bool)));
	connect(this, SIGNAL(updateBallStatus(bool)), mWorker, SLOT(changeBallStatus(bool)));
}

void MainAlg::sendStatistics(const QString & statistics)
{
	emit StatusMessage(statistics);
}

void MainAlg::changeBallStatus(bool status)
{
	emit updateBallStatus(status);
}

bool MainAlg::getIsSimEnabledFlag()
{
	return mWorker->getIsSimEnabledFlag();
}

void MainAlg::setEnableSimFlag(bool flag)
{
	emit updateEnableSimFlag(flag);
}

void MainAlg::EvalString(const QString & s)
{
	emit MLEvalString(s);
}

void MainAlg::moveToConnector(int N, const QByteArray & command)
{
	emit sendToConnector(N, command);
}

void MainAlg::moveToSimConnector(const QByteArray & command)
{
	emit sendToSimConnector(command);
}

void MainAlg::loadVisionData()
{
	mReceiver->swapDataVectors();
	QSharedPointer<pair<QSharedPointer<QVector<QSharedPointer<SSL_WrapperPacket> > >, QSharedPointer<SSL_WrapperPacket> > > tmp = mReceiver->getVisionData();
	QSharedPointer<QVector<QSharedPointer<SSL_WrapperPacket> > > detectionPackets = tmp->first;
	int balls_n, idCam, robots_blue_n, robots_yellow_n;
	SSL_DetectionBall ball;

	QSharedPointer<SSL_WrapperPacket> packet;
	for (int i = 0; i < TEAM_COUNT / 4; i++) {
			mPacketSSL->robots_blue[i] = 0;
			mPacketSSL->robots_yellow[i] = 0;
	}
	mPacketSSL->balls[0] = 0;
	if (detectionPackets.isNull())
	{
		return;
	}
	for (int i = 0; i < detectionPackets->size(); i++) {
		packet = detectionPackets->at(i);
		if (packet.isNull()) {
			continue;
		}
		SSL_DetectionRobot robot;
		SSL_DetectionFrame detection = packet->detection();

		idCam = detection.camera_id() + 1;
		balls_n = detection.balls_size();

		// [Start] Ball info
		if (balls_n != 0) {
			mPacketSSL->balls[0] = idCam;
			ball = detection.balls(0);
			mPacketSSL->balls[1] = ball.x();
			mPacketSSL->balls[2] = ball.y();
		}
		// [End] Ball info

		// [Start] Robot info
		robots_blue_n = detection.robots_blue_size();
		robots_yellow_n = detection.robots_yellow_size();

		for (int i = 0; i < robots_blue_n; i++) {
			robot = detection.robots_blue(i);
			mPacketSSL->robots_blue[robot.robot_id()] = idCam;
			mPacketSSL->robots_blue[robot.robot_id() + 12] = robot.x();
			mPacketSSL->robots_blue[robot.robot_id() + 24] = robot.y();
			mPacketSSL->robots_blue[robot.robot_id() + 36] = robot.orientation();
		}

		for (int i = 0; i < robots_yellow_n; i++) {
			robot = detection.robots_yellow(i);
			mPacketSSL->robots_yellow[robot.robot_id()] = idCam;
			mPacketSSL->robots_yellow[robot.robot_id() + 12] = robot.x();
			mPacketSSL->robots_yellow[robot.robot_id() + 24] = robot.y();
			mPacketSSL->robots_yellow[robot.robot_id() + 36] = robot.orientation();
		}
		// [End] Robot info
	}

	mWorker->setPacketSSL(mPacketSSL);
}

void MainAlg::receivePauseState(const QString & state)
{
	emit UpdatePauseState(state);
}

void MainAlg::start()
{
	mThread->start();
}

void MainAlg::stop()
{
	emit wstop();
}
