﻿// Copyright 2019 Dmitrii Iarosh

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

// http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <iostream>
#include <QObject>
#include <QThread>
#include <QSharedPointer>

#include "packetSSL.h"

#include "receiverWorker.h"

using namespace std;

#include <time.h>       /* clock_t, clock(), CLOCKS_PER_SEC */

struct Receiver : public QObject
{
	Q_OBJECT
private:
	QTimer mDisplayTimer;
	ReceiverWorker * mWorker;
	QThread * mThread;
	bool mDisplayFlag = false;

public:
	const QSharedPointer<pair<QSharedPointer<QVector<QSharedPointer<SSL_WrapperPacket> > >, QSharedPointer<SSL_WrapperPacket> > > & getVisionData();
	Receiver();
	~Receiver();
	void init();
	void start();
	void stop();

public slots:
	void swapDataVectors();
	void sendStatistics(const QString & statistics);
	void clearScene();
	void ChangeSimulatorMode(bool mode);
	void setDisplayFlag();

signals:
	void updateSimulatorMode(bool mode);
	void clearField();
	void UpdateSSLFPS(const QString & status);
	void wstop();
};
