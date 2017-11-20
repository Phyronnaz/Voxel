#include "VoxelNetworking.h"
#include "VoxelPrivatePCH.h"

DECLARE_CYCLE_STAT(TEXT("VoxelNetworking ~ Receive data"), STAT_ReceiveData, STATGROUP_Voxel);
DECLARE_CYCLE_STAT(TEXT("VoxelNetworking ~ Send data"), STAT_SendData, STATGROUP_Voxel);

FVoxelTcpClient::FVoxelTcpClient()
	: Socket(nullptr)
	, ExpectedSize(0)
	, bExpectedSizeUpToDate(false)
{

}

FVoxelTcpClient::~FVoxelTcpClient()
{
	if (Socket)
	{
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
	}
}

void FVoxelTcpClient::ConnectTcpClient(const FString& Ip, const int32 Port)
{
	//Create Remote Address.
	TSharedPtr<FInternetAddr> RemoteAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();

	bool bIsValid;
	RemoteAddr->SetIp(*Ip, bIsValid);
	RemoteAddr->SetPort(Port);

	if (!bIsValid)
	{
		UE_LOG(LogTemp, Error, TEXT("IP address was not valid"));
		return;
	}

	FIPv4Endpoint Endpoint(RemoteAddr);

	if (Socket)
	{
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
	}

	Socket = FTcpSocketBuilder(TEXT("RemoteConnection"));

	int BufferSize = 1000000;
	int NewSize;
	Socket->SetReceiveBufferSize(BufferSize, NewSize);
	check(BufferSize == NewSize);

	if (Socket)
	{
		if (!Socket->Connect(*Endpoint.ToInternetAddr()))
		{
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
			Socket = nullptr;
			return;
		}
	}
}

