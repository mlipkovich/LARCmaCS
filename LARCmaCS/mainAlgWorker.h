#pragma once

#include <QObject>
#include "packetSSL.h"
#include "mlData.h"
#include "client.h"
#include "grSimRobot.h"
#include "defaultRobot.h"
#include "constants.h"

using namespace std;

class MainAlgWorker : public QObject
{
	Q_OBJECT
private:
	Client client;
	bool mIsPause;
	int mTotalPacketsNum = 0;
	int mPacketsPerSecond = 0;
	QSharedPointer<QTimer> mStatisticsTimer;
	QSharedPointer<PacketSSL> mPacketSSL;
	double mIsBallInside;

public:
	MainAlgWorker();
	void setPacketSSL(const QSharedPointer<PacketSSL> & packetSSL);
	bool getIsSimEnabledFlag();
	~MainAlgWorker();

signals:
	void finished();
	void sendToConnector(int N, const QByteArray & command);
	void sendToSimConnector(const QByteArray & command);
	void newPauseState(const QString & state);
	void sendStatistics(const QString & statistics);
	void getDataFromReceiver();

public slots:
	void start();
	void formStatistics();
	void updatePauseState();
	void stop();
	void setEnableSimFlag(bool flag);
	void processPacket(const QSharedPointer<PacketSSL> & packetssl);
	void Pause();
	void run_matlab();
	void stop_matlab();
	void run();
	void evalString(const QString & s);
	void changeBallStatus(bool ballStatus);

private:
	void init();
	bool mIsSimEnabledFlag = 0;
	char mMatlabOutputBuffer[Constants::matlabOutputBufferSize];
	MlData fmldata;
	bool fmtlab;
	bool mShutdownFlag;
	bool pause;
};
