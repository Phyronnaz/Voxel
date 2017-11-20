#pragma once

#include "CoreMinimal.h"
#include "Networking.h"
#include "Engine.h"
#include <deque>

#define PACKET_SIZE_IN_DIFF 250

class FVoxelTcpClient
{
public:
	FVoxelTcpClient();
	~FVoxelTcpClient();

	void ConnectTcpClient(const FString& Ip, const int32 Port);

	void ReceiveData(std::deque<FVoxelValueDiff>& OutValueDiffs, std::deque<FVoxelMaterialDiff>& OutMaterialDiffs);

	bool IsValid();

private:
	FSocket* Socket;

	bool bExpectedSizeUpToDate;
	uint32 ExpectedSize;

	void UpdateExpectedSize();
};

class FVoxelTcpServer
{
public:
	FVoxelTcpServer();
	~FVoxelTcpServer();

	void StartTcpServer(const FString& Ip, const int32 Port);

	bool Accept(FSocket* NewSocket, const FIPv4Endpoint& Endpoint);

	bool IsValid();

	void SendValueDiffs(std::deque<FVoxelValueDiff>& Diffs);
	void SendMaterialDiffs(std::deque<FVoxelMaterialDiff>& Diffs);

private:
	FTcpListener* TcpListener;
	TArray<FSocket*> Sockets;

	template<typename T>
	void SendData(std::deque<T>& DiffList, bool bIsValues);
};