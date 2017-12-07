// Copyright 2017 Phyronnaz

#pragma once

enum EDirection { XMin, XMax, YMin, YMax, ZMin, ZMax };

FORCEINLINE EDirection InvertDirection(EDirection Direction)
{
	if (Direction % 2 == 0)
	{
		return (EDirection)(Direction + 1);
	}
	else
	{
		return (EDirection)(Direction - 1);
	}
}