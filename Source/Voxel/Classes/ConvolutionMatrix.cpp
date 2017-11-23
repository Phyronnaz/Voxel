// Copyright 2017 Phyronnaz

#include "ConvolutionMatrix.h"


const F2DConvolutionMatrix F2DConvolutionMatrix::BoxBlur = F2DConvolutionMatrix(1, 1, 1, 1, 1, 1, 1, 1, 1) / 9;

F2DConvolutionMatrix::F2DConvolutionMatrix()
	: a0(0), a1(0), a2(0)
	, a3(0), a4(0), a5(0)
	, a6(0), a7(0), a8(0)
{

}

F2DConvolutionMatrix::F2DConvolutionMatrix(float a0, float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8)
	: a0(a0), a1(a1), a2(a2)
	, a3(a3), a4(a4), a5(a5)
	, a6(a6), a7(a7), a8(a8)
{

}

float F2DConvolutionMatrix::GetAt(int i, int j) const
{
	check(-1 <= i && i <= 1);
	check(-1 <= j && j <= 1);
	if (i == -1)
	{
		if (j == -1)
		{
			return a0;
		}
		else if (j == 0)
		{
			return a1;
		}
		else
		{
			return a2;
		}
	}
	else if (i == 0)
	{
		if (j == -1)
		{
			return a3;
		}
		else if (j == 0)
		{
			return a4;
		}
		else
		{
			return a5;
		}
	}
	else
	{
		if (j == -1)
		{
			return a6;
		}
		else if (j == 0)
		{
			return a7;
		}
		else
		{
			return a8;
		}
	}
}

F2DConvolutionMatrix& F2DConvolutionMatrix::operator-=(float Divisor)
{
	a0 -= Divisor;
	a1 -= Divisor;
	a2 -= Divisor;
	a3 -= Divisor;
	a4 -= Divisor;
	a5 -= Divisor;
	a6 -= Divisor;
	a7 -= Divisor;
	a8 -= Divisor;

	return *this;
}

F2DConvolutionMatrix F2DConvolutionMatrix::operator-(const F2DConvolutionMatrix& Other) const
{
	return F2DConvolutionMatrix(*this) -= Other;
}

F2DConvolutionMatrix F2DConvolutionMatrix::operator-(float Divisor) const
{
	return F2DConvolutionMatrix(*this) -= Divisor;
}

F2DConvolutionMatrix& F2DConvolutionMatrix::operator+=(float Divisor)
{
	a0 += Divisor;
	a1 += Divisor;
	a2 += Divisor;
	a3 += Divisor;
	a4 += Divisor;
	a5 += Divisor;
	a6 += Divisor;
	a7 += Divisor;
	a8 += Divisor;

	return *this;
}

F2DConvolutionMatrix F2DConvolutionMatrix::operator+(const F2DConvolutionMatrix& Other) const
{
	return F2DConvolutionMatrix(*this) += Other;
}

F2DConvolutionMatrix F2DConvolutionMatrix::operator+(float Divisor) const
{
	return F2DConvolutionMatrix(*this) += Divisor;
}

F2DConvolutionMatrix& F2DConvolutionMatrix::operator*=(float Divisor)
{
	a0 *= Divisor;
	a1 *= Divisor;
	a2 *= Divisor;
	a3 *= Divisor;
	a4 *= Divisor;
	a5 *= Divisor;
	a6 *= Divisor;
	a7 *= Divisor;
	a8 *= Divisor;

	return *this;
}

F2DConvolutionMatrix F2DConvolutionMatrix::operator*(const F2DConvolutionMatrix& Other) const
{
	return F2DConvolutionMatrix(*this) *= Other;
}

F2DConvolutionMatrix F2DConvolutionMatrix::operator*(float Divisor) const
{
	return F2DConvolutionMatrix(*this) *= Divisor;
}

F2DConvolutionMatrix& F2DConvolutionMatrix::operator/=(float Divisor)
{
	a0 /= Divisor;
	a1 /= Divisor;
	a2 /= Divisor;
	a3 /= Divisor;
	a4 /= Divisor;
	a5 /= Divisor;
	a6 /= Divisor;
	a7 /= Divisor;
	a8 /= Divisor;

	return *this;
}



F2DConvolutionMatrix F2DConvolutionMatrix::operator/(const F2DConvolutionMatrix& Other) const
{
	return F2DConvolutionMatrix(*this) /= Other;
}

F2DConvolutionMatrix F2DConvolutionMatrix::operator/(float Divisor) const
{
	return F2DConvolutionMatrix(*this) /= Divisor;
}

