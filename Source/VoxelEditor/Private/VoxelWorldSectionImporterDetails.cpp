#include "VoxelWorldSectionImporterDetails.h"
#include "VoxelWorldSectionImporter.h"

#include "PropertyHandle.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "DetailCategoryBuilder.h"
#include "IDetailsView.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Editor.h"

#include "Runtime/Launch/Resources/Version.h"

TSharedRef<IDetailCustomization> FVoxelWorldSectionImporterDetails::MakeInstance()
{
	return MakeShareable(new FVoxelWorldSectionImporterDetails());
}

void FVoxelWorldSectionImporterDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
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
			AVoxelWorldSectionImporter* CurrentCaptureActor = Cast<AVoxelWorldSectionImporter>(CurrentObject.Get());
			if (CurrentCaptureActor != NULL)
			{
				Importer = CurrentCaptureActor;
				break;
			}
		}
	}

	DetailLayout.EditCategory("Create VoxelDataAsset from World section")
		.AddCustomRow(FText::FromString(TEXT("Import")))
		.NameContent()
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		.Text(FText::FromString(TEXT("Import from world")))
		]
	.ValueContent()
		.MaxDesiredWidth(125.f)
		.MinDesiredWidth(125.f)
		[
			SNew(SButton)
			.ContentPadding(2)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked(this, &FVoxelWorldSectionImporterDetails::OnImport)
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		.Text(FText::FromString(TEXT("Create")))
		]
		];

	DetailLayout.EditCategory("Import configuration")
		.AddCustomRow(FText::FromString(TEXT("Import")))
		.NameContent()
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		.Text(FText::FromString(TEXT("Copy actors positions to corners")))
		]
	.ValueContent()
		.MaxDesiredWidth(125.f)
		.MinDesiredWidth(125.f)
		[
			SNew(SButton)
			.ContentPadding(2)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked(this, &FVoxelWorldSectionImporterDetails::OnImportFromActors)
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		.Text(FText::FromString(TEXT("Copy")))
		]
		];
}

FReply FVoxelWorldSectionImporterDetails::OnImport()
{
	if (Importer.IsValid())
	{
		if (Importer->World)
		{
			// See \Engine\Source\Editor\UnrealEd\PrivateLevelEditorViewport.cpp:409

			if (Importer->FileName.IsEmpty())
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Please enter a file name")));
				return FReply::Handled();
			}

			FString NewPackageName = PackageTools::SanitizePackageName(TEXT("/Game/") + Importer->SavePath.Path + TEXT("/") + Importer->FileName);
			UPackage* Package = CreatePackage(NULL, *NewPackageName);

			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

			// See if the material asset already exists with the expected name, if it does, ask what to do
			TArray<FAssetData> OutAssetData;
			if (AssetRegistryModule.Get().GetAssetsByPackageName(*NewPackageName, OutAssetData) && OutAssetData.Num() > 0)
			{
				auto DialogReturn = FMessageDialog::Open(EAppMsgType::YesNoCancel, FText::Format(FText::FromString(TEXT("{0} already exists. Replace it?")), FText::FromString(NewPackageName)));
				switch (DialogReturn)
				{
				case EAppReturnType::No:
					return FReply::Handled();
				case EAppReturnType::Yes:
					break;
				case EAppReturnType::Cancel:
					return FReply::Handled();
				default:
					check(false);
				}
			}

			// Create a VoxelLandscapeAsset
			UVoxelDataAssetFactory* DataAssetFactory = NewObject<UVoxelDataAssetFactory>();

			UVoxelDataAsset* DataAsset = (UVoxelDataAsset*)DataAssetFactory->FactoryCreateNew(UVoxelDataAsset::StaticClass(), Package, FName(*(Importer->FileName)), RF_Standalone | RF_Public, NULL, GWarn);

			check(DataAsset);

			FDecompressedVoxelDataAsset Asset;
			Importer->ImportToAsset(Asset);
			DataAsset->InitFromAsset(&Asset);

			// Notify the asset registry
			FAssetRegistryModule::AssetCreated(DataAsset);

			// Set the dirty flag so this package will get saved later
			Package->SetDirtyFlag(true);



			FString Text = NewPackageName + TEXT(" was successfully created");
			FNotificationInfo Info(FText::FromString(Text));
			Info.ExpireDuration = 4.0f;
			FSlateNotificationManager::Get().AddNotification(Info);
		}
		else
		{
			if (!Importer->World)
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Invalid World")));
			}
			else
			{
				check(false);
			}
		}

	}
	return FReply::Handled();
}

FReply FVoxelWorldSectionImporterDetails::OnImportFromActors()
{
	if (Importer.IsValid())
	{
		if (!Importer->World)
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Invalid World")));
		}
		else
		{
			Importer->SetCornersFromActors();
		}
	}
	return FReply::Handled();
}