void FVoxelTcpClient::ReceiveData(std::deque<FVoxelValueDiff>& OutValueDiffs, std::deque<FVoxelMaterialDiff>& OutMaterialDiffs)
{
	SCOPE_CYCLE_COUNTER(STAT_ReceiveData);

	if (Socket)
	{
		if (!bExpectedSizeUpToDate)
		{
			UpdateExpectedSize();
		}
		if (bExpectedSizeUpToDate)
		{

			uint32 PendingDataSize = 0;
			if (Socket->HasPendingData(PendingDataSize))
			{
				if (PendingDataSize >= ExpectedSize)
				{
					TArray<uint8> ReceivedData;
					ReceivedData.SetNumUninitialized(ExpectedSize);

					int32 BytesRead = 0;
					Socket->Recv(ReceivedData.GetData(), ReceivedData.Num(), BytesRead);
					check(BytesRead == ExpectedSize);

					FArchiveLoadCompressedProxy Decompressor = FArchiveLoadCompressedProxy(ReceivedData, ECompressionFlags::COMPRESS_ZLIB);
					check(!Decompressor.GetError());

					//Decompress
					FBufferArchive DecompressedDataArray;
					Decompressor << DecompressedDataArray;

					FMemoryReader DecompressedData = FMemoryReader(DecompressedDataArray);

					bool bValues;
					uint32 ItemCount;
					DecompressedData << bValues;
					DecompressedData << ItemCount;

					check(ItemCount <= PACKET_SIZE_IN_DIFF);

					if (bValues)
					{
						for (uint32 i = 0; i < ItemCount; i++)
						{
							FVoxelValueDiff Diff;
							DecompressedData << Diff;
							OutValueDiffs.push_front(Diff);
						}
					}
					else
					{
						for (uint32 i = 0; i < ItemCount; i++)
						{
							FVoxelMaterialDiff Diff;
							DecompressedData << Diff;
							OutMaterialDiffs.push_front(Diff);
						}
					}

					bExpectedSizeUpToDate = false;
					UpdateExpectedSize();
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Client not connected"));
	}
}

bool FVoxelTcpClient::IsValid()
{
	return Socket != nullptr;
}

void FVoxelTcpClient::UpdateExpectedSize()
{
	check(!bExpectedSizeUpToDate);

	uint32 PendingDataSize = 0;
	if (Socket->HasPendingData(PendingDataSize))
	{
		if (PendingDataSize >= 4)
		{
			uint8 ReceivedData[4];

			int BytesRead;
			Socket->Recv(ReceivedData, 4, BytesRead);
			check(BytesRead == 4);

			ExpectedSize = ReceivedData[0] + 256 * (ReceivedData[1] + 256 * (ReceivedData[2] + 256 * ReceivedData[3]));

			bExpectedSizeUpToDate = true;
		}
	}
}





FVoxelTcpServer::FVoxelTcpServer()
	: TcpListener(nullptr)
{

}

FVoxelTcpServer::~FVoxelTcpServer()
{
	for (auto Socket : Sockets)
	{
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
	}
	delete TcpListener;
}

void FVoxelTcpServer::StartTcpServer(const FString& Ip, const int32 Port)
{
	if (TcpListener)
	{
		delete TcpListener;
	}

	FIPv4Address Addr;
	FIPv4Address::Parse(Ip, Addr);

	FIPv4Endpoint Endpoint(Addr, Port);

	TcpListener = new FTcpListener(Endpoint);

	TcpListener->OnConnectionAccepted().BindRaw(this, &FVoxelTcpServer::Accept);
}

bool FVoxelTcpServer::Accept(FSocket* NewSocket, const FIPv4Endpoint& Endpoint)
{
	Sockets.Add(NewSocket);

	int BufferSize = 1000000;
	int NewSize;
	NewSocket->SetSendBufferSize(BufferSize, NewSize);
	check(BufferSize == NewSize);


	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Connected!"));
	return true;
}

bool FVoxelTcpServer::IsValid()
{
	return Sockets.Num() > 0;
}

void FVoxelTcpServer::SendValueDiffs(std::deque<FVoxelValueDiff>& Diffs)
{
	SendData(Diffs, true);
}

void FVoxelTcpServer::SendMaterialDiffs(std::deque<FVoxelMaterialDiff>& Diffs)
{
	SendData(Diffs, false);
}

template<typename T>
void FVoxelTcpServer::SendData(std::deque<T>& DiffList, bool bIsValues)
{
	SCOPE_CYCLE_COUNTER(STAT_SendData);

	int i = 0;
	while (i < DiffList.size())
	{
		uint32 SizeToSend = FMath::Min(PACKET_SIZE_IN_DIFF, (int)DiffList.size() - i);

		FBufferArchive Writer;
		Writer << bIsValues;
		Writer << SizeToSend;

		for (uint32 k = 0; k < SizeToSend; k++)
		{
			Writer << DiffList[i];
			i++;
		}

		TArray<uint8> DataToSend;
		FArchiveSaveCompressedProxy Compressor = FArchiveSaveCompressedProxy(DataToSend, ECompressionFlags::COMPRESS_ZLIB);
		// Send entire binary array/archive to compressor
		Compressor << Writer;
		// Send archive serialized data to binary array
		Compressor.Flush();

		for (auto Socket : Sockets)
		{
			// Send
			int32 BytesSent = 0;

			uint8 Data[4];

			uint32 Tmp = DataToSend.Num();
			Data[0] = Tmp % 256;
			Tmp /= 256;
			Data[1] = Tmp % 256;
			Tmp /= 256;
			Data[2] = Tmp % 256;
			Tmp /= 256;
			Data[3] = Tmp % 256;

			Socket->Send(Data, 4, BytesSent);
			if (BytesSent != 4)
			{
				UE_LOG(LogTemp, Error, TEXT("Header failed to send"));
			}
			else
			{
				bool bSuccess = Socket->Send(DataToSend.GetData(), DataToSend.Num(), BytesSent);
				UE_LOG(LogTemp, Log, TEXT("Bytes to send: %d. Bytes sent: %d. Success: %d"), DataToSend.Num(), BytesSent, bSuccess);
			}
		}
	}
}