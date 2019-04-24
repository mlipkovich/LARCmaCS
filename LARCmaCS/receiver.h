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

#include <QObject>
#include <QSharedPointer>
#include <QThread>

#include "packetSSL.h"
#include "receiverWorker.h"
#include "sharedRes.h"

struct Receiver : public QObject
{
	Q_OBJECT

public:
	Receiver();
	~Receiver();
	void init(SharedRes * sharedRes);
	void start();
	void stop();

public slots:
	void updateDetection(const QSharedPointer<SSL_WrapperPacket> & detection, int camID);
	void updateGeometry(const QSharedPointer<SSL_WrapperPacket> & geometry);
	void clearScene();
	void setDisplayFlag();
	void changeSimulatorMode(bool mode);

	signals:
	void updateSimulatorMode(bool mode);
	void clearField();
	void UpdateSSLFPS(const QString & status);
	void wstop();

private:
	QTimer mDisplayTimer;
	ReceiverWorker * mWorker;
	QThread mThread;
	SharedRes * mSharedRes;
	bool mDisplayFlag = false;
};