F2DConvolutionMatrix& F2DConvolutionMatrix::operator+=(const F2DConvolutionMatrix& Other)
{
	a0 += Other.a0;
	a1 += Other.a1;
	a2 += Other.a2;
	a3 += Other.a3;
	a4 += Other.a4;
	a5 += Other.a5;
	a6 += Other.a6;
	a7 += Other.a7;
	a8 += Other.a8;

	return *this;
}

F2DConvolutionMatrix& F2DConvolutionMatrix::operator-=(const F2DConvolutionMatrix& Other)
{
	a0 -= Other.a0;
	a1 -= Other.a1;
	a2 -= Other.a2;
	a3 -= Other.a3;
	a4 -= Other.a4;
	a5 -= Other.a5;
	a6 -= Other.a6;
	a7 -= Other.a7;
	a8 -= Other.a8;

	return *this;
}

F2DConvolutionMatrix& F2DConvolutionMatrix::operator*=(const F2DConvolutionMatrix& Other)
{
	a0 *= Other.a0;
	a1 *= Other.a1;
	a2 *= Other.a2;
	a3 *= Other.a3;
	a4 *= Other.a4;
	a5 *= Other.a5;
	a6 *= Other.a6;
	a7 *= Other.a7;
	a8 *= Other.a8;

	return *this;
}
F2DConvolutionMatrix& F2DConvolutionMatrix::operator/=(const F2DConvolutionMatrix& Other)
{
	a0 /= Other.a0;
	a1 /= Other.a1;
	a2 /= Other.a2;
	a3 /= Other.a3;
	a4 /= Other.a4;
	a5 /= Other.a5;
	a6 /= Other.a6;
	a7 /= Other.a7;
	a8 /= Other.a8;

	return *this;
}





F3DConvolutionMatrix::F3DConvolutionMatrix()
{

}

F3DConvolutionMatrix::F3DConvolutionMatrix(const F2DConvolutionMatrix& A0, const F2DConvolutionMatrix& A1, const F2DConvolutionMatrix& A2)
	: A0(A0), A1(A1), A2(A2)
{

}

float F3DConvolutionMatrix::GetAt(int i, int j, int k) const
{
	check(-1 <= i && i <= 1);
	check(-1 <= j && j <= 1);
	check(-1 <= k && k <= 1);
	if (k == -1)
	{
		return A0.GetAt(i, j);
	}
	else if (k == 0)
	{
		return A1.GetAt(i, j);
	}
	else
	{
		return A2.GetAt(i, j);
	}
}

const F3DConvolutionMatrix F3DConvolutionMatrix::BoxBlur = F3DConvolutionMatrix(F2DConvolutionMatrix::BoxBlur / 3, F2DConvolutionMatrix::BoxBlur / 3, F2DConvolutionMatrix::BoxBlur / 3);




F2DConvolutionMatrix UConvolutionMatrixUtilities::Make2DConvolutionMatrix(FVector Top, FVector Middle, FVector Bottom)
{
	return F2DConvolutionMatrix(Top.X, Top.Y, Top.Z, Middle.X, Middle.Y, Middle.Z, Bottom.X, Bottom.Y, Bottom.Z);
}

F2DConvolutionMatrix UConvolutionMatrixUtilities::Get2DBoxBlur()
{
	return F2DConvolutionMatrix::BoxBlur;
}



F3DConvolutionMatrix UConvolutionMatrixUtilities::Make3DConvolutionMatrix(F2DConvolutionMatrix Top, F2DConvolutionMatrix Middle, F2DConvolutionMatrix Bottom)
{
	return F3DConvolutionMatrix(Top, Middle, Bottom);
}

F3DConvolutionMatrix UConvolutionMatrixUtilities::Get3DBoxBlur()
{
	return F3DConvolutionMatrix::BoxBlur;
}

F3DConvolutionMatrix UConvolutionMatrixUtilities::Get3DSharpen()
{
	F2DConvolutionMatrix Side(
		0, 0, 0,
		0, -1, 0,
		0, 0, 0);
	F2DConvolutionMatrix Center(
		0, -1, 0,
		-1, 7, -1,
		0, -1, 0
	);

	return F3DConvolutionMatrix(Side, Center, Side);
}

F3DConvolutionMatrix UConvolutionMatrixUtilities::Get3DGaussianBlur()
{
	F2DConvolutionMatrix Side(
		1, 1, 1,
		1, 2, 1,
		1, 1, 1);
	F2DConvolutionMatrix Center(
		1, 2, 1,
		2, 4, 2,
		1, 2, 1
	);

	return F3DConvolutionMatrix(Side / 36, Center / 36, Side / 36);
}
