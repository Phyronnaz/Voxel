// Copyright 2017 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelSave.h"
#include "IPv4Endpoint.h"
#include <deque>

#define PACKET_SIZE_IN_DIFF 250

class FSocket;
class FTcpListener;
class AVoxelWorld;


class FVoxelTcpClient
{
public:
	FVoxelTcpClient();
	~FVoxelTcpClient();

	void ConnectTcpClient(const FString& Ip, const int32 Port);

	void ReceiveDiffs(std::deque<FVoxelValueDiff>& OutValueDiffs, std::deque<FVoxelMaterialDiff>& OutMaterialDiffs);

	void ReceiveSave(FVoxelWorldSave& OutSave);

	bool IsValid() const;

	bool IsNextUpdateRemoteLoad();

	void UpdateExpectedSize();

private:
	FSocket* Socket;

	bool bExpectedSizeUpToDate;
	uint32 ExpectedSize;
	bool bNextUpdateIsRemoteLoad;
};

class FVoxelTcpServer
{
public:
	FVoxelTcpServer();
	~FVoxelTcpServer();

	void SetWorld(AVoxelWorld* World);

	void StartTcpServer(const FString& Ip, const int32 Port);

	bool Accept(FSocket* NewSocket, const FIPv4Endpoint& Endpoint);

	bool IsValid();

	void SendValueDiffs(std::deque<FVoxelValueDiff>& Diffs);
	void SendMaterialDiffs(std::deque<FVoxelMaterialDiff>& Diffs);

	void SendSave(FVoxelWorldSave& Save, bool bOnlyToNewConnections);

private:
	FTcpListener* TcpListener;
	TArray<FSocket*> Sockets;
	TArray<FSocket*> SocketsToSendSave;
	FCriticalSection SocketsLock;
	AVoxelWorld* World;

	template<typename T>
	void SendDiffs(std::deque<T>& DiffList, bool bIsValues);
};