#include "connector.h"

#include <QMap>

#include "grSimRobot.h"
#include "defaultRobot.h"

Connector::Connector(SharedRes * sharedRes)
	: mSharedRes(sharedRes)
	, mUdpSocket(this)
	, mStatisticsTimer(this)
{}

Connector::~Connector()
{}

const QString & Connector::getGrSimIP()
{
	return mGrSimIP;
}

unsigned short Connector::getGrSimPort()
{
	return mGrSimPort;
}

unsigned short Connector::getRobotPort()
{
	return mRobotPort;
}

void Connector::run(int N, const QByteArray & command)
{
	mUdpSocket.writeDatagram(command, QHostAddress(mSharedRes->getRobotIP(N)), DefaultRobot::robotPort);
}

void Connector::runSim(const QByteArray & command)
{
	mUdpSocket.writeDatagram(command, QHostAddress(mGrSimIP), mGrSimPort);
}

void Connector::onConnectorChange(bool isSim, const QString &ip, int port)
{
	mIsSim = isSim;

	if (mIsSim) {
		mGrSimIP = ip;
		mGrSimPort = port;
	}
}

void Connector::sendNewCommand(const QVector<double> & newmess)
{
	if (newmess[0] == 1) {
		QByteArray command;

		int voltage = 12; //fixed while we don't have abilities to change it from algos
		bool simFlag = mIsSim;
		if (!simFlag) {
			if (!mIsPause) {
				DefaultRobot::formControlPacket(command, newmess[1], newmess[3], newmess[2], newmess[5],
						newmess[6], newmess[4], voltage, 0);
			} else {
				DefaultRobot::formControlPacket(command, newmess[1], 0, 0, 0, 0, 0, voltage, 0);
			}
		} else {
			if (!mIsPause) {
				GrSimRobot::formControlPacket(command, newmess[1], newmess[3], newmess[2], newmess[5],
						newmess[6], newmess[4], voltage, 0);
			} else {
				GrSimRobot::formControlPacket(command, newmess[1], 0, 0, 0, 0, 0, voltage, 0);
			}
		}

		if (newmess[1] == 0) {
			for (int i = 1; i <= Constants::maxNumOfRobots; i++) {
				if (!simFlag) {
					emit run(i, command);
				} else {
					QByteArray multiCommand;
					GrSimRobot::formControlPacket(multiCommand, i, newmess[3], newmess[2], newmess[5],
							newmess[6], newmess[4], voltage, 0);
					emit runSim(multiCommand);
				}
			}
		}

		if ((newmess[1] > 0) && (newmess[1] <= Constants::maxNumOfRobots)) {
			if (!simFlag) {
				emit run(newmess[1], command);
			} else {
				emit runSim(command);
			}
		}
	}
}

void Connector::onPauseChanged(bool status)
{
	qDebug() << "onPauseChanged" << status;
	mIsPause = status;

	if (mIsPause) { //TODO: add check of remote control
		QByteArray command;
		if (!mIsSim) {
			for (int i = 1; i <= 12; i++) {
				DefaultRobot::formControlPacket(command, i, 0, 0, 0, 0, 0, 0, 0);
				run(i, command);
			}
		} else {
			for (int i = 0; i <= 12; i++) {
				GrSimRobot::formControlPacket(command, i, 0, 0, 0, 0, 0, 0, 0);
				runSim(command); //for more power of remote control
			}
		}
	}
}
