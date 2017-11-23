// Copyright 2017 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ConvolutionMatrix.generated.h"

USTRUCT(BlueprintType)
struct VOXEL_API F2DConvolutionMatrix
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere)
		float a0;
	UPROPERTY(EditAnywhere)
		float a1;
	UPROPERTY(EditAnywhere)
		float a2;
	UPROPERTY(EditAnywhere)
		float a3;
	UPROPERTY(EditAnywhere)
		float a4;
	UPROPERTY(EditAnywhere)
		float a5;
	UPROPERTY(EditAnywhere)
		float a6;
	UPROPERTY(EditAnywhere)
		float a7;
	UPROPERTY(EditAnywhere)
		float a8;

	static const F2DConvolutionMatrix BoxBlur;

	F2DConvolutionMatrix();

	F2DConvolutionMatrix(float a0, float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8);

	float GetAt(int i, int j) const;


	F2DConvolutionMatrix& operator+=(float Divisor);
	F2DConvolutionMatrix& operator-=(float Divisor);
	F2DConvolutionMatrix& operator*=(float Divisor);
	F2DConvolutionMatrix& operator/=(float Divisor);

	F2DConvolutionMatrix& operator+=(const F2DConvolutionMatrix& Other);
	F2DConvolutionMatrix& operator-=(const F2DConvolutionMatrix& Other);
	F2DConvolutionMatrix& operator*=(const F2DConvolutionMatrix& Other);
	F2DConvolutionMatrix& operator/=(const F2DConvolutionMatrix& Other);

	F2DConvolutionMatrix operator+(float Divisor) const;
	F2DConvolutionMatrix operator-(float Divisor) const;
	F2DConvolutionMatrix operator*(float Divisor) const;
	F2DConvolutionMatrix operator/(float Divisor) const;

	F2DConvolutionMatrix operator+(const F2DConvolutionMatrix& Other) const;
	F2DConvolutionMatrix operator-(const F2DConvolutionMatrix& Other) const;
	F2DConvolutionMatrix operator*(const F2DConvolutionMatrix& Other) const;
	F2DConvolutionMatrix operator/(const F2DConvolutionMatrix& Other) const;
};

USTRUCT(BlueprintType)
struct VOXEL_API F3DConvolutionMatrix
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere)
		F2DConvolutionMatrix A0;
	UPROPERTY(EditAnywhere)
		F2DConvolutionMatrix A1;
	UPROPERTY(EditAnywhere)
		F2DConvolutionMatrix A2;

	static const F3DConvolutionMatrix BoxBlur;

	F3DConvolutionMatrix();

	F3DConvolutionMatrix(const F2DConvolutionMatrix& A0, const F2DConvolutionMatrix& A1, const F2DConvolutionMatrix& A2);

	float GetAt(int i, int j, int k) const;
};


UCLASS()
class VOXEL_API UConvolutionMatrixUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Utilities|Struct", meta = (Keywords = "construct build", NativeMakeFunc))
		static F2DConvolutionMatrix Make2DConvolutionMatrix(FVector Top, FVector Middle, FVector Bottom);

	UFUNCTION(BlueprintPure, Category = "Voxel")
		static F2DConvolutionMatrix Get2DBoxBlur();

	UFUNCTION(BlueprintPure, Category = "Utilities|Struct", meta = (Keywords = "construct build", NativeMakeFunc))
		static F3DConvolutionMatrix Make3DConvolutionMatrix(F2DConvolutionMatrix Top, F2DConvolutionMatrix Middle, F2DConvolutionMatrix Bottom);

	UFUNCTION(BlueprintPure, Category = "Voxel")
		static F3DConvolutionMatrix Get3DBoxBlur();

	UFUNCTION(BlueprintPure, Category = "Voxel")
		static F3DConvolutionMatrix Get3DSharpen();

	UFUNCTION(BlueprintPure, Category = "Voxel")
		static F3DConvolutionMatrix Get3DGaussianBlur();
};