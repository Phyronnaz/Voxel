// Copyright 2017 Phyronnaz

#include "VoxelWorldDetails.h"
#include "VoxelWorld.h"
#include "VoxelWorldEditor.h"
#include "PropertyHandle.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "DetailCategoryBuilder.h"
#include "IDetailsView.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "LevelEditorViewport.h"
#include "Editor.h"
#include "IDetailPropertyRow.h"
#include "Engine.h"



#include "Containers/Array.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Templates/SharedPointer.h"
#include "Toolkits/AssetEditorToolkit.h"

#include "Runtime/Launch/Resources/Version.h"

DEFINE_LOG_CATEGORY(VoxelEditorLog)

FVoxelWorldDetails::FVoxelWorldDetails()
{

}

FVoxelWorldDetails::~FVoxelWorldDetails()
{

}

TSharedRef<IDetailCustomization> FVoxelWorldDetails::MakeInstance()
{
	return MakeShareable(new FVoxelWorldDetails());
}

void FVoxelWorldDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
#if ENGINE_MINOR_VERSION == 17
	const TArray< TWeakObjectPtr<UObject>>& SelectedObjects = DetailLayout.GetDetailsView().GetSelectedObjects();
#else
	const TArray<TWeakObjectPtr<AActor>>& SelectedObjects = DetailLayout.GetDetailsView()->GetSelectedActors();
#endif
	for (int32 ObjectIndex = 0; ObjectIndex < SelectedObjects.Num(); ++ObjectIndex)
	{
#if ENGINE_MINOR_VERSION == 17
		const TWeakObjectPtr<UObject>& CurrentObject = SelectedObjects[ObjectIndex];
#else
		const TWeakObjectPtr<AActor>& CurrentObject = SelectedObjects[ObjectIndex];
#endif
		if (CurrentObject.IsValid())
		{
			AVoxelWorld* CurrentCaptureActor = Cast<AVoxelWorld>(CurrentObject.Get());
			if (CurrentCaptureActor)
			{
				World = CurrentCaptureActor;
				break;
			}
		}
	}

	DetailLayout.EditCategory("Voxel")
		.AddCustomRow(FText::FromString(TEXT("Update")))
		.NameContent()
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		.Text(FText::FromString(TEXT("Toggle world preview")))
		]
	.ValueContent()
		.MaxDesiredWidth(125.f)
		.MinDesiredWidth(125.f)
		[
			SNew(SButton)
			.ContentPadding(2)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked(this, &FVoxelWorldDetails::OnWorldPreviewToggle)
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		.Text(FText::FromString(TEXT("Toggle")))
		]
		];
}

FReply FVoxelWorldDetails::OnWorldPreviewToggle()
{
	if (World.IsValid())
	{
		World->VoxelWorldEditorClass = AVoxelWorldEditor::StaticClass();

		if (World->IsCreated())
		{
			World->DestroyInEditor();
		}
		else
		{
			World->CreateInEditor();
		}
	}

	return FReply::Handled();
}